// 归档核查只解码PNG并比像素，不参与任何检测/准入。
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <iostream>
#include <set>
#include <fstream>
int main(int argc,char** argv){try{if(argc!=4)throw std::runtime_error("VIDEO FRAMES_DIR REPORT required");namespace fs=std::filesystem;fs::path dir=argv[2];std::set<size_t> requested;size_t png=0;
 for(auto& e:fs::directory_iterator(dir))if(e.path().extension()==".png"){auto image=cv::imread(e.path().string());if(image.empty())throw std::runtime_error("unreadable PNG");++png;auto name=e.path().filename().string();if(name.find("-original.png")!=std::string::npos)requested.insert(std::stoull(name.substr(6)));}
 cv::VideoCapture video(argv[1]);if(!video.isOpened())throw std::runtime_error("video failed");cv::Mat image;size_t id=0,matched=0;while(video.read(image)){if(requested.count(id)){auto original=cv::imread((dir/("frame-"+std::to_string(id)+"-original.png")).string());if(original.size()!=image.size()||original.type()!=image.type()||cv::norm(original,image,cv::NORM_INF)!=0)throw std::runtime_error("original pixels changed");++matched;}++id;}
 if(matched!=requested.size()||png!=matched*2)throw std::runtime_error("PNG coverage mismatch");if(fs::exists(argv[3]))throw std::runtime_error("existing report");std::ofstream out(argv[3]);out<<"{\"result\":\"PASS\",\"png_readable\":"<<png<<",\"originals_pixel_exact\":"<<matched<<",\"actual_decoded_frames\":"<<id<<"}\n";out.close();if(!out)throw std::runtime_error("write failed");std::cout<<"PASS PNG "<<png<<" / original pixel-exact "<<matched<<'\n';return 0;
 }catch(std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
