// 一次decode后共享装配；实验预算只读独立配置，不替换生产YAML。
#include "common/temporal_record.hpp"
#include "common/file_digest.hpp"
#include "config/config.hpp"
#include "corners/screen_order.hpp"
#include <opencv2/videoio.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <cmath>
#include <limits>
namespace {
using namespace mark;
// 固定实验网格直接喂可信几何，不将真值传正式Detector；扰动复用C记录。
Detection measured(const std::array<cv::Point2d,4>& physical,bool known) {
    auto rounded=physical;for(auto& p:rounded)p=cv::Point2f(p);
    std::string reason;auto order=orderScreenCycle(rounded,{},reason);
    if(!order)throw std::runtime_error("EXPERIMENT_INPUT_INVALID: "+reason);
    Detection d{};for(size_t i=0;i<4;++i)d.corners[i]=order->screen_points[i];
    d.bbox=boundingBoxFromCorners(d.corners);if(known)d.attributes.orientation=order->input_to_screen;return d;
}
void experiment(std::ostream& out,TemporalConfig config,const std::string& noise_path,const std::string& provenance) {
    std::ifstream in(noise_path);if(!in)throw std::runtime_error("cannot open noise CSV");
    std::string line;std::getline(in,line);std::vector<std::array<cv::Point2d,4>> noise;
    while(std::getline(in,line)) {
        std::istringstream s(line);std::string v;std::getline(s,v,',');std::array<cv::Point2d,4> n{};
        for(auto& p:n){if(!std::getline(s,v,','))throw std::runtime_error("noise columns missing");p.x=std::stod(v);if(!std::getline(s,v,','))throw std::runtime_error("noise columns missing");p.y=std::stod(v);if(!std::isfinite(p.x)||!std::isfinite(p.y))throw std::runtime_error("noise nonfinite");}
        noise.push_back(n);
    }
    if(noise.size()<32)throw std::runtime_error("requires >=32 recorded C errors");
    uint64_t segments=0;
    for(int dt:{7,14,28})for(double budget:{.5,1.,2.,4.})for(bool known:{false,true})for(bool noisy:{false,true})for(int mode:{0,1}) {
        std::vector<double> rates=mode?std::vector<double>{0,.5,2,5}:std::vector<double>{0,.5,1,2,4,8};
        for(double rate:rates) {
            auto c=config;c.stabilization_enabled=true;c.max_smoothing_deviation_px=budget;TemporalStabilizer t(c);
            for(int n=0;n<32;++n) {
                double angle=mode?n*rate*CV_PI/180:0,dx=mode?0:n*rate,cs=std::cos(angle),sn=std::sin(angle);
                std::array<cv::Point2d,4> truth{{{-50,-50},{50,-50},{50,50},{-50,50}}};
                for(auto& p:truth)p={720+dx+cs*p.x-sn*p.y,540+sn*p.x+cs*p.y};
                auto p=truth;if(noisy)for(size_t i=0;i<4;++i)p[i]+=noise[n][i];
                auto raw=measured(p,known);auto r=t.update({raw},FrameStamp{uint64_t(n),int64_t(n*dt*1000),TimestampSource::Unknown},{1440,1080});
                out<<"{"<<provenance<<",\"segment\":"<<segments<<",\"sample\":"<<n<<",\"dt_ms\":"<<dt<<",\"mode\":"<<audit::quote(mode?"rotation":"translation")<<",\"rate\":"<<rate
                   <<",\"known\":"<<(known?"true":"false")<<",\"noisy\":"<<(noisy?"true":"false")<<",\"r_experimental_px\":"<<*c.correspondence_uncertainty_px
                   <<",\"deviation_experimental_px\":"<<budget<<",\"truth_physical\":[";
                for(size_t i=0;i<4;++i){if(i)out<<',';audit::point(out,truth[i]);}
                out<<"],\"raw_physical\":[";for(size_t i=0;i<4;++i){if(i)out<<',';audit::point(out,cv::Point2f(p[i]));}
                out<<"],\"raw\":";audit::detection(out,raw);out<<",\"result\":";audit::frameResult(out,FrameResult{uint64_t(n),int64_t(n*dt*1000),r.status,{raw},r.tracks,std::nullopt,{}});
                out<<",\"temporal\":";audit::temporalDiagnostics(out,r.diagnostics);out<<"}\n";
            }++segments;
        }
    }
    std::cerr<<"experimental_segments="<<segments<<" records="<<segments*32<<'\n';
}
}
int main(int argc,char** argv) {
    try {
        std::map<std::string,std::string> args;bool overwrite=false,grid=false;
        for(int i=1;i<argc;++i) {
            std::string key=argv[i];if(key=="--overwrite"){if(overwrite)throw std::runtime_error("duplicate overwrite");overwrite=true;continue;}
            if(key=="--experiment-grid"){if(grid)throw std::runtime_error("duplicate experiment mode");grid=true;continue;}
            if(key!="--video"&&key!="--config"&&key!="--output"&&key!="--noise-csv")throw std::runtime_error("unknown option: "+key);
            if(i+1>=argc||!args.emplace(key,argv[++i]).second)throw std::runtime_error("missing value/duplicate option");
        }
        for(const char* key:{"--config","--output"})if(!args.count(key)||args[key].empty())throw std::runtime_error(std::string("required ")+key);
        if(grid?(!args.count("--noise-csv")||args.count("--video")):(!args.count("--video")||args.count("--noise-csv")))
            throw std::runtime_error("video mode: --video --config --output [--overwrite]; experiment: --experiment-grid --noise-csv --config --output");
        namespace fs=std::filesystem;fs::path output=args["--output"],effective=output.string()+".effective.yaml";
        if(!overwrite&&(fs::exists(output)||fs::exists(effective)))throw std::runtime_error("output exists; require --overwrite");
        auto app=mark::loadConfig(args["--config"]);auto config=app.detector_config;
        auto model=mark::loadMarkerGeometry(config.marker_geometry_path_);
        if(!output.parent_path().empty())fs::create_directories(output.parent_path());
        mark::writeEffectiveConfig(app,effective);std::ofstream out(output);if(!out)throw std::runtime_error("cannot write output");out<<std::setprecision(17);
        if(grid) {
            if(!config.temporal.correspondence_uncertainty_px)throw std::runtime_error("experiment r missing");
            const std::string provenance="\"experimental\":true,\"config_sha256\":"+audit::quote(audit::sha256(args["--config"]))+
                ",\"effective_config_sha256\":"+audit::quote(audit::sha256(effective))+
                ",\"noise_sha256\":"+audit::quote(audit::sha256(args["--noise-csv"]))+
                ",\"model_sha256\":"+audit::quote(audit::sha256(config.marker_geometry_path_))+
                ",\"code_sha256\":"+audit::quote(MARK_CODE_HASH)+",\"commit_label\":"+audit::quote(MARK_COMMIT);
            experiment(out,config.temporal,args["--noise-csv"],provenance);
        } else {
            const bool ready=config.assignment_completion_&&config.corner_.observation_budget_&&config.temporal.correspondence_uncertainty_px&&config.temporal.max_smoothing_deviation_px;
            std::unique_ptr<TemporalStabilizer> temporal;if(ready)temporal=std::make_unique<TemporalStabilizer>(config.temporal);
            DisplayHistory display({config.temporal.display_hold_enabled,config.temporal.max_hold_frames});
            auto ih=audit::sha256(args["--video"]),ch=audit::sha256(args["--config"]),eh=audit::sha256(effective),mh=audit::sha256(config.marker_geometry_path_);
            cv::VideoCapture capture(args["--video"]);if(!capture.isOpened())throw std::runtime_error("cannot open video");
            double fps=capture.get(cv::CAP_PROP_FPS);if(!std::isfinite(fps)||fps<=0)throw std::runtime_error("invalid fps");
            cv::Mat image;uint64_t id=0,raw_count=0,tracks=0;
            while(capture.read(image)) {
                double time=double(id)*1e6/fps;if(time>=double(std::numeric_limits<int64_t>::max()))throw std::runtime_error("timestamp overflow");
                FrameInput input{image,id,static_cast<int64_t>(std::llround(time)),TimestampSource::Unknown};
                FrameStamp stamp{id,input.timestamp_us,input.time_source};
                auto decoded=runDecodePipeline(input,config,model); // 唯一一次decode
                FrameResult result{};TemporalDiagnostics diagnostics;
                if(ready){result=finalizeDecodedFrame(decoded,stamp,image.size(),*temporal,display);diagnostics=temporal->diagnostics();}
                else {result.frame_id=id;result.timestamp_us=input.timestamp_us;result.status=Status::NOT_READY;result.diagnostics={"PIPELINE_BUDGET_MISSING"};}
                out<<"{\"frame_id\":"<<id<<",\"timestamp_us\":"<<input.timestamp_us<<",\"time_source\":0,\"timestamp_recipe\":\"video_fps\",\"fps\":"<<fps<<",\"original_size\":["<<image.cols<<','<<image.rows
                   <<"],\"integration\":\"SHARED_STAGE_ASSEMBLY\",\"budget_version\":"<<audit::quote(ready?"CONFIG_VALUES_USER_APPROVAL_REQUIRED":"G-B_PENDING")
                   <<",\"input_sha256\":"<<audit::quote(ih)<<",\"config_sha256\":"<<audit::quote(ch)<<",\"effective_config_sha256\":"<<audit::quote(eh)<<",\"model_sha256\":"<<audit::quote(mh)
                   <<",\"commit_label\":"<<audit::quote(MARK_COMMIT)<<",\"code_sha256\":"<<audit::quote(MARK_CODE_HASH)<<",\"decode\":";
                audit::decodePayload(out,decoded);out<<",\"finalized\":";audit::frameResult(out,result);out<<",\"temporal\":";audit::temporalDiagnostics(out,diagnostics);out<<"}\n";
                if(!decoded.detections.empty())++raw_count;tracks+=result.tracks.size();++id;
            }
            if(id==0)throw std::runtime_error("video has no readable frames");
            std::cerr<<"frames="<<id<<" raw_detected="<<raw_count<<" tracks="<<tracks<<" budget_ready="<<ready<<'\n';
        }
        out.flush();if(!out)throw std::runtime_error("record write failed");return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
