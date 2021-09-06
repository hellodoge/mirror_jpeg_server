#ifndef FLIP_JPEG_LOGGER_HPP
#define FLIP_JPEG_LOGGER_HPP

#include <mutex>
#include <ctime>
#include <iomanip> // std::put_time
#include <iostream>

// Why not just use Boost.Log?
// 1. it is not header-only, so it will need to be built to test the project
// 2. the Boost.Log is a pretty heavy library for such a small project

class Logger {
private:
    // TODO really strange bug, output looks like [20:32736:1155534048]
    static auto get_time_prefix() {
        auto unix_time = std::time(nullptr);
        std::tm time {};
        // std::localtime is not thread safe
        localtime_r(&unix_time, &time);
        return std::put_time(&time, "[%T]: ");
    }

public:
    template <typename ...Args>
    static void log(Args ...args) {
        std::lock_guard lock {mutex};
        std::cout << get_time_prefix();
        (std::cout << ... << args);
        std::cout << std::endl;
    }

private:
    inline static std::mutex mutex;
};

#endif //FLIP_JPEG_LOGGER_HPP
