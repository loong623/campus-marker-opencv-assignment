#include <opencv2/opencv.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <stdexcept>
#include <string>

// Sequential decoding: frame IDs are zero based; never seek, resize or overlay.
int main(int argc, char** argv) {
  try {
    if (argc != 4) throw std::runtime_error("usage: exporter VIDEO TARGETS_TSV OUTPUT_ROOT");
    std::ifstream input(argv[2]);
    if (!input) throw std::runtime_error("cannot open targets");
    std::map<int, std::string> targets;
    int id; std::string relative;
    while (input >> id >> relative) {
      if (id < 0 || !targets.emplace(id, relative).second)
        throw std::runtime_error("invalid or duplicate frame ID");
    }
    if (!input.eof() || targets.size() != 700) throw std::runtime_error("expected exactly 700 targets");
    cv::VideoCapture video(argv[1], cv::CAP_FFMPEG);
    if (!video.isOpened()) throw std::runtime_error("cannot open video");
    double fps = video.get(cv::CAP_PROP_FPS);
    cv::Mat frame; int count = 0, exported = 0;
    while (video.read(frame)) {
      if (frame.cols != 1440 || frame.rows != 1080 || frame.type() != CV_8UC3)
        throw std::runtime_error("unexpected source image format");
      auto target = targets.find(count);
      if (target != targets.end()) {
        auto path = std::filesystem::path(argv[3]) / target->second;
        std::filesystem::create_directories(path.parent_path());
        if (std::filesystem::exists(path)) throw std::runtime_error("refusing to overwrite image");
        if (!cv::imwrite(path.string(), frame)) throw std::runtime_error("PNG export failed");
        cv::Mat saved = cv::imread(path.string(), cv::IMREAD_UNCHANGED);
        if (saved.size() != frame.size() || saved.type() != frame.type() ||
            cv::norm(saved, frame, cv::NORM_INF) != 0)
          throw std::runtime_error("PNG pixel verification failed");
        ++exported;
      }
      ++count;
    }
    if (count != 1676 || exported != 700) throw std::runtime_error("incomplete video or export");
    std::cout << "{\"decoded_frames\":" << count << ",\"exported_frames\":" << exported
              << ",\"pixel_exact_verified\":" << exported
              << ",\"width\":1440,\"height\":1080,\"fps\":" << std::setprecision(17) << fps
              << ",\"frame_numbering\":\"zero_based_sequential\",\"backend\":\"FFMPEG\"}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
