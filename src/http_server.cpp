#include <chrono>
#include <string_view>

// converting between std::string_view and boost::string_view is trivial
// but creates a mess, option exists for compatibility
#define BOOST_BEAST_USE_STD_STRING_VIEW
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>

#include "util/logger.hpp"
#include "http_server.hpp"

using namespace server;
using namespace size_literals;
using namespace std::chrono_literals;

namespace http = boost::beast::http;
using tcp = boost::asio::ip::tcp;

namespace {
    // used by Task to distinguish if processing error was internal
    // or something was wrong with request
    enum TaskErrorType {
        BadRequest,
        Internal,
    };

    // worker threads use these callbacks to set server response
    struct TaskCallbacks {
        std::function<void(std::vector<uint8_t>)> success;
        std::function<void(TaskErrorType, std::string_view)> error;
    };

    // used by Task class to enqueue requested task to worker thread
    using enqueue_task_func_type = std::function<void(handler::bytes_span, TaskCallbacks)>;

    struct TaskConfig {
        std::chrono::seconds timeout = default_timeout;
        size_t max_request_size = default_max_request_size;
        enqueue_task_func_type enqueue_task;
        std::string_view mime_type;
        Logger &logger;
    };

    // class responsible to handle IO operations with a client
    class Task : public std::enable_shared_from_this<Task> {

        using clock = std::chrono::system_clock;

    public:
        explicit Task(tcp::socket socket, TaskConfig &config) noexcept(false)
            : config {config}
            , socket {std::move(socket)}
            , endpoint {this->socket.remote_endpoint()}
            , timeout {this->socket.get_executor(), config.timeout}
            , enqueue_task_callback {config.enqueue_task}
            , logger {config.logger} {

            request_parser.body_limit(config.max_request_size);
        }

        void run() {
            auto self = shared_from_this();
            timeout.async_wait([self](boost::system::error_code ec){
                self->socket.close();
            });
            get_request();
        }

    private:

        // non blocking
        void get_request() {
            auto self = shared_from_this();

            http::async_read(socket, buffer, request_parser,
                             [self](boost::system::error_code ec, size_t){
                if (ec.failed()) {
                    self->logger.log(self->endpoint, ": error while reading request: ", ec.message());
                    self->shutdown();
                    return;
                }
                self->enqueue_task();
            });
        }

        void enqueue_task() {
            auto self = shared_from_this();

            TaskCallbacks callbacks {
                .success = [self](std::vector<uint8_t> response_data){
                    self->task_succeed(std::move(response_data));
                },
                .error = [self](TaskErrorType type, std::string_view message){
                    self->task_failed(type, message);
                }
            };

            handler::bytes_span body {
                request_parser.get().body()
            };

            enqueued_at = clock::now();
            enqueue_task_callback(body, callbacks);
        }

        void task_succeed(std::vector<uint8_t> response_data) {
            using milliseconds = std::chrono::milliseconds;
            auto in_ms = std::chrono::duration_cast<milliseconds>(clock::now() - enqueued_at);
            logger.log(endpoint, ": processed successfully in ", in_ms.count(), "ms");

            if (!config.mime_type.empty())
                response.set(http::field::content_type, config.mime_type);
            response.body() = std::move(response_data);
            response.prepare_payload(); // set Content-Length etc
            send_response();
        }

        void task_failed(TaskErrorType type, std::string_view message) {
            logger.log(endpoint, ": error while processing: ", message);

            if (type == Internal)
                response.result(http::status::internal_server_error);
            else if (type == BadRequest)
                response.result(http::status::bad_request);

            response.set(http::field::content_type, "text/plain");
            response.body() = std::vector<uint8_t>(message.size() + 1 /* for newline*/);
            std::copy(message.begin(), message.end(), response.body().begin());
            response.body().push_back(static_cast<uint8_t>('\n'));

            response.prepare_payload(); // set Content-Length etc
            send_response();
        }

        // non blocking
        void send_response() {
            auto self = shared_from_this();

            http::async_write(socket, response,
                              [self](boost::system::error_code ec, size_t){
                if (ec.failed())
                    self->logger.log(self->endpoint, ": error while sending response: ", ec.message());
                self->shutdown();
            });
        }

        void shutdown() {
            logger.log(Logger::Debug, endpoint, ": closing connection");

            boost::system::error_code ec;
            timeout.cancel(ec);
            if (ec.failed())
                logger.log(Logger::Debug, endpoint, ": cancel timeout error: ", ec.message());

            socket.shutdown(boost::asio::socket_base::shutdown_both, ec);
            if (ec.failed())
                logger.log(Logger::Debug, endpoint, ": shutdown error: ", ec.message());
        }

    private:
        Logger &logger;
        TaskConfig &config;
        tcp::socket socket;
        tcp::endpoint endpoint;
        boost::asio::steady_timer timeout;
        enqueue_task_func_type enqueue_task_callback;
        std::chrono::time_point<clock> enqueued_at {};

        boost::beast::flat_buffer buffer { 4_KiB }; // used for reading request
        http::request_parser<http::vector_body<uint8_t>> request_parser;
        http::response<http::vector_body<uint8_t>> response;
    };
}

void accept(tcp::acceptor &acceptor, TaskConfig &config, Logger &logger) {
    if (!acceptor.is_open())
        return;
    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket) {
        accept(acceptor, config, logger);
        if (ec.failed()) {
            if (ec.value() != boost::system::errc::operation_canceled)
                logger.log("error while accepting: ", ec.message());
            return;
        }
        try {
            // socket.remote_endpoint can throw system_error
            logger.log(Logger::Debug, "new connection from ", socket.remote_endpoint());
            // Task constructor can also throw error
            std::make_shared<Task>(std::move(socket), config)->run();
        } catch (std::exception &e) {
            logger.log("error creating task ", e.what());
        }
    });
}

constexpr unsigned default_threads_count = 8;

void HttpServer::run() {
    std::unique_lock lock(running_mutex, std::try_to_lock);
    if (!lock.owns_lock())
        throw std::runtime_error("attempt to run server while it is already running");

    const unsigned cpu_threads_count = std::thread::hardware_concurrency();
    const unsigned num_threads = cpu_threads_count != 0 ? cpu_threads_count : default_threads_count;
    boost::asio::thread_pool pool(num_threads);

    auto enqueue_task_callback = [this, &pool](handler::bytes_span request, TaskCallbacks callback){
        boost::asio::post(pool, [this, request, callback=std::move(callback)](){
            try {
                auto result = handler.handle(request);
                callback.success(result);
            } catch (handler::handling_error &e) {
                callback.error(BadRequest, e.what());
            } catch (std::exception &e) {
                callback.error(Internal, "internal server error");
                logger.log(Logger::Error, "internal error: ", e.what());
            }
        });
    };

    TaskConfig taskConfig {
        .timeout = config.timeout,
        .max_request_size = config.max_request_size,
        .enqueue_task = enqueue_task_callback,
        .mime_type = config.http.mime_type,
        .logger = logger
    };

    tcp::acceptor acceptor {context, tcp::endpoint(tcp::v4(), config.port)};
    accept(acceptor, taskConfig, logger);

    // main server loop
    while (!context.stopped()) {
        try {
            context.run();
        } catch (std::exception &e) {
            logger.log(Logger::Error, "unhandled exception: ", e.what());
        }
        if (!context.stopped())
            context.restart();
    }

    // context seems to be stopped, but we need to fulfil
    // requests of connected clients (might be due to timeout)

    // but first we close gateway for new connections
    acceptor.close();

    // then serve the others
    // (context.run() will return when all the work was finished)
    context.restart();
    context.run();

    // threads will be stopped and joined by thread.pool destructor
}

void HttpServer::stop() {
    logger.log("stopping server");
    context.stop();
}
