// 独立读原始报告并按冻结公式对照；不调用生产验证 helper、不影响检测。
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <map>
#include <iostream>
#include <cmath>
using Json=nlohmann::json;
int main(int argc,char** argv){try{
 if(argc!=6)throw std::runtime_error("usage: BEFORE AFTER MODEL ROWS SUMMARY");
 cv::FileStorage model(argv[3],cv::FileStorage::READ);if(!model.isOpened())throw std::runtime_error("model missing");
 std::map<std::string,std::vector<cv::Point2f>> parts;
 for(auto p:model["polygons"]){std::string id=(std::string)p["id"];for(auto xy:p["vertices"])parts[id].emplace_back((float)xy[0],(float)xy[1]);}
 std::ifstream before(argv[1]),after(argv[2]);std::ofstream rows(argv[4]);std::string a,b;
 size_t frames=0,validated=0,generated=0,completed=0,changed=0;double largest=0,worst=0;
 while(std::getline(before,a)){
  if(!std::getline(after,b))throw std::runtime_error("missing after frame");
  auto old=Json::parse(a),now=Json::parse(b);if(old["frame_id"]!=now["frame_id"])throw std::runtime_error("frame mismatch");++frames;
  auto od=old["details"],nd=now["details"];if(od.is_null()||nd.is_null())throw std::runtime_error("details missing");
  std::map<std::string,std::vector<cv::Point2f>> contours;
  for(auto c:nd["components"])for(auto q:c["contour"])contours[c["component_id"].get<std::string>()].emplace_back(q[0].get<float>(),q[1].get<float>());
  for(auto h:nd["generated"]["hypotheses"]){if(h["validation_residual"]!=-1.)throw std::runtime_error("unmeasured marker mismatch");++generated;}
  auto previous=od["validated"]["hypotheses"],current=nd["validated"]["hypotheses"];
  if(previous.size()!=current.size())throw std::runtime_error("validated count changed");
  for(size_t k=0;k<current.size();++k){
   auto h=current[k],o=previous[k];cv::Mat affine(2,3,CV_64F);for(int i=0;i<6;++i)affine.at<double>(i/3,i%3)=h["affine"][i].get<double>();
   Json values=Json::array();double maximum=0;
   for(auto assignment:h["assignments"]){
    auto id=assignment["component_id"].get<std::string>();auto part=assignment["model_part_id"].get<std::string>();
    std::vector<cv::Point2f> projected;cv::transform(parts.at(part),projected,affine);double sum=0;
    for(auto point:projected)sum+=std::abs(cv::pointPolygonTest(contours.at(id),point,true));
    double mean=sum/projected.size();maximum=std::max(maximum,mean);values.push_back({{"model_part_id",part},{"component_id",id},{"mean",mean}});
   }
   double field=h["validation_residual"].get<double>(),difference=std::abs(field-maximum);largest=std::max(largest,maximum);worst=std::max(worst,difference);
   if(!std::isfinite(field)||field<0||difference>1e-12)throw std::runtime_error("actual residual mismatch");
   changed+=o["validation_residual"]!=h["validation_residual"];++validated;
   rows<<Json{{"frame_id",now["frame_id"]},{"hypothesis_id",h["id"]},{"part_means",values},{"independent_max",maximum},{"before_field",o["validation_residual"]},{"after_field",field},{"absolute_difference",difference},{"unit","working_image_px"}}.dump()<<'\n';
   h.erase("validation_residual");o.erase("validation_residual");if(h!=o)throw std::runtime_error("other parent fields changed");
  }
  for(auto h:nd["completed"]["hypotheses"]){bool found=false;
   for(auto parent:current){auto assignments=h["assignments"];assignments.erase(assignments.begin()+parent["assignments"].size(),assignments.end());
    if(assignments==parent["assignments"]&&h["affine"]==parent["affine"]&&h["validation_residual"]==parent["validation_residual"])found=true;}
   if(!found)throw std::runtime_error("completed hypothesis lost parent residual");++completed;
  }
 }
 if(std::getline(after,b)||frames!=1676)throw std::runtime_error("frame denominator mismatch");
 Json summary={{"frames",frames},{"generated_unmeasured_checked",generated},{"validated_parents_checked",validated},{"completed_parent_scope_checked",completed},{"changed_residual_fields",changed},{"maximum_working_px",largest},{"maximum_independent_difference",worst},{"pass",true}};
 std::ofstream(argv[5])<<summary.dump(2)<<'\n';std::cout<<summary.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
