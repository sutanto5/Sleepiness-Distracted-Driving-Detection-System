// test_opencv.cpp
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <iostream>

int main() {
    cv::Mat img = cv::imread("nonexistent.png");
    std::cout << "OpenCV version: " << CV_VERSION << "\n";
    std::cout << "Loaded image? " << (!img.empty()) << "\n";
    return 0;
}