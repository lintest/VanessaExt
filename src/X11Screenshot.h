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
        */
        X11Screenshot(XImage * image);
        /**
        Public method to save image to png file
        @param path a constant character pointer - path where to create png image
        @return a boolean - true if succesfuly created image file, false otherwise
        */
        bool save_to_png(std::vector<char> &out);
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
        std::vector<std::vector<unsigned char>> process(XImage * image);
};

#endif
