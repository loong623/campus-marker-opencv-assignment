#include "offline_runner.hpp"
#include "input_completion.hpp"
#include "debug_renderer.hpp"
#include "pipeline/detector_diagnostics_access.hpp"
#include "pipeline/stabilize_stage.hpp"
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>
namespace mark {namespace {
using Clock=std::chrono::steady_clock;
// App只使用标准库文件FNV指纹并明确算法名/大小；归档工具另补SHA256，不能冒称密码学摘要。
std::string fingerprintFile(const std::filesystem::path& path){std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("cannot fingerprint input: "+path.string());uint64_t hash=14695981039346656037ull,size=0;char buffer[65536];while(in){in.read(buffer,sizeof buffer);auto n=in.gcount();size+=uint64_t(n);for(std::streamsize i=0;i<n;++i){hash^=static_cast<unsigned char>(buffer[i]);hash*=1099511628211ull;}}if(!in.eof())throw std::runtime_error("fingerprint read failed");std::ostringstream out;out<<"FNV1a64:"<<std::hex<<std::setw(16)<<std::setfill('0')<<hash<<":size="<<std::dec<<size;return out.str();}
// 读取实际CPU名称；环境不可读取时明确UNKNOWN，不能推测硬件。
std::string cpu(){std::ifstream in("/proc/cpuinfo");std::string line;while(std::getline(in,line))if(line.rfind("model name",0)==0)return line.substr(line.find(':')+1);return "UNKNOWN: cpuinfo unavailable";}
// App阶段有独立边界；public total替换内部近似值，其他阶段不允许重复计时。
void externalTiming(FrameRecord& r,Stage stage,Clock::duration elapsed,bool enabled){auto& slot=r.timings.at(size_t(stage));if(slot.status==TimingStatus::MEASURED&&stage!=Stage::ProcessTotal)throw std::logic_error("EXTERNAL_STAGE_DUPLICATE");slot={stage,enabled?TimingStatus::MEASURED:TimingStatus::DISABLED,enabled?std::optional<std::chrono::nanoseconds>(std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed)):std::nullopt};if(stage==Stage::ProcessTotal)r.process_total_boundary="public_call";}
}
// 每次read都处理；detail采样只控制载荷。限速、GUI、编码和记录均在公共process之外。
int runOffline(AppConfig app,const std::string& video,const std::string& config_path,ExecutionScope scope,const std::set<uint64_t>& subset,const std::string& purpose){
 // 原loader保留相对配置定位规则；runner规范已解析路径，避免effective_config重载时再拼run目录。
 app.detector_config.marker_geometry_path_=std::filesystem::absolute(app.detector_config.marker_geometry_path_).lexically_normal().string();
 validateConfig(app);const bool baseline=app.offline.mode=="baseline",timing=app.detector_config.debug.timing_enabled;
 if(app.offline.directory.empty())throw std::runtime_error("run directory required");
 if(baseline&&(!timing||app.diagnostics.level!="summary"||app.detector_config.output.show_window||app.detector_config.output.show_held_state||app.detector_config.debug.draw_candidates||app.render.draw_corner_evidence||app.offline.export_evidence||app.offline.playback_fps!=0))throw std::runtime_error("baseline configuration conflict");
 if(app.offline.export_evidence&&(baseline||app.diagnostics.level!="evidence"))throw std::runtime_error("evidence export requires debug/evidence");
 if(app.detector_config.output.show_window&&std::getenv("DISPLAY")==nullptr&&std::getenv("WAYLAND_DISPLAY")==nullptr)throw std::runtime_error("GUI requested but display unavailable");
 RunMetadata meta;meta.run_id=std::filesystem::path(app.offline.directory).filename().string();if(meta.run_id.empty())throw std::runtime_error("invalid run directory name");meta.mode=app.offline.mode;meta.run_purpose=purpose;meta.source_path=std::filesystem::absolute(video).lexically_normal().string();meta.config_path=std::filesystem::absolute(config_path).lexically_normal().string();meta.model_path=std::filesystem::absolute(app.detector_config.marker_geometry_path_).lexically_normal().string();meta.commit_label=MARK_COMMIT;meta.code_fingerprint=MARK_CODE_HASH;meta.input_fingerprint=fingerprintFile(meta.source_path);meta.environment={{"CPU",cpu()},{"OS","Linux"},{"compiler",__VERSION__},{"OpenCV",CV_VERSION},{"build_type",MARK_BUILD_TYPE},{"OpenCV_threads",std::to_string(cv::getNumThreads())},{"CWD",std::filesystem::current_path().string()},{"model_fingerprint",fingerprintFile(meta.model_path)},{"execution_scope",scopeName(scope)},{"execution_subset",subset.empty()?"all":"explicit frame list"},{"timestamp_recipe","llround(frame_id * 1e6 / video_fps); algorithm source Unknown"}};
 if(!subset.empty()){std::ostringstream ids;for(auto id:subset){if(ids.tellp()>0)ids<<',';ids<<id;}meta.environment["execution_subset_ids"]=ids.str();}
 DiagnosticsRecorder recorder(app.diagnostics);recorder.beginRun(meta,app.offline.directory);writeEffectiveConfig(app,recorder.directory()/"effective_config.yaml");
 const auto startup=Clock::now();cv::VideoCapture capture(meta.source_path);if(!capture.isOpened())throw std::runtime_error("cannot open video");const double fps=capture.get(cv::CAP_PROP_FPS);if(!std::isfinite(fps)||fps<=0)throw std::runtime_error("invalid video fps");
 // 旧 EOF 不核数量；保留容器原值，显式期待数优先而不改写 metadata。
 const auto metadata_expected=metadata_frame_count(capture.get(cv::CAP_PROP_FRAME_COUNT));
 const auto expected=app.offline.expected_frame_count?std::optional<uint64_t>(app.offline.expected_frame_count):metadata_expected;
 meta.environment["final_fixes_contract"]="1";
 meta.environment["expected_frames"]=expected?std::to_string(*expected):"unknown";
 meta.environment["expected_frames_source"]=app.offline.expected_frame_count?"explicit":metadata_expected?"metadata":"unknown";
 meta.environment["video_fps"]=std::to_string(fps);meta.environment["frame_count_metadata"]=std::to_string(capture.get(cv::CAP_PROP_FRAME_COUNT));
 std::unique_ptr<Detector> detector;std::optional<MarkerGeometry> model;std::unique_ptr<TemporalStabilizer> temporal;std::unique_ptr<DisplayHistory> display;
 if(scope==ExecutionScope::Full)detector=std::make_unique<Detector>(app.detector_config);else{model=loadMarkerGeometry(meta.model_path);if(scope==ExecutionScope::Temporal){temporal=std::make_unique<TemporalStabilizer>(app.detector_config.temporal);display=std::make_unique<DisplayHistory>(DisplayHistoryConfig{app.detector_config.temporal.display_hold_enabled,app.detector_config.temporal.max_hold_frames});}}
 meta.environment["startup_us"]=std::to_string(std::chrono::duration<double,std::micro>(Clock::now()-startup).count());writeRunManifest(meta,(recorder.directory()/"manifest.yaml").string());
 meta.environment["video_export_requested"]="false";
 ResultFingerprint fingerprint;uint64_t id=0;auto pending=subset;auto wall_start=Clock::now();cv::Mat image;std::chrono::nanoseconds eof_read{};bool incomplete=false;
 for(;;){auto read_start=Clock::now();bool got=capture.read(image);auto read_elapsed=Clock::now()-read_start;if(!got){eof_read=std::chrono::duration_cast<std::chrono::nanoseconds>(read_elapsed);break;}
  if(!subset.empty()&&!subset.count(id)){++id;continue;}
  long double timestamp=static_cast<long double>(id)*1e6/fps;if(!std::isfinite(timestamp)||timestamp>std::numeric_limits<int64_t>::max()-.5L)throw std::runtime_error("video timestamp overflow");
  FrameInput input{image,id,std::llround(double(id)*1e6/fps),TimestampSource::Unknown};DiagnosticsRequest request;request.run_id=meta.run_id;request.timing_enabled=timing;request.scope=scope;request.timestamp_recipe="video_fps";request.level=app.diagnostics.level=="evidence"?DiagnosticsLevel::Evidence:app.diagnostics.level=="frame"?DiagnosticsLevel::Frame:DiagnosticsLevel::Summary;
  request.selected=request.level!=DiagnosticsLevel::Summary&&id>=app.diagnostics.detail_first&&(!app.diagnostics.detail_last||id<=*app.diagnostics.detail_last)&&(id-app.diagnostics.detail_first)%app.diagnostics.detail_interval==0;
  FrameRecord record;FrameResult result{};
  if(detector){auto prepare_start=Clock::now();DetectorDiagnosticsAccess::prepare(*detector,request);recorder.summary().bookkeeping+=Clock::now()-prepare_start;auto start=Clock::now();try{result=detector->process(input);}catch(...){record=DetectorDiagnosticsAccess::take(*detector);record.run_failed=true;recorder.submit(record);throw;}auto elapsed=Clock::now()-start;auto take_start=Clock::now();record=DetectorDiagnosticsAccess::take(*detector);externalTiming(record,Stage::ProcessTotal,elapsed,timing);recorder.summary().bookkeeping+=Clock::now()-take_start;}
  else{FrameDiagnosticsContext context;context.begin(input,request);auto decoded=runDecodePipeline(input,app.detector_config,*model,&context);result.frame_id=id;result.timestamp_us=input.timestamp_us;result.status=decoded.status;if(temporal)result=finalizeDecodedFrame(decoded,{id,input.timestamp_us,input.time_source},image.size(),*temporal,*display,&context);else result.detections=decoded.detections;if(scope!=ExecutionScope::Geometry)context.output(result);context.finish();record=std::move(context.record);}
  if(!meta.environment.count("first_submitted_frame_id"))meta.environment["first_submitted_frame_id"]=std::to_string(id);
  auto bookkeeping_start=Clock::now();externalTiming(record,Stage::Capture,read_elapsed,timing);if(scope!=ExecutionScope::Geometry)fingerprint.add(result);recorder.summary().bookkeeping+=Clock::now()-bookkeeping_start;
  cv::Mat overlay;if(!baseline){auto start=Clock::now();auto render=app.render;render.draw_candidates=app.detector_config.debug.draw_candidates;render.show_held_state=app.detector_config.output.show_held_state;auto events=validateRenderEvidence(record);record.events.insert(record.events.end(),events.begin(),events.end());overlay=renderDebugFrame(image,result,record,render);if(app.detector_config.output.show_window)cv::imshow("marker debug",overlay);externalTiming(record,Stage::Visualize,Clock::now()-start,timing);}
  if(!baseline&&(app.detector_config.output.show_window||app.offline.playback_fps>0)){auto start=Clock::now();if(app.detector_config.output.show_window){int key=cv::waitKey(1);if(key==27||key=='q')incomplete=true;}if(app.offline.playback_fps>0)std::this_thread::sleep_until(start+std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1/app.offline.playback_fps)));externalTiming(record,Stage::Wait,Clock::now()-start,timing);}
  auto export_start=Clock::now();if(record.selected&&app.offline.export_evidence){auto dir=recorder.directory()/"evidence";std::filesystem::create_directories(dir);if(!cv::imwrite((dir/(std::to_string(id)+"-original.png")).string(),image)||!cv::imwrite((dir/(std::to_string(id)+"-overlay.png")).string(),overlay))throw std::runtime_error("evidence image write failure");}
  recorder.submit(record);if(record.selected)recorder.complete_export(id,
   {Stage::Export,timing?TimingStatus::MEASURED:TimingStatus::DISABLED,
    timing?std::optional<std::chrono::nanoseconds>(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-export_start)):std::nullopt},record.selected);pending.erase(id);++id;if(incomplete)break;
 }
 // 缺失子集也保留已处理记录，以 incomplete 结束而不是丢掉诊断。
 const auto completion=evaluate_input_completion(id,expected,incomplete,!subset.empty(),pending);
 incomplete=completion.incomplete;
 meta.environment["coverage_verified"]=completion.coverage_verified?"true":"false";
 meta.environment["completion_reason"]=completion.reason;
 if(incomplete) {
  std::ofstream marker(recorder.directory()/"INCOMPLETE.json");
  marker<<"{\"reason\":"<<quoteJson(completion.reason)<<",\"frames_read\":"<<id
        <<",\"expected_frames\":"<<(expected?std::to_string(*expected):"null")<<"}\n";
  marker.flush();if(!marker)throw std::runtime_error("incomplete marker write failure");
 }
auto& summary=recorder.summary();summary.incomplete=incomplete;summary.wall=std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-wall_start);summary.result_fingerprint=fingerprint.hex();summary.fingerprint_frames=fingerprint.frames();meta.environment["eof_read_us"]=std::to_string(double(eof_read.count())/1000);meta.environment["frames_read"]=std::to_string(id);meta.environment["coverage"]=subset.empty()?"full_input":"explicit_subset";writeRunManifest(meta,(recorder.directory()/"manifest.yaml").string());auto final=recorder.finishRun();std::ofstream export_report(recorder.directory()/"run_export.yaml");export_report<<"run_export_us: "<<std::setprecision(17)<<double(final.run_export.count())/1000<<"\nboundary: \"side-table writes plus finishRun summary write; this supplement write excluded\"\n";export_report.flush();if(!export_report)throw std::runtime_error("run export supplement write failure");std::cout<<"run="<<meta.run_id<<" frames="<<final.submitted<<" fingerprint="<<final.result_fingerprint<<" incomplete="<<incomplete<<'\n';return incomplete?1:0;
}
}
