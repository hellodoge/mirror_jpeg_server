#ifndef FLIP_JPEG_SIZE_LITERALS_HPP
#define FLIP_JPEG_SIZE_LITERALS_HPP

namespace size_literals {

    constexpr auto operator ""_KiB(unsigned long long bytes) {
        return 1024 * bytes;
    }

    constexpr auto operator ""_MiB(unsigned long long bytes) {
        return 1024 * 1024 * bytes;
    }

}

#endif //FLIP_JPEG_SIZE_LITERALS_HPP
