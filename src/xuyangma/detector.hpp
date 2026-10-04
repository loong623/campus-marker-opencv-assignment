#pragma once
#include <opencv2/opencv.hpp>
#include <array>
using Quad=std::array<cv::Point2f,4>;
// detect marker in one frame
bool detectMarker(const cv::Mat& frame,Quad& bestCorners);
// make corners more stable
void smoothCorners(Quad& current,const Quad& previous,const cv::Size& frameSize);
// draw result
void drawDetection(cv::Mat& frame,const Quad& corners);