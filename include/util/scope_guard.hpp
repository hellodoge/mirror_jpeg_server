#ifndef MIRROR_JPEG_SERVER_SCOPE_GUARD_HPP
#define MIRROR_JPEG_SERVER_SCOPE_GUARD_HPP

// oversimplified version of scope_guard to avoid using external libraries
// description: when destructed, executes the callback passed to the constructor
// motivation: more idiomatic way to free resources while working with C libraries

class scope_guard {
    using callback_type = std::function<void()>;

public:
    explicit scope_guard(callback_type callback)
            : callback {std::move(callback)} {}

    scope_guard(scope_guard&) = delete;
    scope_guard(scope_guard&&) = delete;

    ~scope_guard() {
        callback();
    }

private:
    callback_type callback;
};

#endif //MIRROR_JPEG_SERVER_SCOPE_GUARD_HPP
