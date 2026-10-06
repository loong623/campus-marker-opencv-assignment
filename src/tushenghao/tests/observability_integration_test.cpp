#include "observability_fixture.hpp"
#include <type_traits>
#include "offline_runner.hpp"
#include "common/frame_record_reader.hpp"
#include <opencv2/videoio.hpp>
#include <fstream>
using namespace mark;using namespace observability_fixture;
namespace {
// 旧公共接口仍可单独消费；friend不改变PImpl对象布局，内部通道非法顺序主动失败。
void access() {
 static_assert(sizeof(Detector)==sizeof(std::unique_ptr<int>),"public Detector layout changed");
 Detector a(app().detector_config);a.config();a.reset(ResetReason::External);
 check(a.process(frame()).status==Status::NOT_DETECTED,"legacy consumer failed");
 DetectorDiagnosticsAccess::prepare(a,request());
 rejects([&]{DetectorDiagnosticsAccess::prepare(a,request());});
 a.process(frame(1,14000));auto r=DetectorDiagnosticsAccess::take(a);
 check(r.frame_id==1&&r.result_status==Status::NOT_DETECTED&&r.run_id=="fixture","wrong record");
 rejects([&]{DetectorDiagnosticsAccess::take(a);});
 DetectorDiagnosticsAccess::prepare(a,request());a.process(frame(2,28000));
 rejects([&]{a.process(frame(3,42000));});
 DetectorDiagnosticsAccess::take(a);
}
// 每次非法/NOT_READY也产生记录；非法之后允许低id新段，并保持有效载荷为空。
void statuses() {
 Detector a(app().detector_config);DetectorDiagnosticsAccess::prepare(a,request());
 auto bad=frame();bad.image.release();auto result=a.process(bad);auto record=DetectorDiagnosticsAccess::take(a);
 check(result.status==Status::INVALID_INPUT&&record.result_status==result.status&&!record.events.empty(),"invalid record lost");
 DetectorDiagnosticsAccess::prepare(a,request());check(a.process(frame()).status==Status::NOT_DETECTED,"invalid recovery failed");DetectorDiagnosticsAccess::take(a);
 auto c=app().detector_config;c.temporal.correspondence_uncertainty_px.reset();Detector b(c);
 DetectorDiagnosticsAccess::prepare(b,request());check(b.process(frame()).status==Status::NOT_READY,"budget gate changed");
 check(DetectorDiagnosticsAccess::take(b).result_status==Status::NOT_READY,"notready record lost");
}
// 无全局observer；reset A不能污染B，重复外部reset保留每种真实调用数及未知来源。
void instances() {
 Detector a(app().detector_config),b(app().detector_config);
 a.reset(ResetReason::External);a.reset(ResetReason::External);a.reset(ResetReason::InputChanged);a.reset(ResetReason::InvalidSequence);
 DetectorDiagnosticsAccess::prepare(b,request());b.process(frame());auto rb=DetectorDiagnosticsAccess::take(b);
 auto isReset=[](ReasonCode code){return code==ReasonCode::ResetExternal||code==ReasonCode::ResetInputChanged||code==ReasonCode::ResetInvalidSequence;};
 for(const auto& event:rb.events)check(!isReset(event.reason),"reset crossed instances");
 DetectorDiagnosticsAccess::prepare(a,request());a.process(frame());auto ra=DetectorDiagnosticsAccess::take(a);
 uint64_t n=0;for(const auto& event:ra.events)if(isReset(event.reason)) {check(!event.source_frame_id,"external reset falsely attributed to input");n+=event.occurrences;}
 check(n==4,"reset reasons/counts lost or duplicated");
}
// 同一真实fixture跨计时与三级诊断逐字段相同；采样不能截断时序历史。
void consistency(){std::vector<FrameResult> golden;for(bool timing:{false,true})for(auto level:{DiagnosticsLevel::Summary,DiagnosticsLevel::Frame,DiagnosticsLevel::Evidence}){Detector detector(app().detector_config);for(uint64_t id=0;id<4;++id){auto req=request(timing);req.level=level;DetectorDiagnosticsAccess::prepare(detector,req);auto result=detector.process(frame(id,id*14000,id!=2));auto record=DetectorDiagnosticsAccess::take(detector);if(golden.size()<4)golden.push_back(result);else{const auto& g=golden[id];check(g.status==result.status&&g.detections.size()==result.detections.size()&&g.tracks.size()==result.tracks.size()&&g.display_state.has_value()==result.display_state.has_value(),"I04 status/count changed");for(size_t i=0;i<g.detections.size();++i)check(temporal_fixture::same(g.detections[i],result.detections[i]),"I04 raw changed");for(size_t i=0;i<g.tracks.size();++i)check(g.tracks[i].detection_index==result.tracks[i].detection_index&&temporal_fixture::same(g.tracks[i].result,result.tracks[i].result),"I04 stable changed");}check(record.timings[5].status==(timing?TimingStatus::MEASURED:TimingStatus::DISABLED),"I04 timing state");}}}
// baseline冲突必须拒绝；新增范围、类型和无实现开关均校验，不能静默降级。
void configCases(){auto a=app();a.diagnostics.detail_interval=0;rejects([&]{validateConfig(a);});a=app();a.diagnostics.detail_first=9;a.diagnostics.detail_last=8;rejects([&]{validateConfig(a);});a=app();a.offline.export_video=true;rejects([&]{validateConfig(a);});a=app();a.offline.directory="unused";a.detector_config.debug.timing_enabled=false;rejects([&]{runOffline(a,"unused","unused",ExecutionScope::Full);});}
// 真实十帧视频跑完整公共流程，只选2/5/8记录；处理数与结果指纹不受detail采样影响。
void sampling(){namespace fs=std::filesystem;auto path=fs::temp_directory_path()/("block5-video-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));fs::create_directories(path);auto video=path/"input.avi";cv::VideoWriter writer(video.string(),cv::VideoWriter::fourcc('M','J','P','G'),50,{320,240});check(writer.isOpened(),"I06 fixture codec unavailable");for(int i=0;i<10;++i)writer.write(cv::Mat(240,320,CV_8UC3,cv::Scalar(0,0,0)));writer.release();auto config_source=std::filesystem::path(__FILE__).parent_path().parent_path()/"config/detector.yaml";auto a=loadConfig(std::filesystem::relative(config_source,std::filesystem::current_path()));a.offline.mode="baseline";a.offline.directory=(path/"baseline").string();runOffline(a,video.string(),(std::filesystem::path(__FILE__).parent_path().parent_path()/"config/detector.yaml").string(),ExecutionScope::Full);a.offline.mode="debug";a.offline.directory=(path/"debug").string();a.diagnostics.level="frame";a.diagnostics.detail_first=2;a.diagnostics.detail_last=8;a.diagnostics.detail_interval=3;runOffline(a,video.string(),"fixture-config",ExecutionScope::Full);check(std::filesystem::is_regular_file(loadConfig(path/"baseline/effective_config.yaml").detector_config.marker_geometry_path_),"I06 effective relative model reload broken");std::ifstream in(path/"debug/frames.jsonl");std::string line;std::vector<uint64_t> ids;while(std::getline(in,line))ids.push_back(mark::validation::readJson(line).at("frame_id").u64());check(ids==std::vector<uint64_t>{2,5,8},"I06 detail selection changed");std::ifstream summary(path/"debug/summary.yaml");std::string text((std::istreambuf_iterator<char>(summary)),{});check(text.find("submitted: \"10\"")!=std::string::npos,"I06 algorithm skipped frames");fs::remove_all(path);}
// 四scope都复用真实结果；geometry不冒称检测或公共process，decode不调用稳定层。
void scopes(){auto config=app().detector_config;auto model=loadMarkerGeometry(config.marker_geometry_path_);for(auto scope:{ExecutionScope::Geometry,ExecutionScope::Decode,ExecutionScope::Temporal}){FrameDiagnosticsContext c;auto req=request();req.scope=scope;c.begin(frame(0,0,true),req);auto d=runDecodePipeline(frame(0,0,true),config,model,&c);c.finish();check(c.record.timings[5].status==TimingStatus::NOT_EXECUTED,"I07 fake public total");if(scope==ExecutionScope::Geometry)check(!c.record.result_status&&c.record.geometry_scope_result=="READY"&&c.record.counts.completed.has_value(),"I07 geometry result semantics");else check(d.status==Status::DETECTED,"I07 decode fixture failed");}}
void realtime(){auto r=FrameRecord{};check(!r.enqueue_timestamp_ns&&!r.enqueue_to_result_us&&!r.slot_overwrite_count&&!r.consumed_frame_count&&r.realtime_applicability=="not_applicable","I08 fake offline realtime");}
void raw(){Detector d(app().detector_config);DetectorDiagnosticsAccess::prepare(d,request());auto result=d.process(frame(0,0,true));auto r=DetectorDiagnosticsAccess::take(d);check(r.details&&r.details->decoded.detections.size()==result.detections.size()&&temporal_fixture::same(r.details->decoded.detections[0],result.detections[0]),"I09 publication channel changed raw");}
void errors(){auto a=app();a.offline.directory=(std::filesystem::temp_directory_path()/("block5-missing-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))).string();rejects([&]{runOffline(a,"/nonexistent-block5-video.avi","fixture",ExecutionScope::Full);});check(!std::filesystem::exists(a.offline.directory),"I10 missing input created success run");}

}
int main(){return temporal_fixture::run({{"I01_internal_access",access},{"I02_early_status",statuses},{"I03_R07_reset_isolation",instances},{"I04_modes",consistency},{"I05_config",configCases},{"I06_sampling",sampling},{"I07_scopes",scopes},{"I08_offline",realtime},{"I09_raw",raw},{"I10_io",errors}});}
