#include "http_server.hpp"
#include "mirror_jpeg_handler.hpp"
#include "util/logger.hpp"

int main() {
    server::ServerConfig config {
    	// all the other params are default, see server_config.hpp
        .http={
                .mime_type="image/jpeg"
        }
    };
    Logger logger {};
    handler::MirrorJPEGHandler handler {};
    server::HttpServer server {handler, config, logger};
    server.run();
}
