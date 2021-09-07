#include <cstring>
#include <cstdio> // https://github.com/libjpeg-turbo/libjpeg-turbo/issues/17
using FILE = std::FILE;
using size_t = std::size_t;
extern "C" {
    #include <jpeglib.h>
}

#include "mirror_jpeg_handler.hpp"
#include "util/size_literals.hpp"
#include "util/scope_guard.hpp"

using namespace handler;
using namespace size_literals;

struct Jpeg {
    std::vector<uint8_t> buffer;
    unsigned width;
    unsigned height;
    int pixel_size;
    J_COLOR_SPACE colorspace;
};

Jpeg decompress_jpeg(bytes_span compressed);
std::vector<uint8_t> compress_jpeg(Jpeg &image);
static void mirror_image(Jpeg &image);

auto MirrorJPEGHandler::handle(bytes_span input_jpeg) -> std::vector<uint8_t> {
    Jpeg image = decompress_jpeg(input_jpeg);
    mirror_image(image);
    return compress_jpeg(image);
}

static void mirror_image(Jpeg &image) {
    uint8_t *image_data = image.buffer.data();
    for (unsigned current_row = 0; current_row < image.height; current_row++) {
        // warning: VLA may not be supported by some compilers
        using pixel = uint8_t[image.pixel_size];
        pixel *first = ((pixel *)image_data) + image.width * current_row;
        pixel *last = first + image.width - 1;
        // std::reverse cannot be used here, because pixel is VLA
        while (first < last) {
            // we cannot just assign arrays
            for (size_t i = 0; i < sizeof(pixel); i++) {
                uint8_t tmp;
                tmp = (*first)[i];
                (*first)[i] = (*last)[i];
                (*last)[i] = tmp;
            }
            first++;
            last--;
        }
    }
}

static std::string get_error_message(j_common_ptr err_info);

Jpeg decompress_jpeg(bytes_span compressed) {

    jpeg_decompress_struct info {};
    scope_guard info_destructor([&](){
        jpeg_destroy_decompress(&info);
    });
    jpeg_error_mgr err {};

    info.err = jpeg_std_error(&err);
    err.error_exit = [](j_common_ptr err_info) {
        throw std::runtime_error(get_error_message(err_info));
    };

    jpeg_create_decompress(&info);
    jpeg_mem_src(&info, compressed.data(), compressed.size());

    if (jpeg_read_header(&info, true /* error if EOF encountered */) != JPEG_HEADER_OK)
        throw handling_error("not valid jpeg format");

    jpeg_start_decompress(&info);

    const unsigned width = info.output_width;
    const unsigned height = info.output_height;
    const int pixel_size = info.output_components;

    std::vector<uint8_t> buffer;
    buffer.reserve(height * width * pixel_size);

    while (info.output_scanline < height) {
        uint8_t *cursor = buffer.data() + ((width * pixel_size) * info.output_scanline);
        jpeg_read_scanlines(&info, &cursor, 1 /* scan one line per call */);
    }

    jpeg_finish_decompress(&info);
    // resources will be freed by the scope guard
    return {
        .buffer = std::move(buffer),
        .width = width,
        .height = height,
        .pixel_size = pixel_size,
        .colorspace = info.out_color_space
    };
}

std::vector<uint8_t> compress_jpeg(Jpeg &image) {

    jpeg_compress_struct info {};
    scope_guard info_destructor([&](){
        jpeg_destroy_compress(&info);
    });
    jpeg_error_mgr err {};

    info.err = jpeg_std_error(&err);
    err.error_exit = [](j_common_ptr err_info) {
        throw std::runtime_error(get_error_message(err_info));
    };
    jpeg_create_compress(&info);

    info.image_width = image.width;
    info.image_height = image.height;
    info.in_color_space = image.colorspace;
    info.input_components = image.pixel_size;
    jpeg_set_defaults(&info);

    std::vector<uint8_t> buffer;
    constexpr auto block_size = 32_KiB;

    // 1. library does it the same way
    // https://github.com/LuaDist/libjpeg/blob/6c0fcb8ddee365e7abc4d332662b06900612e923/jdatadst.c#L235
    // 2. pointer used instead of reference to make it standard-layout type
    // https://stackoverflow.com/questions/53850100/warning-offset-of-on-non-standard-layout-type-derivedclass
    struct Dest {
        jpeg_destination_mgr mgr;
        std::vector<uint8_t> *buffer;
    };
    static_assert(offsetof(Dest, mgr) == 0);

    auto init_buffer = [](j_compress_ptr info){
        auto &buffer = *((Dest*)info->dest)->buffer;
        buffer.resize(block_size);
        info->dest->next_output_byte = buffer.data();
        info->dest->free_in_buffer = buffer.size();
    };

    auto grow_buffer = [](j_compress_ptr info){
        auto &buffer = *((Dest*)info->dest)->buffer;
        // only called when free_in_buffer reaches zero
        // https://github.com/libjpeg-turbo/libjpeg-turbo/blob/173900b1cabb027495ae530c71250bcedc9925d5/libjpeg.txt#L1601
        auto old_size = buffer.size();
        buffer.resize(buffer.size() + block_size);
        info->dest->next_output_byte = &buffer.at(old_size);
        info->dest->free_in_buffer = buffer.size() - old_size;
        return (int) true;
    };

    auto trim_buffer = [](j_compress_ptr info){
        auto &buffer = *((Dest*)info->dest)->buffer;
        buffer.resize(buffer.size() - info->dest->free_in_buffer);
        info->dest->next_output_byte = &*buffer.end();
        info->dest->free_in_buffer = 0;
    };

    Dest dest {{}, &buffer};
    info.dest = &dest.mgr;

    dest.mgr.init_destination = init_buffer;
    dest.mgr.empty_output_buffer = grow_buffer;
    dest.mgr.term_destination = trim_buffer;

    jpeg_start_compress(&info, true /* write complete JPEG */);
    while (info.next_scanline < info.image_height) {
        auto cursor = &image.buffer[image.width * image.pixel_size * info.next_scanline];
        jpeg_write_scanlines(&info, &cursor, 1 /* write one line per call */);
    }

    jpeg_finish_compress(&info);
    // resources will be freed by the scope guard
    return buffer;
}

static std::string get_error_message(j_common_ptr err_info) {
    std::string error_message {};
    error_message.resize(JMSG_LENGTH_MAX);
    err_info->err->format_message(err_info, error_message.data());
    return error_message;
}
