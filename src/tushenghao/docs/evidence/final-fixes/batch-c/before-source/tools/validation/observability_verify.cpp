// 只读核验新schema和历史temporal金样；不回写、不补造旧耗时，不参与检测准入。
#include "common/frame_record_reader.hpp"
#include "common/file_digest.hpp"
#include "pipeline/run_report.hpp"
#include "pipeline/diagnostics_recorder.hpp"
#include "config/config.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <map>
#include <sstream>
#include <optional>
namespace {
using namespace mark;using namespace mark::validation;namespace fs=std::filesystem;
std::string file(const fs::path& p){std::ifstream in(p);if(!in)throw std::runtime_error("missing file: "+p.string());return {std::istreambuf_iterator<char>(in),{}};}
// 仅解析本项目生成的标量YAML报告；配置继续用原严格loader，不能用FileStorage丢掉null。
std::map<std::string,std::string> yaml(const fs::path& p){std::map<std::string,std::string> values;std::vector<std::pair<size_t,std::string>> stack;std::istringstream in(file(p));std::string line;while(std::getline(in,line)){size_t offset=line.find_first_not_of(' ');if(offset==std::string::npos||line[offset]=='#'||line[offset]=='%'||line.substr(offset)=="---")continue;size_t colon=line.find(':',offset);if(colon==std::string::npos)throw std::runtime_error("invalid report YAML");std::string key=line.substr(offset,colon-offset);if(key.front()=='"')key=readJson(key).text;while(!stack.empty()&&stack.back().first>=offset)stack.pop_back();std::string name;for(auto& parent:stack)name+=parent.second+".";name+=key;size_t value=line.find_first_not_of(' ',colon+1);if(value==std::string::npos){stack.emplace_back(offset,key);continue;}auto text=line.substr(value);if(!text.empty()&&text.front()=='"')text=readJson(text).text;if(!values.emplace(name,text).second)throw std::runtime_error("duplicate report YAML key");}return values;}
uint64_t u64(const std::string& s){JsonValue v;v.kind=JsonValue::Kind::String;v.text=s;return v.u64();}
// 核对真实文件，不凭manifest自述接受输入/model指纹；同时补SHA256关联到非密码学FNV。
std::string fingerprintFile(const fs::path& path){std::ifstream in(path,std::ios::binary);if(!in)throw std::runtime_error("fingerprint input missing");uint64_t hash=14695981039346656037ull,size=0;char bytes[65536];while(in){in.read(bytes,sizeof bytes);auto n=in.gcount();size+=uint64_t(n);for(std::streamsize i=0;i<n;++i){hash^=static_cast<unsigned char>(bytes[i]);hash*=1099511628211ull;}}if(!in.eof())throw std::runtime_error("fingerprint IO failure");std::ostringstream out;out<<"FNV1a64:"<<std::hex<<std::setw(16)<<std::setfill('0')<<hash<<":size="<<std::dec<<size;return out.str();}
JsonValue canonical(JsonValue v){if(v.kind==JsonValue::Kind::String&&!v.text.empty()&&v.text.find_first_not_of("0123456789")==std::string::npos)v.kind=JsonValue::Kind::Number;for(auto& p:v.object)p.second=canonical(std::move(p.second));for(auto& p:v.array)p=canonical(std::move(p));return v;}
std::vector<std::string> reasons(const JsonValue& v){std::vector<std::string> out;for(const auto& p:v.array)if(p.text.rfind("geometry/parent=",0)!=0)out.push_back(p.text);return out;}
// 旧格式未保存CornerEvidence.error_；新增原值另核等于其真实两线平均残差之和，公共证据共同字段仍exact。
JsonValue commonMeasurements(JsonValue v){for(auto& m:v.array)for(auto& e:m.array){auto it=e.object.find("measurement_error_px");if(it!=e.object.end()){const auto& means=e.at("line_mean_residual_px").array;if(!it->second.null()&&it->second.number()!=means[0].number()+means[1].number())throw std::runtime_error("original evidence error mismatch");e.object.erase(it);}}return canonical(std::move(v));}
struct Row {JsonValue json,result,decode,temporal;uint64_t id{};int64_t time{};FrameResult output;std::string scope;};
Row row(const std::string& line){Row r;r.json=readJson(line);bool legacy=r.json.object.count("finalized");r.scope=legacy?"temporal":r.json.at("execution_scope").text;r.id=r.json.at("frame_id").u64();r.time=r.json.at(legacy?"timestamp_us":"source_timestamp_us").i64();r.result=r.json.at(legacy?"finalized":"result");if(r.result.null())throw std::runtime_error("comparison requires result records");r.output=readResult(r.result,r.id,r.time);if(legacy){r.decode=r.json.at("decode");r.temporal=r.json.at("temporal");}else if(!r.json.at("details").null()){r.decode=r.json.at("details").at("decode");r.temporal=r.json.at("details").at("temporal");}return r;}
fs::path records(const fs::path& p){return fs::is_directory(p)?p/"frames.jsonl":p;}
// 差异定位限量保存字段路径；报告完整失败帧号，不因摘要裁剪而漏报失败。
void diffPaths(const JsonValue& a,const JsonValue& b,const std::string& path,std::vector<std::string>& out){if(a==b||out.size()>=32)return;if(a.kind!=b.kind){out.push_back(path);return;}if(a.kind==JsonValue::Kind::Object){for(const auto& v:a.object){auto it=b.object.find(v.first);if(it==b.object.end())out.push_back(path+"."+v.first);else diffPaths(v.second,it->second,path+"."+v.first,out);}for(const auto& v:b.object)if(!a.object.count(v.first))out.push_back(path+"."+v.first);}else if(a.kind==JsonValue::Kind::Array){if(a.array.size()!=b.array.size())out.push_back(path+".size");for(size_t i=0;i<std::min(a.array.size(),b.array.size());++i)diffPaths(a.array[i],b.array[i],path+"["+std::to_string(i)+"]",out);}else out.push_back(path);}
std::string compare(const fs::path& a,const fs::path& b,bool route,std::optional<uint64_t> expected){std::ifstream left(records(a)),right(records(b));if(!left||!right)throw std::runtime_error("comparison input missing");std::string x,y;uint64_t count=0;std::vector<uint64_t> failures;ResultFingerprint old_hash,new_hash;std::vector<std::pair<uint64_t,std::vector<std::string>>> differences;
 // 旧比较器空/空直接 PASS；每侧原始顺序和全输入范围必须在比较前核验。
 std::optional<uint64_t> previous_left,previous_right;
 while(true){bool one=bool(std::getline(left,x)),two=bool(std::getline(right,y));if(one!=two)throw std::runtime_error("comparison frame counts differ");if(!one)break;auto old=row(x),now=row(y);
 if((previous_left&&old.id<=*previous_left)||(previous_right&&now.id<=*previous_right))
  throw std::runtime_error("comparison IDs must strictly increase");
 previous_left=old.id;previous_right=now.id;
 if(expected&&(old.id!=count||now.id!=count))throw std::runtime_error("full comparison frame range mismatch");
 bool same=old.id==now.id&&old.time==now.time;auto first=canonical(old.result),second=canonical(now.result);first.object.erase("diagnostics");second.object.erase("diagnostics");
 if(route){for(auto key:{"detections"}){auto& aa=first.object.at(key).array;auto& bb=second.object.at(key).array;if(aa.size()!=bb.size())same=false;for(size_t i=0;i<std::min(aa.size(),bb.size());++i){auto original=aa[i].at("corners").array,patched=bb[i].at("corners").array;bool cyclic=false;for(size_t shift=0;shift<4;++shift){bool equal=true;for(size_t j=0;j<4;++j)equal=equal&&original[(j+shift)%4]==patched[j];cyclic=cyclic||equal;}if(!cyclic)same=false;if(aa[i].at("orientation").null()!=bb[i].at("orientation").null())same=false;if(!aa[i].at("orientation").null()&&!bb[i].at("orientation").null())for(size_t p=0;p<4;++p)if(!(original[aa[i].at("orientation").array[p].u64()]==patched[bb[i].at("orientation").array[p].u64()]))same=false;aa[i].object.erase("corners");bb[i].object.erase("corners");aa[i].object.erase("orientation");bb[i].object.erase("orientation");}}}
 // decode未执行稳定/显示，只比较双方实际共同执行的raw；其准入原因仍在decode载荷逐项比较。
 bool raw_only=old.scope=="decode"||now.scope=="decode";if(raw_only){first.object.erase("tracks");second.object.erase("tracks");first.object.erase("display");second.object.erase("display");}
 same=same&&first==second;if(!raw_only)same=same&&reasons(old.result.at("diagnostics"))==reasons(now.result.at("diagnostics"));
 if(!old.decode.null()&&!now.decode.null()){same=same&&old.decode.at("status")==now.decode.at("status")&&old.decode.at("search_truncated")==now.decode.at("search_truncated")&&commonMeasurements(old.decode.at("measurements"))==commonMeasurements(now.decode.at("measurements"))&&reasons(old.decode.at("diagnostics"))==reasons(now.decode.at("diagnostics"));}
 if(!old.temporal.null()&&!now.temporal.null())same=same&&canonical(old.temporal)==canonical(now.temporal);
 if(!same){failures.push_back(old.id);if(differences.size()<20){std::vector<std::string> paths;diffPaths(first,second,"result",paths);diffPaths(canonical(old.decode),canonical(now.decode),"decode",paths);diffPaths(canonical(old.temporal),canonical(now.temporal),"temporal",paths);if(old.id!=now.id)paths.push_back("frame_id");if(old.time!=now.time)paths.push_back("source_timestamp_us");if(paths.empty())paths.push_back("diagnostics or approved-route physical mapping");differences.push_back({old.id,std::move(paths)});}}old_hash.add(old.output);new_hash.add(now.output);++count;
 }
 if(!count)throw std::runtime_error("empty comparison rejected");
 if(expected&&count!=*expected)throw std::runtime_error("comparison expected frame count mismatch");
 std::ostringstream out;out<<"{\"coverage\":"<<quoteJson(expected?"full_input":"compared_records_only")<<",\"result\":"<<quoteJson(failures.empty()?"PASS":"G-UNEXPECTED")<<",\"frames\":"<<quoteJson(std::to_string(count))<<",\"old_fingerprint\":"<<quoteJson(old_hash.hex())<<",\"new_fingerprint\":"<<quoteJson(new_hash.hex())<<",\"excluded_fields\":[\"timing\",\"runmeta\",\"new observation events\",\"geometry/parent detailed trace only\",\"new measurement_error_px separately checked against original mean-residual sum\"],\"failure_indices\":[";for(size_t i=0;i<failures.size();++i){if(i)out<<',';out<<quoteJson(std::to_string(failures[i]));}out<<"],\"minimal_differences\":[";for(size_t i=0;i<differences.size();++i){if(i)out<<',';out<<"{\"frame_id\":"<<quoteJson(std::to_string(differences[i].first))<<",\"field_paths\":[";for(size_t k=0;k<differences[i].second.size();++k){if(k)out<<',';out<<quoteJson(differences[i].second[k]);}out<<"]}";}out<<"]}";return out.str();
}
// 核查真实详情的引用和计数，拒绝只有字段名而引用别帧或不存在组件的伪证据。
void checkDetails(const JsonValue& j){const auto& d=j.at("details");if(d.null())return;const auto& counts=j.at("counts");std::set<uint64_t> components;
 for(const auto& c:d.at("components").array)if(!components.insert(c.at("component_id").u64()).second)throw std::runtime_error("duplicate component id");
 if(!counts.at("components").null()&&counts.at("components").u64()!=components.size())throw std::runtime_error("component count mismatch");
 for(const auto& o:d.at("observations").array)if(!components.count(o.at("source_component_id").u64()))throw std::runtime_error("observation component reference missing");
 for(auto name:{"generated","validated","completed"}){const auto& rows=d.at(name).at("hypotheses").array;if(!counts.at(name).null()&&rows.size()!=counts.at(name).u64())throw std::runtime_error("hypothesis count mismatch");for(const auto& h:rows)for(const auto& a:h.at("assignments").array)if(!components.count(a.at("component_id").u64()))throw std::runtime_error("assignment component reference missing");}
 if(!d.at("decode").null()){const auto& measurements=d.at("decode").at("measurements").array;if(counts.at("measurements").u64()!=measurements.size())throw std::runtime_error("measurement count mismatch");for(const auto& m:measurements){if(m.array.size()!=4)throw std::runtime_error("measurement physical corners missing");for(size_t i=0;i<4;++i){const auto& e=m.array[i];if(e.at("frame_id").u64()!=j.at("frame_id").u64()||e.at("physical").u64()!=i||!components.count(e.at("component_id").u64()))throw std::runtime_error("evidence source reference mismatch");}}}
}
std::string checkRun(const fs::path& dir,std::optional<uint64_t> expected){if(fs::exists(dir/"INCOMPLETE.json"))throw std::runtime_error("run has incomplete marker");if(fs::exists(dir/"FAILED.json"))throw std::runtime_error("run has failure marker");auto manifest=yaml(dir/"manifest.yaml"),summary=yaml(dir/"summary.yaml");auto config=loadConfig(dir/"effective_config.yaml");if(!fs::is_regular_file(config.detector_config.marker_geometry_path_)||fs::weakly_canonical(config.detector_config.marker_geometry_path_)!=fs::weakly_canonical(manifest.at("model_path")))throw std::runtime_error("effective configuration model reload mismatch");
 if(config.offline.mode!=manifest.at("mode"))throw std::runtime_error("effective mode mismatch");
 if(manifest.at("run_purpose")!="experiment-grid"&&fingerprintFile(manifest.at("source_path"))!=manifest.at("input_fingerprint"))throw std::runtime_error("actual input fingerprint mismatch");
 if(summary.at("failed")!="false"||summary.at("incomplete")!="false")throw std::runtime_error("run failed/incomplete");uint64_t n=u64(summary.at("submitted"));if(!n)throw std::runtime_error("zero frame run");if(manifest.at("record_schema_version")!="1")throw std::runtime_error("manifest schema");// 原来只核内部计数；全输入必须同时证明期待数、读取数、提交数相等。
 const bool experiment=manifest.at("run_purpose")=="experiment-grid";
 const bool contract=manifest.count("environment.final_fixes_contract");
 const bool subset=contract?manifest.at("environment.coverage")=="explicit_subset":
  manifest.count("environment.execution_subset")&&manifest.at("environment.execution_subset")!="all";
 if(!experiment&&!subset) {
  if(contract&&manifest.at("environment.coverage_verified")!="true")throw std::runtime_error("coverage unverified");
  auto token=manifest.at(contract?"environment.expected_frames":"environment.frame_count_metadata");
  // 历史 metadata 是十进制浮点字符串，只接受正整值，不能用 0 冒充期待数。
  size_t used=0;long double value=std::stold(token,&used);
  if(used!=token.size()||!std::isfinite(value)||value<=0||std::floor(value)!=value||value>=std::ldexp(1.L,64))
   throw std::runtime_error("invalid expected frame metadata");
  auto frames=static_cast<uint64_t>(value);
  if(frames!=n||u64(manifest.at("environment.frames_read"))!=frames||(expected&&*expected!=frames))
   throw std::runtime_error("input coverage mismatch");
 } else if(expected&&n!=*expected)throw std::runtime_error("submitted/expected mismatch");
 std::set<uint64_t> ids,side_ids;uint64_t selected=0;ResultFingerprint hash;
 if(fs::exists(dir/"frames.jsonl")){std::ifstream frames(dir/"frames.jsonl");std::string line;while(std::getline(frames,line)){auto j=readJson(line);if(j.at("record_schema_version").i64()!=1||j.at("run_id").text!=manifest.at("run_id")||j.at("run_failed").boolean)throw std::runtime_error("record identity/schema/failure");auto id=j.at("frame_id").u64();j.at("source_timestamp_us").i64();checkDetails(j);if(!ids.insert(id).second)throw std::runtime_error("duplicate frame id");const auto& ts=j.at("timings").array;if(ts.size()!=9)throw std::runtime_error("timing slots missing");for(size_t i=0;i<9;++i){if(ts[i].at("stage").text!=stageName(Stage(i)))throw std::runtime_error("timing stage order");auto state=ts[i].at("status").text;bool measured=state=="MEASURED";if(measured==ts[i].at("elapsed_us").null())throw std::runtime_error("timing value/status mismatch");if(measured&&ts[i].at("elapsed_us").number()<0)throw std::runtime_error("negative time");if(state!="MEASURED"&&state!="DISABLED"&&state!="SKIPPED"&&state!="NOT_EXECUTED"&&state!="NOT_IMPLEMENTED")throw std::runtime_error("unknown timing status");}
 auto scope=j.at("execution_scope").text;if(scope!="full"&&ts[5].at("status").text!="NOT_EXECUTED")throw std::runtime_error("audit fake public total");if(scope=="geometry"&&(!j.at("result_status").null()||j.at("geometry_scope_result").null()||ts[3].at("status").text!="NOT_EXECUTED"||ts[4].at("status").text!="NOT_EXECUTED"))throw std::runtime_error("geometry fake detection");if(scope=="decode"&&ts[4].at("status").text!="NOT_EXECUTED")throw std::runtime_error("decode ran stabilize");if(scope=="full"&&j.at("process_total_boundary").text!="public_call")throw std::runtime_error("full total boundary");if(!j.at("realtime").at("enqueue_to_result_us").null()||!j.at("realtime").at("enqueue_timestamp_ns").null()||!j.at("realtime").at("clock_domain").null()||!j.at("realtime").at("slot_overwrite_count").null()||!j.at("realtime").at("consumed_frame_count").null())throw std::runtime_error("offline fake real-time latency");
 if(!j.at("result").null()&&(j.at("result_status").null()||j.at("result_status").i64()!=j.at("result").at("status").i64()))throw std::runtime_error("record/public status mismatch");
 if(!j.at("result").null())hash.add(readResult(j.at("result"),id,j.at("source_timestamp_us").i64()));++selected;
 }
 if(selected!=u64(summary.at("selected")))throw std::runtime_error("selected/export counts mismatch");if(selected==n&&hash.frames()&&hash.hex()!=summary.at("result_fingerprint"))throw std::runtime_error("result fingerprint mismatch");
 }else if(config.diagnostics.level!="summary")throw std::runtime_error("missing detailed records");
 // 原侧表假定全部 Export 都有详情且一定计时；视频未采样帧和 DISABLED 必须按真实范围核验。
 uint64_t measured_exports=0,disabled_exports=0,selected_exports=0;
 std::set<uint64_t> subset_ids;
 if(contract&&subset){std::istringstream list(manifest.at("environment.execution_subset_ids"));std::string id;
  while(std::getline(list,id,','))subset_ids.insert(u64(id));
  if(subset_ids.size()!=n)throw std::runtime_error("subset submitted coverage mismatch");}
 std::ifstream side(dir/"export_timings.jsonl");std::string side_line;
 bool first_export_found=false;
 while(std::getline(side,side_line)) {
  auto j=readJson(side_line);const auto id=j.at("frame_id").u64();
  if(j.at("run_id").text!=manifest.at("run_id")||!side_ids.insert(id).second)
   throw std::runtime_error("export identity/duplicate mismatch");
  const bool modern=j.object.count("status");
  if(contract&&!modern)throw std::runtime_error("new export missing status");
  const auto state=modern?j.at("status").text:"MEASURED";
  const bool measured=state=="MEASURED",disabled=state=="DISABLED";
  if((!measured&&!disabled)||measured==j.at("elapsed_us").null()||
     (measured&&j.at("elapsed_us").number()<0))throw std::runtime_error("export value/status mismatch");
  const bool chosen=modern?j.at("selected_record").boolean:true;
  if(chosen&&!ids.count(id))throw std::runtime_error("selected export record missing");
  if(contract) {
   if(experiment){if(!ids.count(id))throw std::runtime_error("experimental export range");}
   else if(subset){if(!subset_ids.count(id))throw std::runtime_error("subset export out of range");}
   else if(id>=n)throw std::runtime_error("video export outside submitted range");
   if(!chosen&&manifest.at("environment.video_export_requested")!="true")throw std::runtime_error("unselected export without video");
   if(chosen!=bool(ids.count(id)))throw std::runtime_error("export selected flag mismatch");
   const auto first=u64(manifest.at("environment.first_submitted_frame_id"));
   if(id==first) {
    first_export_found=true;const auto value=summary.at("stages.export.first_frame_us");
    if(measured?(value=="null"||std::stod(value)!=j.at("elapsed_us").number()):value!="null")
     throw std::runtime_error("first export snapshot mismatch");
   }
  } else if(!ids.count(id))throw std::runtime_error("legacy export record missing");
  measured_exports+=measured;disabled_exports+=disabled;selected_exports+=chosen;
 }
 if(selected_exports!=selected)throw std::runtime_error("selected export completion mismatch");
 if(contract) {
  if(u64(summary.at("stages.export.statuses.MEASURED"))!=measured_exports||
     u64(summary.at("stages.export.statuses.DISABLED"))!=disabled_exports||
     side_ids.size()>n||u64(summary.at("stages.export.statuses.NOT_EXECUTED"))!=n-side_ids.size())
   throw std::runtime_error("export summary counts mismatch");
  if(manifest.at("environment.video_export_requested")=="true"&&side_ids.size()!=n)
   throw std::runtime_error("video export coverage missing");
  if(side_ids.count(u64(manifest.at("environment.first_submitted_frame_id")))&&!first_export_found)
   throw std::runtime_error("first export completion missing");
 }
 for(size_t i=0;i<9;++i){uint64_t states=0;for(size_t j=0;j<5;++j)states+=u64(summary.at(std::string("stages.")+stageName(Stage(i))+".statuses."+timingStatusName(TimingStatus(j))));if(states!=n)throw std::runtime_error("summary denominator mismatch");}
 return "{\"result\":\"PASS\",\"input_sha256\":"+quoteJson(audit::sha256(manifest.at("source_path")))+",\"model_sha256\":"+quoteJson(audit::sha256(manifest.at("model_path")))+",\"submitted\":"+quoteJson(std::to_string(n))+",\"parsed_selected\":"+quoteJson(std::to_string(selected))+"}";
}
std::string checkArchive(const fs::path& dir){auto manifest=readJson(file(dir/"archive/artifact-manifest.json"));uint64_t n=0;for(const auto& entry:manifest.at("artifacts").array){fs::path relative=entry.at("path").text;if(relative.is_absolute()||relative.lexically_normal().string().rfind("..",0)==0)throw std::runtime_error("archive path escape");auto p=dir/relative;if(!fs::is_regular_file(p)||fs::file_size(p)!=entry.at("size").u64()||audit::sha256(p)!=entry.at("sha256").text)throw std::runtime_error("archive hash mismatch: "+relative.string());if(p.extension()==".jsonl"){std::ifstream in(p);std::string line;while(std::getline(in,line))readJson(line);}++n;}if(!fs::exists(dir/"INDEX.md")||!fs::exists(dir/"archive/commands.json"))throw std::runtime_error("archive navigation/commands missing");return "{\"result\":\"PASS\",\"artifacts\":"+quoteJson(std::to_string(n))+",\"manifest_sha256\":"+quoteJson(audit::sha256(dir/"archive/artifact-manifest.json"))+"}";}
}
int main(int argc,char** argv){fs::path report;try{std::string mode;std::vector<std::string> inputs;bool route=false;std::optional<uint64_t> expected;for(int i=1;i<argc;++i){std::string key=argv[i];if(key=="--report"){if(!report.empty()||i+1==argc)throw std::runtime_error("duplicate/missing report");report=argv[++i];}else if(key=="--expected-frames"){if(expected||i+1==argc)throw std::runtime_error("duplicate/missing expected-frames");std::string token=argv[++i];if(token.empty()||token.find_first_not_of("0123456789")!=std::string::npos||!(expected=u64(token))||!*expected)throw std::runtime_error("expected-frames requires positive integer");}else if(key=="--route-b"){if(route)throw std::runtime_error("duplicate route-b");route=true;}else if(key=="--compare"||key=="--check-run"||key=="--check-archive"){if(!mode.empty())throw std::runtime_error("duplicate verification operation");mode=key;int n=key=="--compare"?2:1;for(int k=0;k<n;++k){if(i+1==argc)throw std::runtime_error("missing input");inputs.push_back(argv[++i]);}}else throw std::runtime_error("unknown option: "+key);}if(mode.empty()||report.empty()||fs::exists(report)||(route&&mode!="--compare")||(expected&&mode=="--check-archive"))throw std::runtime_error("missing/invalid inputs or report already exists");std::string result=mode=="--compare"?compare(inputs[0],inputs[1],route,expected):mode=="--check-run"?checkRun(inputs[0],expected):checkArchive(inputs[0]);std::ofstream out(report);if(!out)throw std::runtime_error("cannot create report");out<<result<<'\n';out.flush();if(!out)throw std::runtime_error("report write failure");bool pass=result.find("G-UNEXPECTED")==std::string::npos;std::cout<<(pass?"PASS":"G-UNEXPECTED")<<'\n';return pass?0:1;}catch(const std::exception& e){if(!report.empty()&&!fs::exists(report)){std::ofstream out(report);out<<"{\"result\":\"FAIL\",\"error\":"<<quoteJson(e.what())<<"}\n";}std::cerr<<e.what()<<'\n';return 1;}}
