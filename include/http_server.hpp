#ifndef FLIP_JPEG_HTTP_SERVER_HPP
#define FLIP_JPEG_HTTP_SERVER_HPP

#include <mutex>
#include <functional>
#include <boost/asio.hpp>

#include "server_config.hpp"
#include "handler_interface.hpp"
#include "util/logger.hpp"

namespace server {

    class HttpServer {
    public:
        explicit HttpServer(handler::IHandler &handler, ServerConfig config, Logger &logger)
            : config {config}
            , handler {handler}
            , logger {logger} {}

        void run();
        void stop();

    private:
        std::mutex running_mutex;
        ServerConfig config;
        handler::IHandler &handler;
        boost::asio::io_context context{};
        Logger &logger;
    };

}

#endif //FLIP_JPEG_HTTP_SERVER_HPP
