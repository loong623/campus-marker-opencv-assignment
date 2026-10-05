#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include "detector.hpp"
struct CalibrationData{
    cv::Mat cameraMatrix;
    cv::Mat distCoeffs;
    cv::Size imageSize;
};
// camera calibration
bool calibrateCameraFromVideo(const std::string& videoPath,int boardCols,int boardRows,double pointSpacing,const std::string& outputPath);
// load calibration data
bool loadCalibration(const std::string& path,CalibrationData& data);
// estimate marker pose
bool estimatePose(const Quad& corners,const cv::Mat& cameraMatrix,const cv::Mat& distCoeffs,double markerSize,cv::Mat& rvec,cv::Mat& tvec,double& reprojectionError);
// draw XYZ axes
void drawPoseAxes(cv::Mat& frame,const cv::Mat& cameraMatrix,const cv::Mat& distCoeffs,const cv::Mat& rvec,const cv::Mat& tvec,double axisLength);