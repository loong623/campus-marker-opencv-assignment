// 原App无参静默退出0且只支持check-config；严格解析离线两模式，实际算法由公共process完成。
#include "offline_runner.hpp"
#include "mark/detector.hpp"
#include <iostream>
#include <map>
int main(int argc,char** argv){try{
 if(argc==1){std::cerr<<"用法: marker_app --check-config [--config YAML] | --video VIDEO --config YAML --run-dir NEW_DIRECTORY --mode baseline|debug [--run-purpose production|verification]\n";return 1;}
 std::map<std::string,std::string> args;bool check=false;
 for(int i=1;i<argc;++i){std::string key=argv[i];if(key=="--check-config"){if(check)throw std::runtime_error("duplicate check-config");check=true;continue;}if(key!="--video"&&key!="--config"&&key!="--run-dir"&&key!="--mode"&&key!="--run-purpose")throw std::runtime_error("unknown option: "+key);if(i+1==argc||std::string(argv[i+1]).rfind("--",0)==0||!args.emplace(key,argv[++i]).second)throw std::runtime_error("missing value or duplicate option");}
 std::string config=args.count("--config")?args["--config"]:(std::filesystem::path(__FILE__).parent_path().parent_path()/"config/detector.yaml").string();auto app=mark::loadConfig(config);
 if(check){if(args.size()>(args.count("--config")?1u:0u))throw std::runtime_error("check-config conflicts with run options");mark::Detector d(app.detector_config);std::cout<<"Config check passed\n";return 0;}
 if(!args.count("--video"))throw std::runtime_error("--video required");if(args.count("--run-dir"))app.offline.directory=args["--run-dir"];if(args.count("--mode"))app.offline.mode=args["--mode"];
 auto purpose=args.count("--run-purpose")?args["--run-purpose"]:"production";if(purpose!="production"&&purpose!="verification")throw std::runtime_error("invalid run-purpose");if(purpose=="verification"&&app.offline.mode!="debug")throw std::runtime_error("verification requires debug mode");
 return mark::runOffline(app,args["--video"],config,mark::ExecutionScope::Full,{},purpose);
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
