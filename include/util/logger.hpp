#ifndef FLIP_JPEG_LOGGER_HPP
#define FLIP_JPEG_LOGGER_HPP

#include <mutex>
#include <ctime>
#include <iomanip> // std::put_time
#include <ostream>
#include <iostream>

// Why not just use Boost.Log?
// 1. it is not header-only, so it will need to be built to test the project
// 2. the Boost.Log is a pretty heavy library for such a small project

class Logger {

public:
    enum Level {
        Debug = 0,
        Info  = 1,
        Error = 2
    };

private:
    static constexpr Level default_min_level = Info;
    static constexpr Level default_level     = Info;

public:
    Logger() = default;
    explicit Logger(Level level) : min_level{level} {};
    explicit Logger(std::ostream &out) : out {out} {};
    Logger(Level level, std::ostream &out)
        : min_level{level}
        , out {out} {};

private:
    void print_time_prefix() {
        auto unix_time = std::time(nullptr);
        std::tm time {};
        // std::localtime is not thread safe
        // we have locked the static mutex, but still cannot guarantee that std::localtime
        // won't be used by other component of the project in other thread
        localtime_r(&unix_time, &time);
        out << std::put_time(&time, "[%T]: ");
    }

public:
    template <typename ...Args>
    void log(Level level, Args ...args) {
        if (level < min_level)
            return;

        std::lock_guard lock {mutex};
        print_time_prefix();
        (out << ... << args);
        out << std::endl;
    }

    template <typename ...Args>
    void log(Args ...args) {
        log(default_level, args...);
    }

private:
    std::mutex mutex;
    std::ostream &out = std::clog;
    const Level min_level = default_min_level;
};

#endif //FLIP_JPEG_LOGGER_HPP
