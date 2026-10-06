// 输入固定视频/配置，调用唯一runDecodePipeline；输出逐帧JSONL和摘要到stderr。
// 阶段status与公共NOT_READY分开记录；没有真值时定位误差为null，不写0。
#include "config/config.hpp"
#include "pipeline/decode_stage.hpp"
#include "common/file_digest.hpp"
#include <opencv2/videoio.hpp>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <set>
#include <sstream>
#include <chrono>
namespace {
// JSON字符串转义防止中文原因/路径中的引号破坏逐帧记录。
std::string quote(const std::string& value) {
    std::ostringstream out;out<<'"';for(unsigned char c:value) {
        if(c=='"'||c=='\\') out<<'\\'<<c;
        else if(c<32) out<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<int(c);
        else out<<c;
    }out<<'"';return out.str();
}
void point(cv::Point2d p) {std::cout<<'['<<p.x<<','<<p.y<<']';}
// 记录所有成功测量原始证据；拒绝原因仍在diagnostics，不用empty吞模块失败。
void record(uint64_t id,int64_t timestamp,const mark::DecodeStageResult& r,const std::string& input_hash,const std::string& config_hash,double stage_ms) {
    std::cout<<std::setprecision(17)<<"{\"frame_id\":"<<id<<",\"timestamp_us\":"<<timestamp
      <<",\"stage\":\"decode\",\"status\":"<<int(r.status)<<",\"public_status\":\"NOT_READY\",\"input_sha256\":"<<quote(input_hash)
      <<",\"stage_ms\":"<<stage_ms<<",\"config_sha256\":"<<quote(config_hash)<<",\"commit\":"<<quote(MARK_COMMIT)
      <<",\"code_sha256\":"<<quote(MARK_CODE_HASH)<<",\"search_truncated\":"<<(r.search_truncated?"true":"false")<<",\"detections\":[";
    for(size_t d=0;d<r.detections.size();++d) {
        if(d) std::cout<<',';const auto& detection=r.detections[d];std::cout<<"{\"corners\":[";
        for(int i=0;i<4;++i){if(i)std::cout<<',';point(detection.corners[i]);}
        const auto& b=detection.bbox;std::cout<<"],\"bbox\":["<<b.x<<','<<b.y<<','<<b.width<<','<<b.height<<"],\"orientation\":";
        if(detection.attributes.orientation){std::cout<<'[';for(int i=0;i<4;++i){if(i)std::cout<<',';std::cout<<(*detection.attributes.orientation)[i];}std::cout<<']';}
        else std::cout<<"null";std::cout<<",\"marker_code\":null,\"truth_error_px\":null}";
    }
    std::cout<<"],\"measurements\":[";
    for(size_t m=0;m<r.measurements.size();++m) {
        if(m) std::cout<<',';std::cout<<'[';
        for(int c=0;c<4;++c){if(c)std::cout<<',';const auto& e=r.measurements[m].evidence_[c];
            std::cout<<"{\"physical\":"<<c<<",\"frame_id\":"<<e.frame_id_<<",\"component_id\":"<<e.component_id_
              <<",\"model_vertex_id\":"<<e.model_vertex_id_<<",\"model_edge_ids\":["<<e.model_edge_ids_[0]<<','<<e.model_edge_ids_[1]
              <<"],\"evidence_id\":"<<quote(e.stable_id_)<<",\"original_observation\":"<<(e.original_observation_?"true":"false")
              <<",\"truncated\":"<<(e.truncated_?"true":"false")<<",\"observed_segment_ids\":[";
            for(size_t i=0;i<e.observed_segment_ids_.size();++i){if(i)std::cout<<',';std::cout<<e.observed_segment_ids_[i];}
            std::cout<<"],\"observed_segment_end_ids\":["<<e.observed_segment_end_ids_[0]<<','<<e.observed_segment_end_ids_[1]<<"],\"fitted_lines\":[";
            for(int a=0;a<2;++a){if(a)std::cout<<',';auto fit=a==0?e.line_a_:e.line_b_;std::cout<<'[';for(int i=0;i<4;++i){if(i)std::cout<<',';std::cout<<fit[i];}std::cout<<']';}
            std::cout<<"],\"finite_segments\":[";
            for(int a=0;a<2;++a){if(a)std::cout<<',';const auto& segment=a==0?e.edge_segment_a_:e.edge_segment_b_;std::cout<<'[';point(segment[0]);std::cout<<',';point(segment[1]);std::cout<<']';}
            std::cout<<"],\"intersection\":";point(e.intersection_);
            std::cout<<",\"line_mean_residual_px\":["<<e.line_mean_residual_px_[0]<<','<<e.line_mean_residual_px_[1]<<"],\"line_max_residual_px\":["<<e.line_max_residual_px_[0]<<','<<e.line_max_residual_px_[1]
              <<"],\"support_extension_px\":["<<e.support_extension_px_[0]<<','<<e.support_extension_px_[1]<<"],\"corner_error_px\":"<<e.corner_error_px_<<",\"support_arcs\":[";
            for(int a=0;a<2;++a){if(a)std::cout<<',';std::cout<<'[';for(size_t p=0;p<e.original_support_arcs_[a].size();++p){if(p)std::cout<<',';point(e.original_support_arcs_[a][p]);}std::cout<<']';}
            std::cout<<"],\"turn_arc\":[";for(size_t p=0;p<e.original_turn_arc_.size();++p){if(p)std::cout<<',';point(e.original_turn_arc_[p]);}std::cout<<"]}";
        }std::cout<<']';
    }
    std::cout<<"],\"diagnostics\":[";for(size_t i=0;i<r.diagnostics.size();++i){if(i)std::cout<<',';std::cout<<quote(r.diagnostics[i]);}std::cout<<"]}\n";
}
}
int main(int argc,char** argv) {
    try {
        if(argc<2||argc>4) throw std::invalid_argument("用法: decode_audit <视频> [帧号列表或all] [配置]");
        std::set<int> selected;
        if(argc>=3&&std::string(argv[2])!="all") {std::istringstream ss(argv[2]);std::string item;
            while(std::getline(ss,item,',')){size_t end=0;int n=std::stoi(item,&end);if(n<0||end!=item.size())throw std::invalid_argument("invalid frame list");selected.insert(n);}}
        std::string config_path=argc==4?argv[3]:MARK_DEFAULT_CONFIG;
        auto config=mark::loadConfig(config_path).detector_config;auto model=mark::loadMarkerGeometry(config.marker_geometry_path_);
        auto input_hash=audit::sha256(argv[1]),config_hash=audit::sha256(config_path);
        cv::VideoCapture capture(argv[1]);if(!capture.isOpened())throw std::runtime_error("cannot open video");
        double fps=capture.get(cv::CAP_PROP_FPS);if(!std::isfinite(fps)||fps<=0)throw std::runtime_error("invalid fps");
        uint64_t id=0,records=0,detected=0;cv::Mat image;auto pending=selected;
        while(capture.read(image)) {
            if(selected.empty()||selected.count(static_cast<int>(id))) {
                mark::FrameInput input{};input.image=image;input.frame_id=id;input.timestamp_us=static_cast<int64_t>(std::llround(id*1e6/fps));input.time_source=mark::TimestampSource::Unknown;
                auto begin=std::chrono::steady_clock::now();auto result=mark::runDecodePipeline(input,config,model);
                double stage_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
                record(id,input.timestamp_us,result,input_hash,config_hash,stage_ms);
                ++records;if(!result.detections.empty())++detected;pending.erase(static_cast<int>(id));
            }++id;
        }
        std::cerr<<"decoded_frames="<<id<<" records="<<records<<" stage_detected="<<detected<<" public_status=NOT_READY\n";
        if(!pending.empty()) throw std::runtime_error("requested frame missing");
        return 0;
    } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
