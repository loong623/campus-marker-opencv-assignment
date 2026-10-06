// Offline decode replay from recorded completed hypotheses, using unchanged production functions.
#include "corners/corner_resolver.hpp"
#include "corners/detection_validator.hpp"
#include "corners/semantic_resolver.hpp"
#include "corners/detection_publication.hpp"
#include <opencv2/opencv.hpp>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>

std::string quote(const std::string& s){std::ostringstream o;o<<std::quoted(s);return o.str();}
void replay(const mark::PreparedFrame& frame,const std::vector<mark::GeometryHypothesis>& hypotheses,
            bool truncated,const mark::MarkerGeometry& model,const mark::CornerConfig& config) {
    const auto started=std::chrono::steady_clock::now();
    std::vector<mark::CornerMeasurement> measurements;
    std::vector<std::string> failures;
    bool unresolved=false;
    for(size_t h=0;h<hypotheses.size();++h) {
        const auto& hypothesis=hypotheses[h];
        if(hypothesis.completeness_==mark::GeometryCompleteness::CLEARLY_INCOMPLETE)continue;
        std::set<std::string> parts;
        for(const auto& a:hypothesis.assignments_)parts.insert(a.model_part_id_);
        if(parts!=std::set<std::string>{"L0","L2","L3","M1","S1a","S1b"})throw std::runtime_error("unexpected assignment set");
        auto corner=mark::resolveObservedCorners(frame,hypothesis,model,config);
        if(!corner.measurement_) {unresolved=true;failures.push_back(std::to_string(h)+"/"+corner.rejection_reason_);continue;}
        auto order=mark::orderScreenCorners(corner.measurement_->physical_corners_,config);
        if(!order.screen_order_) {unresolved=true;failures.push_back("screen/"+order.rejection_reason_);continue;}
        auto validation=mark::validateDetectionGeometry(*corner.measurement_,*order.screen_order_,frame.original_image_.size(),config);
        if(!validation.valid_) {unresolved=true;failures.push_back("validator/"+validation.rejection_reason_);continue;}
        measurements.push_back(*corner.measurement_);
    }
    std::vector<mark::Detection> detections;
    bool competition=unresolved&&!measurements.empty();
    if(!competition) {
        auto semantics=mark::resolveSemantics(measurements,truncated,config);
        if(semantics.geometry_consistent_) {
            for(const auto& measurement:semantics.retained_measurements_) {
                std::string reason;
                auto detection=mark::publishFloatDetection(measurement,semantics.orientation_unique_,frame.original_image_.size(),config,reason);
                if(!detection){failures.push_back("publish/"+reason);detections.clear();break;}
                detections.push_back(*detection);
            }
        }else if(!measurements.empty()) failures.push_back("semantic/"+semantics.rejection_reason_);
    }
    double elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
    std::cout<<"{\"detected\":"<<(!detections.empty()?"true":"false")<<",\"measurements\":"<<measurements.size()
             <<",\"unresolved_competition\":"<<(competition?"true":"false")<<",\"elapsed_ms\":"<<elapsed<<",\"failures\":[";
    for(size_t i=0;i<failures.size();++i){if(i)std::cout<<',';std::cout<<quote(failures[i]);}
    std::cout<<"],\"detections\":[";
    for(size_t i=0;i<detections.size();++i) {
        if(i)std::cout<<',';
        auto d=detections[i];std::cout<<"{\"corners\":[";
        for(int p=0;p<4;++p){if(p)std::cout<<',';std::cout<<'['<<d.corners[p].x<<','<<d.corners[p].y<<']';}
        std::cout<<"],\"orientation\":";
        if(d.attributes.orientation){std::cout<<'[';for(int p=0;p<4;++p){if(p)std::cout<<',';std::cout<<(*d.attributes.orientation)[p];}std::cout<<']';}
        else std::cout<<"null";
        std::cout<<'}';
    }
    std::cout<<"]}";
}
int main(int argc,char** argv) {
  try {
    if(argc<5||argc>7)throw std::runtime_error("usage: full_replay VIDEO CONFIG MODEL TARGETS [POSITION] [probe-only]");
    cv::setNumThreads(1);std::cout<<std::setprecision(17);
    cv::FileStorage fs(argv[2],cv::FileStorage::READ),model_file(argv[3],cv::FileStorage::READ);
    if(!fs.isOpened()||!model_file.isOpened())throw std::runtime_error("config/model unavailable");
    auto node=fs["detector"]["corner"],obs=node["observation"];
    mark::CornerConfig config;
    config.min_line_points_=(int)node["min_line_points"];
    config.max_line_fit_error_=(double)node["max_line_fit_error"];
    config.min_intersection_angle_deg_=(double)node["min_intersection_angle_deg"];
    config.max_corner_error_=(double)node["max_corner_error"];
    config.approximation_epsilon_=(double)node["approximation_epsilon"];
    config.semantic_geometry_threshold_=(double)node["semantic_geometry_threshold"];
    config.observation_budget_=mark::CornerObservationBudget{
      (double)obs["max_edge_direction_diff_deg"],(double)obs["max_edge_position_distance_px"],
      (double)obs["max_component_mapping_distance_px"],(double)obs["turn_trim_distance_px"],
      (double)obs["max_turn_connection_length_px"],(double)obs["max_support_extension_px"],(double)obs["min_support_span_px"]};
    auto probe=config;probe.max_line_fit_error_=0.75;
    if(argc>=6)probe.observation_budget_->max_edge_position_distance_px=std::stod(argv[5]);
    const bool probe_only=argc==7&&std::string(argv[6])=="probe-only";
    mark::MarkerGeometry model;
    for(auto node:model_file["polygons"]) {
      mark::GeometryPolygon polygon;polygon.id=(std::string)node["id"];
      polygon.area=(double)node["area"];
      auto anchor=node["anchor"];polygon.anchor={(float)anchor[0],(float)anchor[1]};
      for(auto xy:node["vertices"])polygon.vertices.push_back({(float)xy[0],(float)xy[1]});
      model.polygons.push_back(polygon);
    }
    std::ifstream in(argv[4]);cv::VideoCapture video(argv[1],cv::CAP_FFMPEG);
    if(!in||!video.isOpened())throw std::runtime_error("targets/video unavailable");
    int id,ncomponents,nhypotheses,truncated,status,count=0;cv::Mat image;
    while(in>>id>>ncomponents>>nhypotheses>>truncated>>status) {
      if(id!=count||!video.read(image))throw std::runtime_error("frame sequence mismatch");
      mark::PreparedFrame frame;frame.frame_id_=id;frame.original_image_=image;
      frame.original_white_threshold_=(int)fs["preprocess"]["threshold"];
      cv::resize(image,frame.image_,{(int)fs["preprocess"]["work_width"],(int)fs["preprocess"]["work_height"]});
      frame.scale_x_=(double)frame.image_.cols/image.cols;frame.scale_y_=(double)frame.image_.rows/image.rows;
      for(int c=0;c<ncomponents;++c) {
        mark::WhiteComponent component;int border,n;
        in>>component.component_id_>>component.area_>>border>>n;component.touches_border_=border;
        for(int p=0;p<n;++p){int x,y;in>>x>>y;component.contour_.emplace_back(x,y);}
        frame.components_.push_back(component);
      }
      std::vector<mark::GeometryHypothesis> hypotheses;
      for(int h=0;h<nhypotheses;++h) {
        mark::GeometryHypothesis hypothesis;int completeness,n;
        in>>completeness>>hypothesis.validation_residual_;
        hypothesis.completeness_=static_cast<mark::GeometryCompleteness>(completeness);
        hypothesis.affine_transform_=cv::Mat(2,3,CV_64F);
        for(int r=0;r<2;++r)for(int c=0;c<3;++c)in>>hypothesis.affine_transform_.at<double>(r,c);
        in>>n;
        for(int a=0;a<n;++a){mark::ComponentAssignment assignment;in>>assignment.model_part_id_>>assignment.component_id_;hypothesis.assignments_.push_back(assignment);}
        hypotheses.push_back(hypothesis);
      }
      if(!in)throw std::runtime_error("incomplete target");
      std::cout<<"{\"frame_id\":"<<id<<",\"recorded_status\":"<<status<<",\"baseline\":";
      if(probe_only)std::cout<<"null";else replay(frame,hypotheses,truncated,model,config);
      std::cout<<(argc>=6?",\"candidate\":":",\"residual_0_75\":");replay(frame,hypotheses,truncated,model,probe);
      std::cout<<'}'<<std::endl;++count;
    }
    if(count!=1676||video.read(image))throw std::runtime_error("incomplete or extra video");
    return 0;
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
