#include <map>
#include <set>
// Bounded imaging controls; reuse the original renderer unchanged.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "corners/corner_edge_fit.hpp"
#include "core/observed_geometry_utils.hpp"
#define MODEL_YAML "src/tushenghao/config/marker_geometry.yaml"
#define main archived_generator_main
#include "/home/tushenghao/projects/campus-marker-opencv-assignment/src/tushenghao/tools/block3_fixture/generation/main.cpp"
#undef main
namespace mark {
EdgePairFitResult measureEdgePair(const std::vector<cv::Point>&,const std::array<std::array<cv::Point2d,2>,2>&,const CornerConfig&);
}
int main(int argc,char** argv) {
 try {
  if(argc!=4)throw std::runtime_error("usage: controls generate|measure IMAGE_DIR MODE_NAME");
  cv::setNumThreads(1);std::string command=argv[1],tag=argv[3];std::filesystem::path dir=argv[2];
  if(command=="generate")std::filesystem::create_directories(dir);
    cv::FileStorage fs("src/tushenghao/docs/evidence/final-fixes/path-a/verification-release/effective_config.yaml",cv::FileStorage::READ),model("src/tushenghao/config/marker_geometry.yaml",cv::FileStorage::READ);
    if(!fs.isOpened()||!model.isOpened())throw std::runtime_error("cannot open config/model");
    auto node=fs["detector"]["corner"], obs=node["observation"];
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
      (double)obs["max_turn_connection_length_px"],(double)obs["max_support_extension_px"],
      (double)obs["min_support_span_px"]};
    std::map<std::string,std::vector<cv::Point2d>> polygons;
    for(auto polygon:model["polygons"]) {
      std::string id=(std::string)polygon["id"];
      for(auto xy:polygon["vertices"])polygons[id].push_back({(double)xy[0],(double)xy[1]});
    }
  auto pieces=::model(MODEL_YAML);
  int number=0;
  for(int phase=0;phase<2;++phase)for(int radius:{0,2})for(int angle:{0,1,2,3}) {
    sg::Point center=phase?sg::Point(720.25,540.5):sg::Point(720,540);
    auto generated=scene(pieces,angle,radius,center);
    std::ostringstream filename;filename<<"input-"<<std::setw(2)<<std::setfill('0')<<number<<".png";
    auto path=dir/filename.str();
    if(command=="generate") {
      auto image=input(sg::rasterize(generated,1440,1080),0);
      if(std::filesystem::exists(path)||!cv::imwrite(path.string(),image))throw std::runtime_error("refusing overwrite / write failed");
    }else {
      auto image=cv::imread(path.string());if(image.empty())throw std::runtime_error("missing control image");
      cv::Mat gray,mask;cv::cvtColor(image,gray,cv::COLOR_BGR2GRAY);cv::threshold(gray,mask,200,255,cv::THRESH_BINARY);
      std::vector<std::vector<cv::Point>> contours;cv::findContours(mask,contours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_NONE);
      std::vector<cv::Point2d> centers;
      for(const auto& contour:contours){auto m=cv::moments(contour);centers.push_back(m.m00?cv::Point2d(m.m10/m.m00,m.m01/m.m00):cv::Point2d(-999,-999));}
      for(int corner=0;corner<4;++corner) {
        std::string id=std::array<std::string,4>{"L0","M1","L2","L3"}[corner];
        auto piece=std::find_if(generated.pieces.begin(),generated.pieces.end(),[&](const auto& x){return x.id==id;});
        std::vector<cv::Point2f> moment_pixels;for(auto point:piece->perturbed)moment_pixels.emplace_back(point);auto expected_moments=cv::moments(moment_pixels);cv::Point2d expected_center(expected_moments.m10/expected_moments.m00,expected_moments.m01/expected_moments.m00);
        size_t index=contours.size();double distance=INFINITY;
        for(size_t k=0;k<centers.size();++k){double d=cv::norm(centers[k]-expected_center);if(d<distance){distance=d;index=k;}}
        if(index==contours.size()||distance>4)throw std::runtime_error("control component identity failed");
        std::array<std::array<cv::Point2d,2>,2> edges;int first=corner==1?0:5,second=corner==1?1:0;
        for(int edge=0;edge<2;++edge)for(int end=0;end<2;++end)edges[edge][end]=piece->ideal[((edge?second:first)+end)%piece->ideal.size()];
        auto production=mark::fitObservedEdgePair(contours[index],edges,config);
        auto raw=mark::measureEdgePair(contours[index],edges,config);
        sg::Json record={{"mode",tag},{"control",number},{"angle",angle},{"phase",phase},{"radius",radius},{"corner",corner},
          {"frozen_gate_pass",bool(production.evidence)},{"frozen_reason",production.reason},{"raw_available",bool(raw.evidence)},{"raw_reason",raw.reason}};
        if(raw.evidence) {
          auto& e=*raw.evidence;std::array<size_t,2> visits,unique,retraces{};
          std::array<double,2> span{};bool correct=true;
          for(int edge=0;edge<2;++edge) {
            auto arc=e.original_support_arcs_[edge];std::set<std::pair<double,double>> points;
            auto line=edge?e.line_b_:e.line_a_;double lo=INFINITY,hi=-INFINITY;
            for(size_t k=0;k<arc.size();++k){auto point=arc[k];points.emplace(point.x,point.y);
              double projection=point.dot(cv::Point2d(line[0],line[1]));lo=std::min(lo,projection);hi=std::max(hi,projection);
              if(k>=2&&point==arc[k-2])++retraces[edge];
              double desired=mark::observed::segmentDistance(point,edges[edge][0],edges[edge][1]);
              if(desired-mark::observed::boundaryDistance(point,piece->ideal)>1e-10)correct=false;
            }
            visits[edge]=arc.size();unique[edge]=points.size();span[edge]=hi-lo;
          }
          auto truth=piece->ideal[corner==1?1:0];
          record["mean_residual"]=e.line_mean_residual_px_;record["maximum_residual"]=e.line_max_residual_px_;
          record["occurrences"]=visits;record["unique_pixels"]=unique;record["retraces"]=retraces;record["span"]=span;
          record["truth_error"]=cv::norm(e.intersection_-truth);record["correct_model_edge_support"]=correct;
        }
        std::cout<<sg::canonical(record)<<std::endl;
      }
    }
    ++number;
  }
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
