#include <fstream>
#include "utils.h"
#include "opencv2/imgproc.hpp"
#include "string_utils.hpp"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#ifdef WIN32
#include <windows.h>                 //Windows API   FindFirstFile
#include <shlwapi.h>
#endif


namespace h7 {

//box: [xmin, ymin, xmax, ymax]
void Utils::getRect(const cv::Mat &srcimage,
                                  std::array<float, 4>& box,cv::Rect& rt){
    int xmin = box[0] - 1.0f;
    int xmax = box[2] + 1.0f;
    int ymin = box[1] - 1.0f;
    int ymax = box[3] + 1.0f;
    xmin = HMAX(0, xmin);
    xmax = HMIN(xmax, srcimage.cols);
    ymin = HMAX(0, ymin);
    ymax = HMIN(ymax, srcimage.rows);
    //
   // cv::Rect rt;
    rt.x = xmin;
    rt.y = ymin;
    rt.width = xmax - xmin;
    rt.height = ymax - ymin;
}

//box: [xmin, ymin, xmax, ymax]
cv::Mat Utils::getRotateCropImage(const cv::Mat &srcimage,
                                  std::array<float, 4>& box){
    //
    cv::Rect rt;
    getRect(srcimage, box, rt);
    return cv::Mat(srcimage, rt);
}

bool Utils::hasChinese(const char* chs){
    char *str = (char*)chs;
    char c;
    while(1)
    {
        c = *str++;
        if (c == 0) break;  //is the end
        if (c & 0x80){      //high is 1 and next char high is 1. means has chinese
             if (*str & 0x80) return 1;
        }
    }
    return 0;
}

bool Utils::isFileExists(const std::string &path) {
#ifdef _WIN32
    WIN32_FIND_DATA wfd;
    HANDLE hFind = FindFirstFile(path.data(), &wfd);
    if ( INVALID_HANDLE_VALUE != hFind )  return true;
    else                                  return false;
#else
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
#endif // !_WIN32
}

void Utils::createDir(const std::string &path) {
#ifdef _WIN32
  _mkdir(path.c_str());
#else
  mkdir(path.c_str(), 0777);
#endif // !_WIN32
}

void Utils::writeFile(CString file, CString data){
    FILE* _f = fopen(file.data(), "w");
    fwrite(data.data(), 1, data.length(), _f);
    fflush(_f);
    fclose(_f);
}

int Utils::get_box_height(std::array<int, 8>& box){
    //int x_collect[4] = {box[0], box[2], box[4], box[6]};
    int y_collect[4] = {box[1], box[3], box[5], box[7]};
    //use compensate to help reduce some case. like 131/61 -> 131/6
    //int left = int(*std::min_element(x_collect, x_collect + 4));
    //int right = int(*std::max_element(x_collect, x_collect + 4));
    int top = int(*std::min_element(y_collect, y_collect + 4));
    int bottom = int(*std::max_element(y_collect, y_collect + 4));
    return bottom - top;
}

void Utils::find_two_largest(int* array, int array_length, int* out_idxes){
    int m1, m2;

    if(array[0] > array[1]){
        m1 = array[0];
        m2 = array[1];
        out_idxes[0] = 0;
        out_idxes[1] = 1;
    }else{
        m1 = array[1];
        m2 = array[0];

        out_idxes[0] = 1;
        out_idxes[1] = 0;
    }

    for(int i = 2; i < array_length; i++){
        if(array[i] > m1){
            m2 = m1;
            m1 = array[i];
            out_idxes[1] = out_idxes[0];
            out_idxes[0] = i;
        }else if(array[i] > m2){
            m2 = array[i];
            out_idxes[1] = i;
        }

    }
}
/*
cv::Mat Utils::med_VisualizeResult(
    const cv::Mat& img,
    const std::vector<IcuRegItem>& results,
    const std::vector<std::string>& lables,
    const bool is_rbox) {
    std::map<int, std::vector<int>> _color_map = {
        {2, {255, 255, 255}},
        {1, {255, 0, 255}},
        {3, {0, 255, 255}},
        {0, {0, 255, 0}},
        {4, {139,61 ,72}},
        {5, {0 ,0 ,255}},
        {6, {0 ,165 ,255}},
        {7, {255 ,191 ,0}},
        {8, {205,181,205}},

        {9, {205,100,205}},
    };

  cv::Mat vis_img = img.clone();
  int img_h = vis_img.rows;
  int img_w = vis_img.cols;
  for (int i = 0; i < results.size(); ++i) {
    // Configure color and text size
    std::ostringstream oss;
    oss << std::setiosflags(std::ios::fixed) << std::setprecision(3);
    oss << lables[results[i].cls] << " ";
    oss << results[i].clsScore;
    oss << " : " << results[i].dText << " " << results[i].score;
    //
    std::string text = oss.str();
    int c1 = _color_map[results[i].cls][0];
    int c2 = _color_map[results[i].cls][1];
    int c3 = _color_map[results[i].cls][2];
    cv::Scalar roi_color = cv::Scalar(c1, c2, c3); //bgr
    int font_face = cv::FONT_HERSHEY_COMPLEX_SMALL;
    double font_scale = 1.2f;
    float thickness = 0.8f;
    cv::Size text_size =
        cv::getTextSize(text, font_face, font_scale, thickness, nullptr);
    cv::Point origin;

    if (is_rbox) {
      // Draw object, text, and background
//      for (int k = 0; k < 4; k++) {
//        cv::Point pt1 = cv::Point(results[i].dORet->rect[(k * 2) % 8],
//                                  results[i].dORet->rect[(k * 2 + 1) % 8]);
//        cv::Point pt2 = cv::Point(results[i].dORet->rect[(k * 2 + 2) % 8],
//                                  results[i].dORet->rect[(k * 2 + 3) % 8]);
//        cv::line(vis_img, pt1, pt2, roi_color, 2);
//      }
      std::cerr << "is_rbox not support!" << std::endl;
    } else {
      cv::Rect roi;
      //roi = cv::Rect(results[i].dORet->rect[0], results[i].dORet->rect[1], w, h);
      getRect(img, *results[i].box, roi);
      // Draw roi object, text, and background
      cv::rectangle(vis_img, roi, roi_color, 2);

      // Draw mask
      std::vector<uint8_t> mask_v;
      if(results[i].mask != nullptr){
          mask_v = results[i].mask->data;
      }
      if (mask_v.size() > 0) {
          //pre is 32S
        cv::Mat mask = cv::Mat(img_h, img_w, CV_8U);
        std::memcpy(mask.data, mask_v.data(), mask_v.size() * sizeof(uint8_t));

        cv::Mat colored_img = vis_img.clone();

        std::vector<cv::Mat> contours;
        cv::Mat hierarchy;
        //mask.convertTo(mask, CV_8U);
        cv::findContours(
            mask, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);
        cv::drawContours(colored_img,
                         contours,
                         -1,
                         roi_color,
                         -1,
                         cv::LINE_8,
                         hierarchy,
                         100);

        //cv::Mat debug_roi = vis_img;
        colored_img = 0.4 * colored_img + 0.6 * vis_img;
        colored_img.copyTo(vis_img, mask);
      }
    }

    origin.x = (*results[i].box)[0];
    origin.y = (*results[i].box)[1] - 2;

    // Configure text background
    cv::Rect text_back = cv::Rect((*results[i].box)[0],
                                  (*results[i].box)[1] - text_size.height - 4,
                                  text_size.width,
                                  text_size.height + 4);
    // Draw text, and background
    cv::rectangle(vis_img, text_back, roi_color, -1);
    cv::putText(vis_img,
                text,
                origin,
                font_face,
                font_scale,
               // cv::Scalar(255, 255, 255),
                cv::Scalar(0, 0, 0),
                thickness);
  }
  return vis_img;
}
*/

}
