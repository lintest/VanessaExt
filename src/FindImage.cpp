#ifdef _WINDOWS

#include "FindImage.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "json.hpp"

using namespace std;
using namespace cv;

std::string ImageFinder::find(const string &picture, const string &fragment, int match_method)
{
    Mat img = imread(picture, IMREAD_COLOR);
    Mat templ = imread(fragment, IMREAD_COLOR);
    if (img.empty() || templ.empty()) {
        return {};
    }

    Mat result;
    Mat img_display;
    img.copyTo(img_display);

    /// Create the result matrix
    int result_cols = img.cols - templ.cols + 1;
    int result_rows = img.rows - templ.rows + 1;
    result.create(result_rows, result_cols, CV_32FC1);

    /// Do the Matching and Normalize
    matchTemplate(img, templ, result, match_method);
    normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());

    /// Localizing the best match with minMaxLoc
    double minVal; double maxVal; Point minLoc; Point maxLoc;
    minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());
    /// For SQDIFF and SQDIFF_NORMED, the best matches are lower values. For all the other methods, the higher the better
    Point matchLoc = (match_method == TM_SQDIFF || match_method == TM_SQDIFF_NORMED) ? minLoc : maxLoc;

    nlohmann::json j;
    j["x"] = matchLoc.x;
    j["y"] = matchLoc.y;
    j["w"] = templ.cols;
    j["h"] = templ.rows;
    return j.dump();
}

#endif//_WINDOWS
