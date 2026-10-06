// 边拟合独立反例：真实相切2px圆弧、真正近退化角/反向表示；不使用95°假门限。
#include "block3_fixture.hpp"
#include "corners/corner_edge_fit.hpp"
#include "corners/corner_evidence_validation.hpp"
#include "core/observed_geometry_utils.hpp"
#include <iostream>
#include <stdexcept>
#include <fstream>
namespace {
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
// 将连续圆弧栅格化为真实图像，轮廓提取仍保留所有像素，不使用blur/形态学圆角。
void roundedCorner() {
    std::vector<cv::Point> polygon;
    for(int step=0;step<=32;++step){double a=CV_PI+CV_PI/2*step/32;polygon.emplace_back(cv::Point2d(102+2*std::cos(a),102+2*std::sin(a)));}
    polygon.insert(polygon.end(),{{130,100},{130,130},{100,130}});
    cv::Mat image=cv::Mat::zeros(200,200,CV_8UC1);cv::fillPoly(image,std::vector<std::vector<cv::Point>>{polygon},cv::Scalar(255));
    std::vector<std::vector<cv::Point>> contours;cv::findContours(image,contours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_NONE);
    std::array<std::array<cv::Point2d,2>,2> edges{{{cv::Point2d(100,130),cv::Point2d(100,100)},{cv::Point2d(100,100),cv::Point2d(130,100)}}};
    auto result=mark::fitObservedEdgePair(contours.at(0),edges,fixture::cornerConfig());
    check(bool(result.evidence),result.reason.c_str());check(cv::norm(result.evidence->intersection_-cv::Point2d(100,100))<1,"rounded nominal intersection wrong");
    check(result.evidence->original_turn_arc_.size()>2,"rounded connector not measured");
}
// 同一无向夹角对0°和180°表示一致；真实小角不能误认为安全大夹角。
void parallelEdges() {
    cv::Point2d a{1,0},b{1,0.001};
    check(mark::observed::angleDeg(a,b)<1&&mark::observed::angleDeg(a,-b)<1,"anti-parallel not folded to acute angle");
    cv::Mat image=cv::Mat::zeros(200,300,CV_8UC1);
    std::vector<cv::Point> polygon{{20,20},{120,20},{220,21},{220,70},{20,70}};
    cv::fillPoly(image,std::vector<std::vector<cv::Point>>{polygon},cv::Scalar(255));
    std::vector<std::vector<cv::Point>> contours;cv::findContours(image,contours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_NONE);
    std::array<std::array<cv::Point2d,2>,2> edges{{{cv::Point2d(20,20),cv::Point2d(120,20)},{cv::Point2d(120,20),cv::Point2d(220,21)}}};
    auto config=fixture::cornerConfig();config.approximation_epsilon_=0.1;
    auto result=mark::fitObservedEdgePair(contours.at(0),edges,config);
    check(!result.evidence&&!result.reason.empty(),"nearly parallel pair produced valid corner");
}
// 两条长直边拟合完美，但被大倒角隔开，交点远离真实转折，不能补名义角。
void remoteIntersection() {
    cv::Mat image=cv::Mat::zeros(300,300,CV_8UC1);
    cv::fillPoly(image,std::vector<std::vector<cv::Point>>{{{150,100},{200,100},{200,200},{100,200},{100,150}}},cv::Scalar(255));
    std::vector<std::vector<cv::Point>> contours;cv::findContours(image,contours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_NONE);
    std::array<std::array<cv::Point2d,2>,2> edges{{{cv::Point2d(100,200),cv::Point2d(100,100)},{cv::Point2d(100,100),cv::Point2d(200,100)}}};
    auto config=fixture::cornerConfig();config.observation_budget_->max_support_extension_px=100;
    config.observation_budget_->max_turn_connection_length_px=100;
    auto result=mark::fitObservedEdgePair(contours[0],edges,config);
    check(!result.evidence,"remote infinite-line intersection published");
}
// 已保留H-v1失败数据；旧单简化段实现会断开直边末端，预算完全保持冻结v1。
void splitRoundedBoundary() {
    std::ifstream source(std::filesystem::path(__FILE__).parent_path()/"data/rounded_split_boundary.txt");
    check(bool(source),"rounded regression fixture missing");cv::Point2d expected;
    source>>expected.x>>expected.y;std::array<std::array<cv::Point2d,2>,2> edges{};
    for(auto& edge:edges)for(auto& point:edge)source>>point.x>>point.y;
    std::vector<cv::Point> pixels;double x,y;while(source>>x>>y)pixels.emplace_back(cv::Point2d(x,y));
    cv::Mat image=cv::Mat::zeros(1080,1440,CV_8UC1);cv::fillPoly(image,std::vector<std::vector<cv::Point>>{pixels},cv::Scalar(255));
    std::vector<std::vector<cv::Point>> contours;cv::findContours(image,contours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_NONE);
    auto c=fixture::cornerConfig();c.min_line_points_=10;c.max_line_fit_error_=0.5;c.max_corner_error_=1.5;
    c.observation_budget_=mark::CornerObservationBudget{1,2,4.5,2,13.5,9.5,14};
    auto result=mark::fitObservedEdgePair(contours.at(0),edges,c);
    check(bool(result.evidence),result.reason.c_str());check(cv::norm(result.evidence->intersection_-expected)<=2,"split boundary truth error exceeded");
}
// 指定外边被大量小简化段拆分；完整连续支持必须覆盖两段以上，不能拼非连续点。
void manySimplifiedSegments() {
    std::vector<cv::Point> polygon;
    for(int x=100;x<=160;++x)polygon.emplace_back(x,100+(x%4==2));
    polygon.insert(polygon.end(),{{160,160},{100,160}});
    cv::Mat image=cv::Mat::zeros(240,240,CV_8UC1);
    cv::fillPoly(image,std::vector<std::vector<cv::Point>>{polygon},cv::Scalar(255));
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(image,contours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_NONE);
    auto c=fixture::cornerConfig();c.approximation_epsilon_=0.1;c.min_line_points_=10;
    c.observation_budget_->min_support_span_px=14;
    std::vector<cv::Point> simplified;cv::approxPolyDP(contours[0],simplified,c.approximation_epsilon_,true);
    check(simplified.size()>30,"fragmentation fixture lost simplified vertices");
    std::array<std::array<cv::Point2d,2>,2> edges{{{cv::Point2d(100,160),cv::Point2d(100,100)},
        {cv::Point2d(100,100),cv::Point2d(160,100)}}};
    auto result=mark::fitObservedEdgePair(contours[0],edges,c);
    check(bool(result.evidence),result.reason.c_str());
    check(cv::norm(result.evidence->intersection_-cv::Point2d(100,100))<1,"fragmented edge intersection wrong");
}
// 视频第1帧真实阈值轮廓含51个简化顶点，外边不能受1～2段候选上限阻断。
void fragmentedVideoBoundary() {
    std::ifstream source(std::filesystem::path(__FILE__).parent_path()/"data/video_fragmented_boundary.txt");
    check(bool(source),"video regression fixture missing");
    std::array<std::array<cv::Point2d,2>,2> edges{};
    for(auto& edge:edges)for(auto& point:edge)source>>point.x>>point.y;
    std::vector<cv::Point> contour;int x,y;while(source>>x>>y)contour.emplace_back(x,y);
    auto c=fixture::cornerConfig();c.min_line_points_=10;c.max_line_fit_error_=0.5;c.max_corner_error_=1.5;
    c.observation_budget_=mark::CornerObservationBudget{3,9,4.5,2,13.5,9.5,14};
    auto result=mark::fitObservedEdgePair(contour,edges,c);
    check(bool(result.evidence),result.reason.c_str());
    check(result.evidence->corner_error_px_<=1.5,"video corner gate changed");
    // 真实弧证据送进生产复算校验，防止只通过局部fit却违反连续性或端点绑定。
    auto evidence=*result.evidence;evidence.stable_id_="frame-1/M1";evidence.component_id_=7;
    evidence.physical_corner_=static_cast<mark::PhysicalCorner>(1);
    evidence.model_vertex_id_=1;evidence.model_edge_ids_={0,1};std::string reason;
    check(mark::validateCornerEvidence(evidence,evidence.intersection_,1,c,reason),reason.c_str());
    for(auto& arc:result.evidence->original_support_arcs_)
        for(auto p:arc)check(std::find(contour.begin(),contour.end(),cv::Point(p))!=contour.end(),"fabricated video support");
}

}
int main(){int failures=0;for(auto item:{std::pair<const char*,void(*)()>{"RoundedCornerDoesNotCrash",roundedCorner},
  {"ParallelEdgesFail",parallelEdges},{"RemoteIntersectionFails",remoteIntersection},{"SplitRoundedBoundary",splitRoundedBoundary},{"FragmentedVideoBoundary",fragmentedVideoBoundary},{"ManySimplifiedSegments",manySimplifiedSegments}}){try{item.second();std::cout<<"PASS "<<item.first<<'\n';}catch(const std::exception& e){++failures;std::cerr<<"FAIL "<<item.first<<": "<<e.what()<<'\n';}}return failures?1:0;}
