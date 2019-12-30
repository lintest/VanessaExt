#ifndef SCREENSHOT_H_INCLUDED_
#define SCREENSHOT_H_INCLUDED_
#include <vector>
#include <string>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/X.h>

/** @class X11Screenshot
    @brief A class for screenshots

    Process x11 image data and save to file.
*/
class X11Screenshot {
    public:
        X11Screenshot();
        /**
        A constructror. Construct a X11Screenshot object.
        @param image a XImage pointer - image to process
        @param new_width integer - change initial image width to new value, default is 0 - no resize
        @param new_height integer - change initial image height to new value, default is 0 - no resize
        @param scale_type string - type of interpolation for scaling, available "linear" and "bilinear", default is "linear"
        */
        X11Screenshot(XImage * image, int new_width=0, int new_height=0, std::string scale_type="linear");
        /**
        Public method to save image to png file
        @param path a constant character pointer - path where to create png image
        @return a boolean - true if succesfuly created image file, false otherwise
        */
        bool save_to_png(const char * path);
        /**
        Public method to save image to jpeg file
        @param path a constant character pointer - path where to create jpeg image
        @param quality integer - level of jpeg compression, lower the value higher the compression
        @return a boolean - true if succesfuly created image file, false otherwise
        */
        bool save_to_jpeg(const char * path, int quality);
        /**
        Public method to aquire image current width
        @return a integer - image width in pexels
        */
        int get_width(void);
        /**
        Public method to aquire image current height
        @return a integer - image height in pexels
        */
        int get_height(void);

    private:
        /**
        A private integer variable width
        Stores image current max width in pixels
        */
        int width = 0;
        /**
        A private integer variable height
        Stores image max height in pixels
        */
        int height = 0;
        /**
        A private vector of unsigned characters vectors image_data
        Contains rgb values of an image
        */
        std::vector<std::vector<unsigned char>> image_data = std::vector<std::vector<unsigned char>>();
        /**
        A private method to process XImage pixels to rgb bytes as is
        @param image an XImage pointer - image to process to rgb char vector
        @return vector of unsigned characters vectors - representing rgb values line by line
        */
        std::vector<std::vector<unsigned char>> process_original(XImage * image);
        /**
        A private method to process XImage pixels to rgb bytes with
        scale procesed by a lineral function (f(x) =  ax + b)
        @param image an XImage pointer - image to process to rgb char vector
        @param new_width a integer - scale to this max width
        @param new_height a integer - scale to this max height
        @return vector of unsigned characters vectors - representing rgb values line by line
        */
        std::vector<std::vector<unsigned char>> process_scale_linear(XImage * image, int new_width=0, int new_height=0);
        /**
        A private method to process XImage pixels to rgb bytes with
        scale procesed by a bilineral function (f(x, y) = a0 + a1x + a2y + a3xy)
        @param image an XImage pointer - image to process to rgb char vector
        @param new_width a integer - scale to this max width
        @param new_height a integer - scale to this max height
        @return vector of unsigned characters vectors - representing rgb values line by line
        */
        std::vector<std::vector<unsigned char>> process_scale_bilinear(XImage * image, int new_width=0, int new_height=0);
};

#endif
