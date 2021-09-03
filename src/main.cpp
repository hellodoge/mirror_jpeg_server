#include "http_server.hpp"
#include "mirror_jpeg_handler.hpp"

int main() {
    server::ServerConfig config {
        .http={
                .mime_type="image/jpeg"
        }
    };
    handler::MirrorJPEGHandler handler {};
    server::HttpServer server {handler, config};
    server.run();
}
