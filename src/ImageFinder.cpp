#include "ImageFinder.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "json.hpp"

using namespace std;
using namespace cv;

std::string BaseHelper::ImageFinder::find(VH picture, VH fragment, int match_method)
{
    std::vector<uchar> v_picture(picture.data(), picture.data() + picture.size());
    std::vector<uchar> v_fragment(fragment.data(), fragment.data() + fragment.size());
    Mat img = imdecode(v_picture, IMREAD_COLOR);
    Mat templ = imdecode(v_fragment, IMREAD_COLOR);

    if (img.empty() || templ.empty()) {
        return {};
    }

    /// Create the result matrix
    Mat result;
    int result_cols = img.cols - templ.cols + 1;
    int result_rows = img.rows - templ.rows + 1;
    result.create(result_rows, result_cols, CV_32FC1);

    /// Do the Matching and Normalize
    matchTemplate(img, templ, result, match_method);
    normalize(result, result, 0, 1, NORM_MINMAX, -1, Mat());

    /// Localizing the best match
    Point minLoc; Point maxLoc;
    minMaxLoc(result, nullptr, nullptr, &minLoc, &maxLoc, Mat());
    /// For SQDIFF and SQDIFF_NORMED, the best matches are lower values. For all the other methods, the higher the better
    Point matchLoc = (match_method == TM_SQDIFF || match_method == TM_SQDIFF_NORMED) ? minLoc : maxLoc;

    nlohmann::json j;
    j["x"] = matchLoc.x + templ.cols / 2;
    j["y"] = matchLoc.y + templ.rows / 2;
    j["width"] = templ.cols;
    j["height"] = templ.rows;
    j["left"] = matchLoc.x;
    j["top"] = matchLoc.y;
    j["right"] = matchLoc.x + templ.cols;
    j["bottom"] = matchLoc.y + templ.rows;
    return j.dump();
}
