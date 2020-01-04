#ifndef _WINDOWS

#include <iostream>
#include <cstdlib>
#include <png.h>
#include <stdexcept>
#include "screenshot.h"

X11Screenshot::X11Screenshot() {};

X11Screenshot::X11Screenshot(XImage * image, int new_width, int new_height, std::string scale_type) {
    this->width = image->width;
    this->height = image->height;
    if ((new_width == 0 && new_height == 0) ||(new_width == this->width && new_height == this->height))
        this->image_data = this->process_original(image);
    else if (scale_type == "linear")
        this->image_data = this->process_scale_linear(image, new_width, new_height);
    else if (scale_type == "bilinear")
        this->image_data = this->process_scale_bilinear(image, new_width, new_height);
    else
        throw std::invalid_argument("Invalid initialisation parameters.");
};

std::vector<std::vector<unsigned char>> X11Screenshot::process_original(XImage * image) {
    std::vector<std::vector<unsigned char>> image_data;
    std::vector<unsigned char> image_data_row;
    unsigned long red_mask = image->red_mask;
    unsigned long green_mask = image->green_mask;
    unsigned long blue_mask = image->blue_mask;

    for (int y = 0; y < this->height; y++) {
        for (int x = 0; x < this->width; x++) {
            unsigned long pixel = XGetPixel(image, x, y);

            unsigned char blue = pixel & blue_mask;
            unsigned char green = (pixel & green_mask) >> 8;
            unsigned char red = (pixel & red_mask) >> 16;

            image_data_row.push_back(red);
            image_data_row.push_back(green);
            image_data_row.push_back(blue);
        }
        image_data.push_back(image_data_row);
        image_data_row.clear();
    }

    return image_data;
};

std::vector<std::vector<unsigned char>> X11Screenshot::process_scale_linear(XImage * image, int new_width, int new_height){
    std::vector<std::vector<unsigned char>> image_data;
    std::vector<unsigned char> image_data_row;
    unsigned long red_mask = image->red_mask;
    unsigned long green_mask = image->green_mask;
    unsigned long blue_mask = image->blue_mask;
    float x_ratio = ((float) (this->width))/new_width;
    float y_ratio = ((float) (this->height))/new_height;

    for (int new_y=0; new_y < new_height; new_y++) {
        for (int new_x=0; new_x < new_width; new_x++) {
            unsigned long pixel = XGetPixel(image, (int) new_x * x_ratio, (int) new_y * y_ratio);

            unsigned char blue = pixel & blue_mask;
            unsigned char green = (pixel & green_mask) >> 8;
            unsigned char red = (pixel & red_mask) >> 16;

            image_data_row.push_back(red);
            image_data_row.push_back(green);
            image_data_row.push_back(blue);
        }
        image_data.push_back(image_data_row);
        image_data_row.clear();
    }
    // update width and height after resize
    this->width = new_width;
    this->height = new_height;
    return image_data;
};

std::vector<std::vector<unsigned char>> X11Screenshot::process_scale_bilinear(XImage * image, int new_width, int new_height){
    std::vector<std::vector<unsigned char>> image_data;
    std::vector<unsigned char> image_data_row;
    float x_ratio = ((float) (this->width))/new_width;
    float y_ratio = ((float) (this->height))/new_height;
    unsigned long red_mask = image->red_mask;
    unsigned long green_mask = image->green_mask;
    unsigned long blue_mask = image->blue_mask;

    for (int new_y=0; new_y < new_height; new_y++) {
        for (int new_x=0; new_x < new_width; new_x++) {

            // x1, y1 is coordinates original pixel to take from original image
            // x2 is step to the right
            //y2 is step down
            int x_1 =  new_x * x_ratio;
            if (x_1 >= this->width) x_1 = this->width - 1; //becouse start pint is 0 and final is 1 less
            int y_1 =  new_y * y_ratio;
            if(y_1 >= this->height) y_1 = this->height - 1; //becouse start pint is 0 and final is 1 less
            int x_2 = x_1 + x_ratio;
            if (x_2 >= this->width) x_2 = this->width - 1;
            int y_2 = y_1 + y_ratio;
            if(y_2 >= this->height) y_2 = this->height - 1;
            float x_diff = (x_ratio * new_x) - x_1 ;
            float y_diff = (y_ratio * new_y) - y_1 ;

            // 4 point for bilineral function
            unsigned long q_1_1 = XGetPixel(image, x_1, y_1);
            unsigned long q_1_2 = XGetPixel(image, x_1, y_2);
            unsigned long q_2_1 = XGetPixel(image, x_2, y_1);
            unsigned long q_2_2 = XGetPixel(image, x_2, y_2);
            // blue element
            // Yb = Ab(1-w1)(1-h1) + Bb(w1)(1-h1) + Cb(h1)(1-w1) + Db(wh)
            float blue = (q_1_1 & blue_mask) * (1 - x_diff) * (1 - y_diff) + (q_1_2 & blue_mask) * (x_diff) * (1 - y_diff) +
                (q_2_1 & blue_mask) * (y_diff) * (1 - x_diff) + (q_2_2 & blue_mask) * (x_diff * y_diff);

            // green element
            // Yg = Ag(1-w1)(1-h1) + Bg(w1)(1-h1) + Cg(h1)(1-w1) + Dg(wh)
            float green = ((q_1_1 & green_mask) >> 8) * (1-x_diff) * (1-y_diff) + ((q_1_2 & green_mask) >> 8) * (x_diff) * (1 - y_diff) +
                ((q_2_1 & green_mask) >> 8) * (y_diff) * (1-x_diff) + ((q_2_2 & green_mask) >> 8) * (x_diff * y_diff);

            // red element
            // Yr = Ar(1-w1)(1-h1) + Br(w1)(1-h1) + Cr(h1)(1-w1) + Dr(wh)
            float red = ((q_1_1 & red_mask) >> 16) * (1 - x_diff) * (1 - y_diff) + ((q_1_2 & red_mask) >> 16) * (x_diff) * (1 - y_diff) +
                ((q_2_1 & red_mask) >> 16) * (y_diff) * (1 - x_diff) + ((q_2_2 & red_mask) >> 16) * (x_diff * y_diff);

            image_data_row.push_back((int) red);
            image_data_row.push_back((int) green);
            image_data_row.push_back((int )blue);
        }
        image_data.push_back(image_data_row);
        image_data_row.clear();
    }
    // update width and height after resize
    this->width = new_width;
    this->height = new_height;
    return image_data;
};

static void PngWriteCallback(png_structp  png_ptr, png_bytep data, png_size_t length) {
    std::vector<char> *p = (std::vector<char>*)png_get_io_ptr(png_ptr);
    p->insert(p->end(), data, data + length);
}

bool X11Screenshot::save_to_png(std::vector<char> &out) {
    FILE *fp = NULL;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_bytep row;

    png_ptr = png_create_write_struct(
        PNG_LIBPNG_VER_STRING,
        NULL,
        NULL,
        NULL
    );
    if (!png_ptr) {
        std::cout << "Failed to create PNG write structure" << std::endl;
        fclose(fp);
        return false;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        std::cout << "Failed to create PNG info structure" << std::endl;
        fclose(fp);
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        return false;
    }

    // Setup png lib error handling
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        fclose(fp);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    }

    png_init_io(png_ptr, fp);

    png_set_write_fn(png_ptr, &out, PngWriteCallback, NULL);

    // Output is 8bit depth, RGBA format.
    png_set_IHDR(
        png_ptr,
        info_ptr,
        this->width,
        this->height,
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    // set compression level
    png_set_compression_level(png_ptr, 9);
    // write info header
    png_write_info(png_ptr, info_ptr);
    for(std::vector<std::vector<unsigned char>>::size_type i = 0; i != this->image_data.size(); i++) {
        // build character row from array of characters
        row = (png_bytep) reinterpret_cast<unsigned char*>(this->image_data[i].data());
        // write byterow
        png_write_row(png_ptr, row);
    }
    // end writing file
    png_write_end(png_ptr, NULL);
    // cleanup
    if (fp != NULL) fclose(fp);
    if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

    return true;
};

#endif //_WINDOWS
