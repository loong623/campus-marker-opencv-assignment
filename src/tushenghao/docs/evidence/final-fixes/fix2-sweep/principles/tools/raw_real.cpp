#include <map>
#include <set>
// Standalone evidence tool. Reads PNGs and recorded correspondences; writes stdout only.
#include "corners/corner_observation.hpp"
#include "corners/corner_edge_fit.hpp"
#include "core/observed_geometry_utils.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <filesystem>
namespace mark {
EdgePairFitResult measureEdgePair(const std::vector<cv::Point>&,const std::array<std::array<cv::Point2d,2>,2>&,const CornerConfig&);
EdgePairFitResult unitPixelWeightPair(const std::vector<cv::Point>&,const std::array<std::array<cv::Point2d,2>,2>&,const CornerConfig&);
}

std::string quoted(const std::string& s) {
    std::ostringstream o; o << std::quoted(s); return o.str();
}
int main(int argc,char** argv) {
  try {
    if(argc!=5) throw std::runtime_error("usage: replay OUT CONFIG MODEL TARGETS");
    cv::setNumThreads(1);
    cv::FileStorage fs(argv[2],cv::FileStorage::READ),model(argv[3],cv::FileStorage::READ);
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
    std::ifstream in(argv[4]);if(!in)throw std::runtime_error("cannot open targets");
    int id,h,p,cid,n;char category;
    while(in>>id>>category>>h>>p>>cid>>n) {
      cv::Mat affine(2,3,CV_64F);
      for(int r=0;r<2;++r)for(int c=0;c<3;++c)in>>affine.at<double>(r,c);
      mark::WhiteComponent component;component.component_id_=cid;
      for(int i=0;i<n;++i){int x,y;in>>x>>y;component.contour_.emplace_back(x,y);}
      if(!in)throw std::runtime_error("incomplete target");
      std::ostringstream relative;relative<<"frames/"<<category<<"/frame-"<<std::setw(6)<<std::setfill('0')<<id<<".png";
      mark::PreparedFrame frame;
      frame.original_image_=cv::imread((std::filesystem::path(argv[1])/relative.str()).string());
      frame.original_white_threshold_=(int)fs["preprocess"]["threshold"];
      frame.scale_x_=(double)(int)fs["preprocess"]["work_width"]/frame.original_image_.cols;
      frame.scale_y_=(double)(int)fs["preprocess"]["work_height"]/frame.original_image_.rows;
      auto contour=mark::observeOriginalContour(frame,component,*config.observation_budget_);
      if(!contour.reason.empty())throw std::runtime_error(contour.reason);
      std::string part=std::array<std::string,4>{"L0","M1","L2","L3"}[p];
      auto vertices=polygons.at(part);int edge0=p==1?0:5,edge1=p==1?1:0;
      std::array<std::array<cv::Point2d,2>,2> edges;
      for(int edge=0;edge<2;++edge)for(int end=0;end<2;++end)
        edges[edge][end]=frame.workToOriginal(mark::observed::project(affine,vertices[((edge?edge1:edge0)+end)%vertices.size()]));
      auto baseline=mark::fitObservedEdgePair(contour.contour,edges,config);
      auto raw=mark::measureEdgePair(contour.contour,edges,config);
      auto unit=mark::unitPixelWeightPair(contour.contour,edges,config);
      std::cout<<std::setprecision(17)<<"{\"frame_id\":"<<id<<",\"hypothesis\":"<<h<<",\"corner\":"<<p
       <<",\"baseline_reason\":"<<quoted(baseline.reason)<<",\"raw_measurement_available\":"<<(raw.evidence?"true":"false")
       <<",\"raw_reason\":"<<quoted(raw.reason)<<",\"unit_weight_frozen_gate_pass\":"<<(unit.evidence?"true":"false")
       <<",\"unit_reason\":"<<quoted(unit.reason)<<",\"arcs\":[";
      if(raw.evidence) {
        const auto& e=*raw.evidence;
        for(int edge=0;edge<2;++edge) {
          if(edge)std::cout<<',';
          const auto& arc=e.original_support_arcs_[edge];auto line=edge?e.line_b_:e.line_a_;
          std::set<std::pair<double,double>> unique;
          for(auto xy:arc)unique.emplace(xy.x,xy.y);
          std::vector<cv::Point2d> pixels;for(auto xy:unique)pixels.emplace_back(xy.first,xy.second);
          cv::Vec4d refit;cv::fitLine(pixels,refit,cv::DIST_L2,0,.01,.01);
          double norm=std::hypot(refit[0],refit[1]);refit[0]/=norm;refit[1]/=norm;
          double unique_same=0,unique_refit=0,squared=0;int retraces=0;
          for(auto xy:pixels) {
            double d=std::abs(line[0]*(xy.y-line[3])-line[1]*(xy.x-line[2]));unique_same+=d;
            unique_refit+=std::abs(refit[0]*(xy.y-refit[3])-refit[1]*(xy.x-refit[2]));
          }
          for(size_t k=0;k<arc.size();++k) {
            double d=line[0]*(arc[k].y-line[3])-line[1]*(arc[k].x-line[2]);squared+=d*d;
            if(k>=2&&arc[k]==arc[k-2])++retraces;
          }
          std::cout<<"{\"occurrences\":"<<arc.size()<<",\"unique_pixels\":"<<pixels.size()
             <<",\"retraces\":"<<retraces<<",\"mean_per_visit\":"<<e.line_mean_residual_px_[edge]
             <<",\"RMS_per_visit\":"<<std::sqrt(squared/arc.size())
             <<",\"mean_unique_same_line\":"<<unique_same/pixels.size()
             <<",\"mean_unique_refit\":"<<unique_refit/pixels.size()<<",\"support\":[";
          for(size_t k=0;k<arc.size();++k){if(k)std::cout<<',';std::cout<<'['<<arc[k].x<<','<<arc[k].y<<']';}
          std::cout<<"]}";
        }
        std::cout<<"],\"intersection\":["<<e.intersection_.x<<','<<e.intersection_.y
                 <<"],\"corner_error\":"<<e.corner_error_px_<<",\"extension\":["<<e.support_extension_px_[0]<<','<<e.support_extension_px_[1]<<']';
      }else std::cout<<']';
      std::cout<<'}'<<std::endl;
    }
    return 0;
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
