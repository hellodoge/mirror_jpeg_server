#include <future>
#include <csignal>
#include <functional>

#include "http_server.hpp"
#include "mirror_jpeg_handler.hpp"
#include "util/logger.hpp"

void wait_for_stop_server_signal(const std::function<void(void)> &callback);

int main() {
    server::ServerConfig config {
        // all the other params are default, see server_config.hpp
        .http={
                .mime_type="image/jpeg"
        }
    };
    Logger logger {std::cout};
    handler::MirrorJPEGHandler handler {};
    server::HttpServer server {handler, config, logger};


    auto server_thread = std::async(std::launch::async, [&]() {
        server.run();
    });

    wait_for_stop_server_signal([&](){
        server.stop();
    });
    server_thread.get();
}

void wait_for_stop_server_signal(const std::function<void(void)> &callback) {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGINT);

    sigprocmask(SIG_BLOCK, &sigset, nullptr /* do not store prev values */);

    int placeholder;
    sigwait(&sigset, &placeholder);

    callback();

    // SIGTERM should be called after traffic switching
    // then, after timeout, SIGKILL (for some corner cases)
}