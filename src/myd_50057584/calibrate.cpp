// Camera calibration + calibration-board pose estimation.
//
// Stage 1 — Calibration:
//   Reads data/raw/calibration_video.avi, detects a 7×7 symmetric
//   circle grid (spacing 0.03 m) in sampled frames, and runs
//   cv::calibrateCamera to obtain intrinsics + distortion coefficients.
//   Results saved to calib_results.yaml.
//
// Stage 2 — Pose estimation:
//   In every frame of the same calibration video, detects the circle
//   grid and uses cv::solvePnP with the calibrated intrinsics to
//   estimate the board's 6-DOF pose.  Draws 3D coordinate axes and
//   reports translation / distance statistics.
//
// Board coordinate system:
//   Origin at the top-left circle centre.
//   X axis: rightward (along columns), Y axis: downward (along rows),
//   Z axis: toward the camera (board normal, right-handed).
//   Units: metres.  Circle spacing: 0.03 m.
//
// Camera coordinate system (OpenCV convention):
//   X right, Y down, Z forward (into the scene).
//   tvec gives the board origin's position in camera coordinates.
//
// Usage:
//   ./calibrate                                    # calibrate + pose, default paths
//   ./calibrate --input <video>                    # specify input video
//   ./calibrate --calib <yaml>                     # run calibration, output to yaml
//   ./calibrate --pose <video>                     # run pose estimation, output to video
//   If both --calib and --pose are given, calibrate first then estimate pose.
//   If neither is given, do both with default output paths.

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/objdetect.hpp>     // findCirclesGrid, CALIB_CB_*
#include <opencv2/calib.hpp>         // calibrateCamera
#include <opencv2/geometry/3d.hpp>   // solvePnP, projectPoints, Rodrigues

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <filesystem>

namespace fs = std::filesystem;
namespace {

// ── Parameters ────────────────────────────────────────────

struct CalibParams {
    cv::Size patternSize = {7, 7};     // 7×7 symmetric circle grid
    double squareSize    = 0.03;       // 0.03 m spacing
    int frameStride      = 30;         // sample every N-th frame for calibration
    int minFrames        = 15;
    int maxFrames        = 80;
};

// ── Generate 3D object points for the circle grid ────────
// Origin at top-left circle centre, X right, Y down, Z=0.

std::vector<cv::Point3f> circleGridObjectPoints(const CalibParams& cp) {
    std::vector<cv::Point3f> pts;
    for (int r = 0; r < cp.patternSize.height; ++r)
        for (int c = 0; c < cp.patternSize.width; ++c)
            pts.emplace_back((float)(c * cp.squareSize),
                             (float)(r * cp.squareSize), 0.0f);
    return pts;
}

// ── Stage 1: Calibration ─────────────────────────────────

bool runCalibration(const std::string& videoPath, const CalibParams& cp,
                    cv::Mat& cameraMatrix, cv::Mat& distCoeffs,
                    const std::string& outputPath) {
    std::cout << "=== Calibration ===\n"
              << "Video: " << videoPath << "\n"
              << "Pattern: " << cp.patternSize.width << "x" << cp.patternSize.height
              << " circles, spacing=" << cp.squareSize << " m\n"
              << "Sampling every " << cp.frameStride << " frames\n";

    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        std::cerr << "Cannot open " << videoPath << "\n";
        return false;
    }
    int W = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int H = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    int total = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
    std::cout << "Resolution: " << W << "x" << H << "  frames=" << total << "\n";

    auto objTemplate = circleGridObjectPoints(cp);
    std::vector<std::vector<cv::Point3f>> objectPoints;
    std::vector<std::vector<cv::Point2f>> imagePoints;

    cv::Mat frame, gray;
    int frameIdx = 0, goodCount = 0;
    while (cap.read(frame) && goodCount < cp.maxFrames) {
        ++frameIdx;
        if (frameIdx % cp.frameStride != 0) continue;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        std::vector<cv::Point2f> centers;
        if (!cv::findCirclesGrid(gray, cp.patternSize, centers,
                                 cv::CALIB_CB_SYMMETRIC_GRID |
                                 cv::CALIB_CB_CLUSTERING))
            continue;
        objectPoints.push_back(objTemplate);
        imagePoints.push_back(centers);
        if (++goodCount % 10 == 0)
            std::cout << "  Collected " << goodCount << " good frames...\n";
    }
    cap.release();
    std::cout << "Collected " << goodCount << " good frames\n";
    if (goodCount < cp.minFrames) {
        std::cerr << "Error: only " << goodCount << " good frames (need >= "
                  << cp.minFrames << ")\n";
        return false;
    }

    cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    distCoeffs = cv::Mat::zeros(5, 1, CV_64F);
    std::vector<cv::Mat> rvecs, tvecs;
    double rms = cv::calibrateCamera(objectPoints, imagePoints, {W, H},
                                     cameraMatrix, distCoeffs, rvecs, tvecs);

    // Per-view reprojection error
    double totalErr = 0;
    for (size_t i = 0; i < objectPoints.size(); ++i) {
        std::vector<cv::Point2f> projected;
        cv::projectPoints(objectPoints[i], rvecs[i], tvecs[i],
                          cameraMatrix, distCoeffs, projected);
        totalErr += cv::norm(imagePoints[i], projected, cv::NORM_L2) / imagePoints[i].size();
    }
    std::cout << "\nCalibration complete.\n"
              << "  RMS reprojection error: " << rms << " px\n"
              << "  Mean per-view error: " << totalErr / objectPoints.size() << " px\n"
              << "  Camera matrix:\n" << cameraMatrix << "\n"
              << "  Distortion: " << distCoeffs.t() << "\n";

    cv::FileStorage fs(outputPath, cv::FileStorage::WRITE);
    if (!fs.isOpened()) { std::cerr << "Cannot write " << outputPath << "\n"; return false; }
    fs << "camera_matrix" << cameraMatrix
       << "distortion_coefficients" << distCoeffs
       << "image_width" << W
       << "image_height" << H
       << "rms_reprojection_error" << rms
       << "num_frames_used" << goodCount
       << "pattern_type" << "symmetric_circles"
       << "pattern_size" << cp.patternSize
       << "square_size" << cp.squareSize;
    fs.release();
    std::cout << "  Saved to " << outputPath << "\n";
    return true;
}

// ── Stage 2: Pose estimation of the calibration board ────

bool runPoseEstimation(const std::string& videoPath, const CalibParams& cp,
                       const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs,
                       const std::string& outputPath) {
    std::cout << "\n=== Pose Estimation ===\n"
              << "Video: " << videoPath << "\n"
              << "Camera matrix:\n" << cameraMatrix << "\n";

    // 3D points: 4 corners of the board for PnP
    // (use the 4 extreme circle centres: TL, TR, BL, BR)
    auto objPoints = circleGridObjectPoints(cp);
    int cols = cp.patternSize.width, rows = cp.patternSize.height;
    std::vector<cv::Point3f> boardCorners = {
        objPoints[0],                  // TL: (0, 0, 0)
        objPoints[cols - 1],           // TR: ((cols-1)*s, 0, 0)
        objPoints[cols * rows - 1],    // BR: ((cols-1)*s, (rows-1)*s, 0)
        objPoints[cols * (rows - 1)]   // BL: (0, (rows-1)*s, 0)
    };

    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) { std::cerr << "Cannot open " << videoPath << "\n"; return false; }
    int W = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int H = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    int total = (int)cap.get(cv::CAP_PROP_FRAME_COUNT);
    double fps = cap.get(cv::CAP_PROP_FPS);
    std::cout << "Resolution: " << W << "x" << H << "  fps=" << fps
              << "  frames=" << total << "\n"
              << "Board physical size: "
              << (cols - 1) * cp.squareSize * 100 << " x "
              << (rows - 1) * cp.squareSize * 100 << " cm\n";

    cv::VideoWriter writer;
    if (!outputPath.empty()) {
        writer.open(outputPath, cv::CAP_FFMPEG,
                    cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
                    (fps > 0 ? fps : 25.0), {W, H});
    }

    cv::Mat frame, gray;
    int frameIdx = 0, poseCount = 0;
    std::vector<double> distances, txs, tys, tzs;

    // For 3D axis drawing: 5 cm axes
    float axisLen = (float)(cp.squareSize * 3);  // 9 cm axes
    std::vector<cv::Point3f> axis3D = {
        {0, 0, 0}, {axisLen, 0, 0}, {0, axisLen, 0}, {0, 0, -axisLen}
    };

    while (cap.read(frame)) {
        ++frameIdx;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        std::vector<cv::Point2f> centers;
        bool found = cv::findCirclesGrid(gray, cp.patternSize, centers,
                                         cv::CALIB_CB_SYMMETRIC_GRID |
                                         cv::CALIB_CB_CLUSTERING);

        cv::Vec3d rvec, tvec;
        bool havePose = false;

        if (found) {
            // Use the 4 extreme circle centres for PnP
            std::vector<cv::Point2f> imgCorners = {
                centers[0], centers[cols - 1],
                centers[cols * rows - 1], centers[cols * (rows - 1)]
            };
            if (cv::solvePnP(boardCorners, imgCorners, cameraMatrix,
                             distCoeffs, rvec, tvec, false, cv::SOLVEPNP_ITERATIVE)) {
                havePose = true;
                ++poseCount;
                double dist = std::sqrt(tvec[0]*tvec[0] + tvec[1]*tvec[1] + tvec[2]*tvec[2]);
                distances.push_back(dist);
                txs.push_back(tvec[0]);
                tys.push_back(tvec[1]);
                tzs.push_back(tvec[2]);
            }
        }

        // ── Draw ──
        cv::Mat out = frame.clone();

        if (found) {
            // Draw detected circle centres
            for (auto& c : centers)
                cv::circle(out, c, 3, {0, 255, 255}, -1);
            // Draw board outline
            std::vector<cv::Point> outline = {
                centers[0], centers[cols - 1],
                centers[cols * rows - 1], centers[cols * (rows - 1)]
            };
            cv::polylines(out, outline, true, {0, 255, 0}, 2);
        }

        if (havePose) {
            // Draw 3D coordinate axes
            std::vector<cv::Point2f> axis2D;
            cv::projectPoints(axis3D, rvec, tvec, cameraMatrix, distCoeffs, axis2D);
            cv::line(out, axis2D[0], axis2D[1], {0, 0, 255}, 3);   // X red
            cv::line(out, axis2D[0], axis2D[2], {0, 255, 0}, 3);   // Y green
            cv::line(out, axis2D[0], axis2D[3], {255, 0, 0}, 3);   // Z blue

            double dist = std::sqrt(tvec[0]*tvec[0] + tvec[1]*tvec[1] + tvec[2]*tvec[2]);
            // Convert rvec to Euler angles (degrees) for readability
            cv::Mat R;
            cv::Rodrigues(rvec, R);
            // ZYX Euler decomposition
            double pitch = std::asin(-R.at<double>(2, 0)) * 180.0 / CV_PI;
            double yaw   = std::atan2(R.at<double>(2, 1), R.at<double>(2, 2)) * 180.0 / CV_PI;
            double roll  = std::atan2(R.at<double>(1, 0), R.at<double>(0, 0)) * 180.0 / CV_PI;

            std::string info = cv::format("t=(%.3f,%.3f,%.3f)m  d=%.3fm  RPY=(%.1f,%.1f,%.1f)deg",
                                          tvec[0], tvec[1], tvec[2], dist, roll, pitch, yaw);
            cv::putText(out, info, {20, 40}, cv::FONT_HERSHEY_SIMPLEX, 0.5, {0, 255, 0}, 1);
            cv::putText(out, "POSE ESTIMATED", {20, 70}, cv::FONT_HERSHEY_SIMPLEX, 0.7, {0, 255, 0}, 2);
        } else {
            cv::putText(out, "NOT DETECTED", {20, 40}, cv::FONT_HERSHEY_SIMPLEX, 1.0, {0, 0, 255}, 2);
        }

        cv::putText(out, "frame " + std::to_string(frameIdx) + "/" + std::to_string(total),
                    {20, H - 20}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {255, 255, 255}, 1);

        if (writer.isOpened()) writer.write(out);

        if (havePose) {
            double dist = std::sqrt(tvec[0]*tvec[0] + tvec[1]*tvec[1] + tvec[2]*tvec[2]);
            printf("frame %4d  POSE  t=(%.4f, %.4f, %.4f)  dist=%.4f m\n",
                   frameIdx, tvec[0], tvec[1], tvec[2], dist);
        }
    }
    cap.release();
    if (writer.isOpened()) writer.release();

    // ── Statistics ──
    auto stats = [](const std::vector<double>& v) {
        if (v.empty()) return std::string("(no data)");
        double mean = std::accumulate(v.begin(), v.end(), 0.0) / v.size();
        double minV = *std::min_element(v.begin(), v.end());
        double maxV = *std::max_element(v.begin(), v.end());
        double var = 0;
        for (double d : v) var += (d - mean) * (d - mean);
        double std_ = std::sqrt(var / v.size());
        return cv::format("mean=%.4f  min=%.4f  max=%.4f  std=%.4f", mean, minV, maxV, std_);
    };

    std::cout << "\nPose estimation complete.\n"
              << "  Poses estimated: " << poseCount << "/" << frameIdx << " frames ("
              << 100.0 * poseCount / frameIdx << "%)\n"
              << "  Distance (m): " << stats(distances) << "\n"
              << "  TX (m): " << stats(txs) << "\n"
              << "  TY (m): " << stats(tys) << "\n"
              << "  TZ (m): " << stats(tzs) << "\n";
    return true;
}

} // namespace

int main(int argc, char** argv) {
    std::string inputPath    = "data/raw/calibration_video.avi";
    std::string calibOutput  = "output/calib_results.yaml";
    std::string poseOutput   = "output/pose_result.avi";
    bool doCalib = false, doPose = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--input" && i + 1 < argc) {
            inputPath = argv[++i];
        } else if (a == "--calib" && i + 1 < argc) {
            calibOutput = argv[++i];
            doCalib = true;
        } else if (a == "--pose" && i + 1 < argc) {
            poseOutput = argv[++i];
            doPose = true;
        } else {
            std::cerr << "Usage: " << argv[0]
                      << " [--input <video>] [--calib <yaml>] [--pose <video>]\n"
                      << "  --input  输入视频路径（默认 data/raw/calibration_video.avi）\n"
                      << "  --calib  执行标定，输出标定结果到指定 yaml 路径\n"
                      << "  --pose   执行位姿估计，输出标注视频到指定路径\n"
                      << "  不带 --calib 和 --pose 时，同时执行标定和位姿估计\n";
            return 1;
        }
    }
    if (!doCalib && !doPose) { doCalib = true; doPose = true; }

    CalibParams cp;
    cv::Mat cameraMatrix, distCoeffs;

    if (doCalib) {
        if (!runCalibration(inputPath, cp, cameraMatrix, distCoeffs, calibOutput))
            return 1;
    }
    if (doPose) {
        if (!doCalib) {
            cv::FileStorage fs(calibOutput, cv::FileStorage::READ);
            if (!fs.isOpened()) {
                std::cerr << "Cannot read " << calibOutput
                          << ". Run calibration first.\n";
                return 1;
            }
            fs["camera_matrix"] >> cameraMatrix;
            fs["distortion_coefficients"] >> distCoeffs;
            fs.release();
        }
        if (!runPoseEstimation(inputPath, cp, cameraMatrix, distCoeffs, poseOutput))
            return 1;
    }
    return 0;
}
