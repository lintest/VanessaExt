#ifdef USE_OPENCV

#include "ImageFinder.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "json.hpp"

using namespace std;
using namespace cv;

std::string BaseHelper::ImageFinder::find(VH picture, VH fragment, int method)
{
    Mat image = imdecode(std::vector<uchar>(picture.data(), picture.data() + picture.size()), IMREAD_COLOR);
    Mat templ = imdecode(std::vector<uchar>(fragment.data(), fragment.data() + fragment.size()), IMREAD_COLOR);
    if (image.empty() || templ.empty()) {
        return {};
    }

    int result_cols = image.cols - templ.cols + 1;
    int result_rows = image.rows - templ.rows + 1;
    Mat result(result_rows, result_cols, CV_32FC1);
    
    int match_method = (method % 3) * 2 + 1;
    matchTemplate(image, templ, result, match_method);
    double minVal, maxVal; Point minLoc; Point maxLoc;
    minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());
    /// For SQDIFF_NORMED, the best matches are lower values. For all the other methods, the higher the better
    double matchVal = (match_method == TM_SQDIFF_NORMED) ? 1 - minVal : maxVal;
    Point matchLoc = (match_method == TM_SQDIFF_NORMED) ? minLoc : maxLoc;

    nlohmann::json j;
    j["x"] = matchLoc.x + templ.cols / 2;
    j["y"] = matchLoc.y + templ.rows / 2;
    j["width"] = templ.cols;
    j["height"] = templ.rows;
    j["left"] = matchLoc.x;
    j["top"] = matchLoc.y;
    j["right"] = matchLoc.x + templ.cols;
    j["bottom"] = matchLoc.y + templ.rows;
    j["match"] = matchVal;
    return j.dump();
}

#endif //USE_OPENCV
