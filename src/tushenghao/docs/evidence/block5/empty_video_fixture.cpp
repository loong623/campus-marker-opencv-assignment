// 用真实容器零帧测试EOF失败，不拿打不开的路径冒充已处理零帧视频。
#include <opencv2/videoio.hpp>
#include <iostream>
int main(int argc,char** argv){if(argc!=2)return 1;cv::VideoWriter writer(argv[1],cv::VideoWriter::fourcc('M','J','P','G'),50,{320,240});if(!writer.isOpened())return 2;writer.release();cv::VideoCapture capture(argv[1]);cv::Mat frame;std::cout<<"opened="<<capture.isOpened()<<" read="<<capture.read(frame)<<'\n';return capture.isOpened()?0:3;}
