// 诊断复现：正式前半链路先运行；逐组件 raw metrics 不改变生产配置，不用于 truth 输入。
#include "config.hpp"
#include "preprocess.hpp"
#include "geometry_observation.hpp"
#include "geometry_matcher.hpp"
#include "geometry_validation.hpp"
#include "geometry_assignment_completion.hpp"
#ifdef MS_BASELINE_HEADER
#include MS_BASELINE_HEADER
#else
#include "assignment_match_metrics.hpp"
#endif
#include <nlohmann/json.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <fstream>
#include <iostream>
#include <set>
using J=nlohmann::json;
J diag(const cv::Mat& im,int width,const mark::DetectorConfig& base,const mark::MarkerGeometry& model) {
 auto cfg=base;cfg.preprocess.work_width=width;cfg.preprocess.work_height=width*3/4;
 mark::FrameInput input{};input.image=im;auto f=mark::preprocess(input,cfg.preprocess);f.components_=mark::extractWhiteComponents(f,cfg.geometry_);
 auto obs=mark::observeShapes(f.components_,cfg.geometry_);auto gen=mark::generateGeometryHypotheses(obs,model,cfg.geometry_);auto val=mark::validateGeometryBatch(gen,model,f.components_,cfg.geometry_);
 auto complete=mark::completeSegmentedAssignments(val,f.components_,model,*cfg.assignment_completion_);
 J j={{"work_width",width},{"generated",gen.hypotheses_.size()},{"validated",val.hypotheses_.size()},{"completed",complete.hypotheses_.size()}};
 for(auto& c:f.components_) {J p=J::array();for(auto pt:c.contour_)p.push_back({pt.x,pt.y});j["components"].push_back({{"id",c.component_id_},{"area",c.area_},{"contour",p}});}
 for(auto& h:gen.hypotheses_){J parent;parent["matrix"]=J::array();for(int a=0;a<2;++a)for(int b=0;b<3;++b)parent["matrix"].push_back(h.affine_transform_.at<double>(a,b));
 for(auto as:h.assignments_){const mark::GeometryPolygon* p=nullptr;for(auto& part:model.polygons)if(part.id==as.model_part_id_)p=&part;auto& c=f.components_.at(as.component_id_);J distances=J::array();for(auto pt:p->vertices)distances.push_back(mark::observed::boundaryDistance(mark::observed::project(h.affine_transform_,pt),std::vector<cv::Point2d>(c.contour_.begin(),c.contour_.end())));parent["assignments"].push_back({{"part",as.model_part_id_},{"component",as.component_id_},{"vertex_distances",distances}});}
 j["parents"].push_back(parent);}
 for(size_t hi=0;hi<val.hypotheses_.size();++hi){auto& h=val.hypotheses_[hi];std::vector<double> areas;std::set<size_t> used;for(auto as:h.assignments_){areas.push_back(f.components_.at(as.component_id_).area_);used.insert(as.component_id_);}std::sort(areas.begin(),areas.end());
 for(auto& part:model.polygons)if(part.id=="M1"||part.id=="S1a"||part.id=="S1b")for(auto& c:f.components_)if(!used.count(c.component_id_)){auto m=mark::measureAssignmentMatch(c,part,h.affine_transform_,areas[1],416,3,1);J row={{"parent",hi},{"part",part.id},{"component",c.component_id_},{"boundary",m.boundary_distance},{"area",m.relative_area_error},{"topology",m.topology_valid},{"direction",m.direction_diff}};J counts=J::array();for(int k=100;k<=300;++k){std::vector<cv::Point> p;cv::approxPolyDP(c.contour_,p,k/100.,true);if(p.size()==part.vertices.size())counts.push_back(k/100.);}row["matching_vertex_epsilons"]=counts;j["metrics"].push_back(row);}}
 return j;
}
int main(int argc,char**argv){auto cfg=mark::loadConfig(argv[1]).detector_config;auto model=mark::loadMarkerGeometry(cfg.marker_geometry_path_);std::ifstream in(argv[2]);std::ofstream out(argv[3]);std::string s;cv::VideoCapture cap;
 while(std::getline(in,s)){auto r=J::parse(s);cv::Mat im;if(r.contains("case_id"))im=cv::imread("build/block3-H-v1/"+r["case_id"].get<std::string>()+".png");else{if(!cap.isOpened())cap.open("data/raw/marker_video.avi");cap.set(cv::CAP_PROP_POS_FRAMES,r["frame_id"].get<int>());cap.read(im);}auto d=diag(im,r.value("work_width",960),cfg,model);if(r.contains("case_id"))d["case_id"]=r["case_id"];else d["frame_id"]=r["frame_id"];out<<d.dump()<<'\n';}
}
