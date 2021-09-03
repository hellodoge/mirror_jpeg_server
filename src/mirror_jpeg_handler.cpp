
#include <algorithm>

#include "mirror_jpeg_handler.hpp"

using namespace handler;

auto MirrorJPEGHandler::handle(bytes_span input_jpeg) -> std::vector<uint8_t> {
    throw std::logic_error("mirror jpeg handle is not implemented");
}
