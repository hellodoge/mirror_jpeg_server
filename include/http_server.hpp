#ifndef FLIP_JPEG_HTTP_SERVER_HPP
#define FLIP_JPEG_HTTP_SERVER_HPP

#include <mutex>
#include <functional>
#include <boost/asio.hpp>

#include "server_config.hpp"
#include "handler_interface.hpp"

namespace server {

    class HttpServer {
    public:
        explicit HttpServer(handler::IHandler &handler, ServerConfig config)
            : config{config}
            , handler{handler} {}

        void run();
        void stop();

    private:
        std::mutex running_mutex;
        ServerConfig config;
        handler::IHandler &handler;
        boost::asio::io_context context{};
    };

}

#endif //FLIP_JPEG_HTTP_SERVER_HPP
