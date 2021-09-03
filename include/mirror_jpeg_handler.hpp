#ifndef FLIP_JPEG_MIRROR_JPEG_HANDLER_HPP
#define FLIP_JPEG_MIRROR_JPEG_HANDLER_HPP

#include "handler_interface.hpp"

namespace handler {
    class MirrorJPEGHandler final : public IHandler {
        auto handle(bytes_span input_jpeg) -> std::vector<uint8_t> override;
    };
}

#endif //FLIP_JPEG_MIRROR_JPEG_HANDLER_HPP
