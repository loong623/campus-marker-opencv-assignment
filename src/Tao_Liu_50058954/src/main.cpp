#include<iostream>
#include<string>
#include<vector>

#include<opencv2/opencv.hpp>

void test() {
    std::cout << "This is a test function." << std::endl;
}

void print_information(){

}

//MASK
cv::Mat get_mask(const cv::Mat& frame, const cv::Mat& kernel) {
    cv::Mat gray, mask;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::inRange(gray, cv::Scalar(200), cv::Scalar(255), mask);
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);
    return mask;
}

//获得角点
std::vector<cv::Point2f> orderQuadByAngle(std::vector<cv::Point2f>& points){
    cv::Point2f center(0, 0);
    for (const auto& point : points) {
        center += point;
    }
    center *= (1.0 / points.size());
    std::vector<cv::Point2f> ordered_points(4);
    for (const auto& point : points) {
        // Calculate the angle between the center and the point
        float angle = atan2(point.y - center.y, point.x - center.x);
        // Order the points based on their angles
        if (angle < 0) angle += 2 * M_PI;
        if (angle < M_PI_2) {
            ordered_points[0] = point;
        } else if (angle < M_PI) {
            ordered_points[1] = point;
        } else if (angle < 3 * M_PI_2) {
            ordered_points[2] = point;
        } else {
            ordered_points[3] = point;
        }
    }
    return ordered_points;
}

bool validateRect(const cv::RotatedRect& rect) {
    // Check if the rectangle is within the frame boundaries
    const float w=rect.size.width;
    const float h=rect.size.height;
    if (w < 30 || h < 30)  return false; // Too small
    if(rect.size.area()<2000) return false; // Too small
    const float aspect_ratio = std::max(w, h) / std::min(w, h);
    if (aspect_ratio > 1.5f) return false; // Too elongated
        
    return true;
}




int main(int argc, char** argv) {
    //视频文件读取
    std::cout<<"How to use: ./build/tliu154 <video_file_path>"<<std::endl;
    std::cout<<"If no video file path is provided, the default video file '../data/marker_video.avi' will be used."<<std::endl;
    std::string video_path = argc > 1 ? argv[1] : "../data/marker_video.avi";
    std::cout << "Opening video file: " << video_path << std::endl;
    cv::VideoCapture cap(video_path,cv::CAP_FFMPEG);
    if(!cap.isOpened()) {
        std::cerr << "Error: Could not open video file: " << video_path << std::endl;
        return -1;
    }
    
    
    cv::Mat kernel=cv::getStructuringElement(cv::MORPH_RECT,cv::Size(3,3));
    cv::Mat frame,result;
    cv::Mat mask,gray;
    cv::RotatedRect rect;
    cv::Point2f vertices[4];
    std::vector<cv::Vec4i> hierarchy;
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Point> pts;
    std::string position="";
    //读取视频信息
    int frame_index = 0;
    int video_w=cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int video_h=cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    double video_fps=cap.get(cv::CAP_PROP_FPS);
    std::cout << "Video width: " << video_w << ", height: " << video_h << ", fps: " << video_fps << std::endl;

    cv::VideoWriter writer("../output/videos/result.avi",cv::CAP_FFMPEG, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), video_fps, cv::Size(video_w, video_h));
    if(!writer.isOpened()) {
        std::cerr << "Error: Could not open video writer." << std::endl;
        return -1;
    }

    //主循环
    while(cap.read(frame)) {
        frame_index++;
        //if(frame_index %30!=0) continue;
        if(frame.empty()) {
            break; // End of video
        }

        mask = get_mask(frame, kernel);

        //拿到轮廓
        contours.clear();
        hierarchy.clear();
        cv::findContours(mask.clone(), contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        //所有的点
        pts.clear();
        if(!contours.empty()){
            for (auto &c:contours){
                if(cv::contourArea(c) < 30) continue;
                pts.insert(pts.end(), c.begin(), c.end());
            }
        }
        std::vector<cv::Point> hull;
        if (pts.size()<4){
           
            cv::putText(frame, "NOT DETECTED", cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
            cv::putText(frame, position, cv::Point(50, 100), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
            
            writer.write(frame);
            if(frame_index % 30 != 0) continue; // Save every 30 frames
            std::cout << "Frame " << frame_index << ": Not enough points detected." <<" Position: "<< position << std::endl;
            cv::imwrite("../output/frames/frame_" + std::to_string(frame_index) + ".png", frame);
            continue;
        }
        cv::convexHull(pts, hull);
        

        rect = cv::minAreaRect(hull);
        rect.points(vertices);
        //几何检验
        if (!validateRect(rect)) {
            if(rect.center.x<frame.cols/2){
                position="LEFT";
            } else {
                position="RIGHT";
            }
            
            cv::putText(frame, "NOT DETECTED", cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
            cv::putText(frame, position, cv::Point(50, 100), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
            
            writer.write(frame);
            if(frame_index % 30 != 0) continue; // Save every 30 frames
            std::cout << "Frame " << frame_index << ": Detected rectangle is invalid." <<" Position: "<< position << std::endl;
            cv::imwrite("../output/frames/frame_" + std::to_string(frame_index) + ".png", frame);
            continue;
        }
        //画线
        for(int i = 0; i < 4; i++) {
            cv::line(frame, vertices[i], vertices[(i+1)%4], cv::Scalar(0, 255, 0), 2);
        }
        //获得角点：LT，RT，RB，LB
        std::vector<cv::Point2f> corner_points(4);
        std::vector<cv::Point2f> quad{vertices[0], vertices[1], vertices[2], vertices[3]};
        corner_points = orderQuadByAngle(quad);
        //画角点
        for(int i = 0; i < 4; i++) {
            cv::circle(frame, corner_points[i], 5, cv::Scalar(0, 0, 255), -1);
            if(i==2){
                cv::putText(frame, "LT", corner_points[i], cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
            } else if(i==3){
                cv::putText(frame, "RT", corner_points[i], cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
            } else if(i==0){
                cv::putText(frame, "RB", corner_points[i], cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
            } else {
                cv::putText(frame, "LB", corner_points[i], cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 0, 255), 2);
            }
        }
        //画中心点
        cv::Point2f center = rect.center;
        cv::circle(frame, center, 5, cv::Scalar(255, 0, 0), -1);
        
        //保存
        writer.write(frame);
        if(frame_index % 30 != 0) continue; // Save every 30 frames
   
        cv::imwrite("../output/masks/mask_" + std::to_string(frame_index) + ".png", mask);
        cv::imwrite("../output/frames/frame_" + std::to_string(frame_index) + ".png", frame);
    }
    cap.release();
    writer.release();
    return 0;
}