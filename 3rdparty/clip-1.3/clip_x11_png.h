//////////////////////////////////////////////////////////////////////
// Clip Library
// Copyright (c) 2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "clip.h"

#include <algorithm>
#include <vector>

#include "png.h"

namespace clip {
namespace x11 {

//////////////////////////////////////////////////////////////////////
// Functions to convert clip::image into png data to store it in the
// clipboard.

void write_data_fn(png_structp png, png_bytep buf, png_size_t len) {
  std::vector<uint8_t>& output = *(std::vector<uint8_t>*)png_get_io_ptr(png);
  const size_t i = output.size();
  output.resize(i+len);
  std::copy(buf, buf+len, output.begin()+i);
}

bool write_png(const image& image,
               std::vector<uint8_t>& output) {
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                              nullptr, nullptr, nullptr);
    if (!png)
        return false;

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, nullptr);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        return false;
    }

    png_set_write_fn(png,
                     (png_voidp)&output,
                     write_data_fn,
                     nullptr);    // No need for a flush function

    const image_spec& spec = image.spec();
    int color_type = (spec.alpha_mask ?
                      PNG_COLOR_TYPE_RGB_ALPHA:
                      PNG_COLOR_TYPE_RGB);

    png_set_IHDR(png, info,
                 spec.width, spec.height, 8, color_type,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png, info);

    png_bytepp rows = (png_bytepp)png_malloc(png, sizeof(png_bytep) * spec.height);

    for (png_uint_32 y = 0; y < spec.height; ++y)
        rows[y] = (png_bytep) (image.data() + y * png_get_rowbytes(png, info));
    png_write_image(png, rows);
    png_write_end(png, info);

    png_free(png, rows);
    png_destroy_write_struct(&png, &info);
    return true;
}

//////////////////////////////////////////////////////////////////////
// Functions to convert png data stored in the clipboard to a
// clip::image.

struct read_png_io {
  const uint8_t* buf;
  size_t len;
  size_t pos;
};

void read_data_fn(png_structp png, png_bytep buf, png_size_t len) {
  read_png_io& io = *(read_png_io*)png_get_io_ptr(png);
  if (io.pos < io.len) {
    size_t n = std::min(len, io.len-io.pos);
    if (n > 0) {
      std::copy(io.buf+io.pos,
                io.buf+io.pos+n,
                buf);
      io.pos += n;
    }
  }
}

bool read_png(const uint8_t* buf,
              const size_t len,
              image* output_image,
              image_spec* output_spec) {
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                             nullptr, nullptr, nullptr);
    if (!png)
        return false;

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        return false;
    }

    read_png_io io = { buf, len, 0 };
    png_set_read_fn(png, (png_voidp)&io, read_data_fn);

    png_read_info(png, info);

    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    png_get_IHDR(png, info, &width, &height,
                 &bit_depth, &color_type,
                 &interlace_type,
                 nullptr, nullptr);

    int number_of_passes = png_set_interlace_handling(png);
    png_read_update_info(png, info);

    png_bytepp rows = (png_bytepp)png_malloc(png, sizeof(png_bytep)*height);
    png_uint_32 y;

    image_spec spec;
    spec.width = width;
    spec.height = height;
    spec.bits_per_pixel = 24;
    spec.bytes_per_row = png_get_rowbytes(png, info);

    spec.red_mask    = 0x000000ff;
    spec.green_mask  = 0x0000ff00;
    spec.blue_mask   = 0x00ff0000;
    spec.red_shift   = 0;
    spec.green_shift = 8;
    spec.blue_shift  = 16;

    // TODO indexed images with alpha
    if (color_type == PNG_COLOR_TYPE_RGB_ALPHA || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        spec.alpha_mask = 0xff000000;
        spec.alpha_shift = 24;
    } else {
        spec.alpha_mask = 0;
        spec.alpha_shift = 0;
    }

    if (output_spec)
        *output_spec = spec;

    if (output_image &&
        width > 0 &&
        height > 0) {
        image img(spec);

        for (y = 0; y < height; ++y)
            rows[y] = (png_bytep) (img.data() + y * png_get_rowbytes(png, info));

        png_read_image(png, rows);
        png_free(png, rows);
        std::swap(*output_image, img);
    }
    png_destroy_read_struct(&png, &info, nullptr);
    return true;
}

} // namespace x11
} // namespace clip
