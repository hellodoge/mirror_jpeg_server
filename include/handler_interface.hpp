#ifndef FLIP_JPEG_HANDLER_INTERFACE_HPP
#define FLIP_JPEG_HANDLER_INTERFACE_HPP

#include <vector>
#include <boost/beast.hpp>

namespace handler {

    // there is std::span in cpp20 standard, but it might be not supported yet
    using bytes_span = boost::beast::span<uint8_t>;

    class IHandler {
    public:
        virtual ~IHandler() = default;
        virtual auto handle(bytes_span) -> std::vector<uint8_t> = 0;
    };
}

#endif //FLIP_JPEG_HANDLER_INTERFACE_HPP
