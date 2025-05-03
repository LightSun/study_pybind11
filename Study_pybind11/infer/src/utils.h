#ifndef UTILS_H
#define UTILS_H

#include "ocr_context.h"

namespace h7 {

class Utils
{
public:
    static void getRect(const cv::Mat &srcimage,
                          std::array<float, 4>& box,cv::Rect& rt);
    //box: [xmin, ymin, xmax, ymax]
    static cv::Mat getRotateCropImage(const cv::Mat &srcimage,
                                      std::array<float, 4>& box);
    static bool hasChinese(const char* chs);

    static bool isFileExists(const std::string &path);
    static void createDir(const std::string &path);
    static void writeFile(CString file, CString data);

    static int get_box_height(std::array<int, 8>& box);
    static void find_two_largest(int* array, int array_length, int* out_idxes);
//    static cv::Mat med_VisualizeResult(
//        const cv::Mat& img,
//        const std::vector<IcuRegItem>& results,
//        const std::vector<std::string>& lables,
//        const bool is_rbox);

};

}

#endif // UTILS_H
