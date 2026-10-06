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
#include "instrumented_edge_fit.cpp"

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
      fix2::metrics={};auto instrumented=mark::auditObservedEdgePair(contour.contour,edges,config);
      if(baseline.reason!=instrumented.reason || bool(baseline.evidence)!=bool(instrumented.evidence))
        throw std::runtime_error("instrumentation changed baseline");
      auto m=fix2::metrics;
      std::cout<<std::setprecision(17)<<"{\"frame_id\":"<<id<<",\"hypothesis\":"<<h<<",\"corner\":"<<p
       <<",\"component_id\":"<<cid<<",\"baseline_success\":"<<(baseline.evidence?"true":"false")
       <<",\"reason\":"<<quoted(baseline.reason)<<",\"contour_points\":"<<contour.contour.size()
       <<",\"arc_masks\":["<<m.mask1<<','<<m.mask2<<','<<m.mask3<<"],\"pre_final_gate_pairs\":"<<m.pairs
       <<",\"corner_only_fail_pairs\":"<<m.corner_only<<",\"extension_only_fail_pairs\":"<<m.extension_only
       <<",\"both_fail_pairs\":"<<m.both<<",\"best_final_gate_score\":";
      if(std::isfinite(m.best_score))std::cout<<m.best_score;else std::cout<<"null";
      std::cout<<",\"best_pair\":{\"corner_error\":"<<m.corner<<",\"extension\":["<<m.ea<<','<<m.eb
       <<"],\"connection_length\":"<<m.connection<<",\"intersection\":["<<m.x<<','<<m.y
       <<"],\"mean_residuals\":["<<m.residual_a<<','<<m.residual_b<<"]},\"sensitivity\":[";
      // Offline one-variable probes only; never write a production configuration.
      const std::vector<std::string> variants={"residual_0.75","residual_1.0","span_10","direction_6",
          "position_15","connection_20","extension_15","trim_3","residual_connection_extension","all_wide",
          "position_9.25","position_9.5","position_10","position_11","position_12","residual_0.75_position_10"};
      bool comma=false;
      for(const auto& name:variants) {
        auto probe=config;
        if(name=="residual_0.75")probe.max_line_fit_error_=0.75;
        if(name=="residual_1.0")probe.max_line_fit_error_=1.0;
        if(name=="span_10")probe.observation_budget_->min_support_span_px=10;
        if(name=="direction_6")probe.observation_budget_->max_edge_direction_diff_deg=6;
        if(name=="position_15")probe.observation_budget_->max_edge_position_distance_px=15;
        if(name=="position_9.25")probe.observation_budget_->max_edge_position_distance_px=9.25;
        if(name=="position_9.5")probe.observation_budget_->max_edge_position_distance_px=9.5;
        if(name=="position_10")probe.observation_budget_->max_edge_position_distance_px=10;
        if(name=="position_11")probe.observation_budget_->max_edge_position_distance_px=11;
        if(name=="position_12")probe.observation_budget_->max_edge_position_distance_px=12;
        if(name=="residual_0.75_position_10") {
          probe.max_line_fit_error_=0.75;
          probe.observation_budget_->max_edge_position_distance_px=10;
        }
        if(name=="connection_20")probe.observation_budget_->max_turn_connection_length_px=20;
        if(name=="extension_15")probe.observation_budget_->max_support_extension_px=15;
        if(name=="trim_3")probe.observation_budget_->turn_trim_distance_px=3;
        if(name=="residual_connection_extension"||name=="all_wide") {
          probe.max_line_fit_error_=0.75;
          probe.observation_budget_->max_turn_connection_length_px=20;
          probe.observation_budget_->max_support_extension_px=15;
        }
        if(name=="all_wide") {
          probe.max_line_fit_error_=1;
          probe.observation_budget_->min_support_span_px=10;
          probe.observation_budget_->max_edge_direction_diff_deg=6;
          probe.observation_budget_->max_edge_position_distance_px=15;
        }
        auto result=mark::fitObservedEdgePair(contour.contour,edges,probe);
        if(comma)std::cout<<',';comma=true;
        std::cout<<"{\"variant\":"<<quoted(name)<<",\"success\":"<<(result.evidence?"true":"false")
                 <<",\"reason\":"<<quoted(result.reason);
        if(result.evidence) {
          const auto& e=*result.evidence;
          std::array<double,2> max_position{0,0},direction;
          for(int edge=0;edge<2;++edge) {
            for(auto point:e.original_support_arcs_[edge])
              max_position[edge]=std::max(max_position[edge],mark::observed::segmentDistance(point,edges[edge][0],edges[edge][1]));
            auto line=edge?e.line_b_:e.line_a_;
            direction[edge]=mark::observed::angleDeg({line[0],line[1]},edges[edge][1]-edges[edge][0]);
          }
          std::cout<<",\"intersection\":["<<e.intersection_.x<<','<<e.intersection_.y
                   <<"],\"corner_error\":"<<e.corner_error_px_
                   <<",\"mean_residuals\":["<<e.line_mean_residual_px_[0]<<','<<e.line_mean_residual_px_[1]
                   <<"],\"extension\":["<<e.support_extension_px_[0]<<','<<e.support_extension_px_[1]
                   <<"],\"max_model_edge_distance\":["<<max_position[0]<<','<<max_position[1]
                   <<"],\"direction_difference_deg\":["<<direction[0]<<','<<direction[1]<<']';
        }
        std::cout<<'}';
      }
      std::cout<<"]}"<<std::endl;
    }
    return 0;
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
