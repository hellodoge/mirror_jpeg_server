#ifndef FLIP_JPEG_SERVER_CONFIG_HPP
#define FLIP_JPEG_SERVER_CONFIG_HPP

#include <chrono>
#include <string_view>

#include "size_literals.hpp"

namespace server {

    using namespace size_literals;

    inline constexpr int default_port = 17070;
    inline constexpr size_t default_max_request_size = 32_MiB; // MiB defined in size_literals.hpp
    inline constexpr std::chrono::seconds default_timeout = std::chrono::seconds(15);

    struct ServerConfig {
        int port = default_port;
        size_t max_request_size = default_max_request_size;
        std::chrono::seconds timeout = default_timeout;

        struct HttpServerConfig {
            std::string_view mime_type;
        } http;
    };
}

#endif //FLIP_JPEG_SERVER_CONFIG_HPP
