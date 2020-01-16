// Clip Library
// Copyright (c) 2015-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef CLIP_H_INCLUDED
#define CLIP_H_INCLUDED

#include <cassert>
#include <memory>
#include <string>

namespace clip {

  // ======================================================================
  // Low-level API to lock the clipboard/pasteboard and modify it
  // ======================================================================

  // Clipboard format identifier.
  typedef size_t format;

  class image;
  struct image_spec;

  class lock {
  public:
    // You can give your current HWND as the "native_window_handle."
    // Windows clipboard functions use this handle to open/close
    // (lock/unlock) the clipboard. From the MSDN documentation we
    // need this handler so SetClipboardData() doesn't fail after a
    // EmptyClipboard() call. Anyway it looks to work just fine if we
    // call OpenClipboard() with a null HWND.
    lock(void* native_window_handle = nullptr);
    ~lock();

    // Returns true if we've locked the clipboard successfully in
    // lock() constructor.
    bool locked() const;

    // Clears the clipboard content. If you don't clear the content,
    // previous clipboard content (in unknown formats) could persist
    // after the unlock.
    bool clear();

    // Returns true if the clipboard can be converted to the given
    // format.
    bool is_convertible(format f) const;
    bool set_data(format f, const char* buf, size_t len);
    bool get_data(format f, char* buf, size_t len) const;
    size_t get_data_length(format f) const;

    // For images
    bool set_image(const image& image);
    bool get_image(image& image) const;
    bool get_image_spec(image_spec& spec) const;

  private:
    class impl;
    std::unique_ptr<impl> p;
  };

  format register_format(const std::string& name);

  // This format is when the clipboard has no content.
  format empty_format();

  // When the clipboard has UTF8 text.
  format text_format();

  // When the clipboard has an image.
  format image_format();

  // Returns true if the clipboard has content of the given type.
  bool has(format f);

  // Clears the clipboard content.
  bool clear();

  // ======================================================================
  // Error handling
  // ======================================================================

  enum class ErrorCode {
    CannotLock,
    ImageNotSupported,
  };

  typedef void (*error_handler)(ErrorCode code);

  void set_error_handler(error_handler f);
  error_handler get_error_handler();

  // ======================================================================
  // Text
  // ======================================================================

  // High-level API to put/get UTF8 text in/from the clipboard. These
  // functions returns false in case of error.
  bool set_text(const std::string& value);
  bool get_text(std::string& value);

  // ======================================================================
  // Image
  // ======================================================================

  struct image_spec {
    unsigned long width = 0;
    unsigned long height = 0;
    unsigned long bits_per_pixel = 0;
    unsigned long bytes_per_row = 0;
    unsigned long red_mask = 0;
    unsigned long green_mask = 0;
    unsigned long blue_mask = 0;
    unsigned long alpha_mask = 0;
    unsigned long red_shift = 0;
    unsigned long green_shift = 0;
    unsigned long blue_shift = 0;
    unsigned long alpha_shift = 0;
  };

  // The image data must contain straight RGB values
  // (non-premultiplied by alpha). The image retrieved from the
  // clipboard will be non-premultiplied too. Basically you will be
  // always dealing with straight alpha images.
  //
  // Details: Windows expects premultiplied images on its clipboard
  // content, so the library code make the proper conversion
  // automatically. macOS handles straight alpha directly, so there is
  // no conversion at all. Linux/X11 images are transferred in
  // image/png format which are specified in straight alpha.
  class image {
  public:
    image();
    image(const image_spec& spec);
    image(const void* data, const image_spec& spec);
    image(const image& image);
    image(image&& image);
    ~image();

    image& operator=(const image& image);
    image& operator=(image&& image);

    char* data() const { return m_data; }
    const image_spec& spec() const { return m_spec; }

    bool is_valid() const { return m_data != nullptr; }
    void reset();

  private:
    void copy_image(const image& image);
    void move_image(image&& image);

    bool m_own_data;
    char* m_data;
    image_spec m_spec;
  };

  // High-level API to set/get an image in/from the clipboard. These
  // functions returns false in case of error.
  bool set_image(const image& img);
  bool get_image(image& img);
  bool get_image_spec(image_spec& spec);

  // ======================================================================
  // Platform-specific
  // ======================================================================

  // Only for X11: Sets the time (in milliseconds) that we must wait
  // for the selection/clipboard owner to receive the content. This
  // value is 1000 (one second) by default.
  void set_x11_wait_timeout(int msecs);
  int get_x11_wait_timeout();

} // namespace clip

#endif // CLIP_H_INCLUDED
// Clip Library
// Copyright (c) 2015-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef CLIP_LOCK_IMPL_H_INCLUDED
#define CLIP_LOCK_IMPL_H_INCLUDED

namespace clip {

class lock::impl {
public:
  impl(void* native_window_handle);
  ~impl();

  bool locked() const { return m_locked; }
  bool clear();
  bool is_convertible(format f) const;
  bool set_data(format f, const char* buf, size_t len);
  bool get_data(format f, char* buf, size_t len) const;
  size_t get_data_length(format f) const;
  bool set_image(const image& image);
  bool get_image(image& image) const;
  bool get_image_spec(image_spec& spec) const;

private:
  bool m_locked;
};

} // namespace clip

#endif

//////////////////////////////////////////////////////////////////////
// Clip Library
// Copyright (c) 2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

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
  png_set_packing(png);

  png_bytep row =
    (png_bytep)png_malloc(png, png_get_rowbytes(png, info));

  for (png_uint_32 y=0; y<spec.height; ++y) {
    const uint32_t* src =
      (const uint32_t*)(((const uint8_t*)image.data())
                        + y*spec.bytes_per_row);
    uint8_t* dst = row;
    unsigned int x, c;

    for (x=0; x<spec.width; x++) {
      c = *(src++);
      *(dst++) = (c & spec.red_mask  ) >> spec.red_shift;
      *(dst++) = (c & spec.green_mask) >> spec.green_shift;
      *(dst++) = (c & spec.blue_mask ) >> spec.blue_shift;
      if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        *(dst++) = (c & spec.alpha_mask) >> spec.alpha_shift;
    }

    png_write_rows(png, &row, 1);
  }

  png_free(png, row);
  png_write_end(png, info);
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

  image_spec spec;
  spec.width = width;
  spec.height = height;
  spec.bits_per_pixel = 32;
  spec.bytes_per_row = png_get_rowbytes(png, info);

  spec.red_mask    = 0x000000ff;
  spec.green_mask  = 0x0000ff00;
  spec.blue_mask   = 0x00ff0000;
  spec.red_shift   = 0;
  spec.green_shift = 8;
  spec.blue_shift  = 16;

  // TODO indexed images with alpha
  if (color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    spec.alpha_mask = 0xff000000;
    spec.alpha_shift = 24;
  }
  else {
    spec.alpha_mask = 0;
    spec.alpha_shift = 0;
  }

  if (output_spec)
    *output_spec = spec;

  if (output_image &&
      width > 0 &&
      height > 0) {
    image img(spec);

    // We want RGB 24-bit or RGBA 32-bit as a result
    png_set_strip_16(png); // Down to 8-bit (TODO we might support 16-bit values)
    png_set_packing(png);  // Use one byte if color depth < 8-bit
    png_set_expand_gray_1_2_4_to_8(png);
    png_set_palette_to_rgb(png);
    png_set_gray_to_rgb(png);
    png_set_tRNS_to_alpha(png);

    int number_passes = png_set_interlace_handling(png);
    png_read_update_info(png, info);

    png_bytepp rows = (png_bytepp)png_malloc(png, sizeof(png_bytep)*height);
    png_uint_32 y;
    for (y=0; y<height; ++y)
      rows[y] = (png_bytep)png_malloc(png, spec.bytes_per_row);

    for (int pass=0; pass<number_passes; ++pass)
      for (y=0; y<height; ++y)
        png_read_rows(png, rows+y, nullptr, 1);

    for (y=0; y<height; ++y) {
      const uint8_t* src = rows[y];
      uint32_t* dst = (uint32_t*)(img.data() + y*spec.bytes_per_row);
      unsigned int x, r, g, b, a = 255;

      for (x=0; x<width; x++) {
        r = *(src++);
        g = *(src++);
        b = *(src++);
        if (spec.alpha_mask)
          a = *(src++);
        *(dst++) =
          (r << spec.red_shift) |
          (g << spec.green_shift) |
          (b << spec.blue_shift) |
          (a << spec.alpha_shift);
      }
      png_free(png, rows[y]);
    }
    png_free(png, rows);

    std::swap(*output_image, img);
  }

  png_destroy_read_struct(&png, &info, nullptr);
  return true;
}

} // namespace x11
} // namespace clip
