#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <map>

namespace fs = std::filesystem;
const int inf=1e9;
const int min_color=150,ignore_area=50;
const double eps=0.1;

struct Lightbar{
    cv::Point outer_point;   //外顶点
    cv::Point inner_point;   //内顶点
};


void drawDot(cv::Mat image,cv::Point p){
    cv::circle(image,cv::Point(p.x,p.y),5,cv::Scalar(255,0,0),cv::FILLED);
}    

bool cmp_point(const cv::Point& a,const cv::Point& b){
    return a.x==b.x?a.y<b.y:a.x<b.x;
}
//判断一个轮廓是不是L型灯条，返回内外顶点
bool processLightBar(std::vector<cv::Point> in_contour,Lightbar& result){
    std::vector<cv::Point> contour;
    //进行外型的逼近（尝试不同精度）
    double epsilon=0.025*cv::arcLength(in_contour,true);
    cv::approxPolyDP(in_contour,contour,epsilon,true);
    if(contour.size()!=6){
        epsilon=0.035*cv::arcLength(in_contour,true);
        cv::approxPolyDP(in_contour,contour,epsilon,true);
        if(contour.size()!=6) return 0;
    }
    //找凸包，确定内顶点
    std::vector<cv::Point> hull;
    cv::convexHull(contour,hull,false,false);
    if(hull.size()!=5) return 0;    
    std::map<cv::Point,bool,
             bool(*)(const cv::Point&,const cv::Point&)> on_hull(cmp_point);
    for(cv::Point i:hull) on_hull[i]=1;
    int inner_point;
    for(int i=0;i<6;i++)
        if(!on_hull[contour[i]]) inner_point=i;
    result.inner_point=contour[inner_point];
    //找出外顶点
    result.outer_point=contour[(inner_point+3)%6];
    return 1;
}

double dis(cv::Point a,cv::Point b){   //两点距离
    return sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y));
}

bool para(cv::Point a,cv::Point b,cv::Point c,cv::Point d){   //ab和cd平行
    return std::abs((a.x-b.x)*(c.y-d.y)-(a.y-b.y)*(c.x-d.x))<=eps*dis(a,b)*dis(c,d);
}
bool same_line(cv::Point a,cv::Point b,cv::Point c){   //abc三点共线
    return std::abs((a.x-b.x)*(a.y-c.y)-(a.x-c.x)*(a.y-b.y))<=eps*dis(a,b)*dis(a,c);
}

//判断四个灯条疑似物能否组成装甲板
bool is_armor(const std::vector<Lightbar> point,std::vector<cv::Point>& result){
    //判断装甲板中心，内顶点，外顶点是否共线
    cv::Point mid={0,0};
    for(Lightbar i:point) mid.x+=i.outer_point.x,mid.y+=i.outer_point.y;
    mid.x/=4,mid.y/=4;
    for(Lightbar i:point)
        if(!same_line(mid,i.inner_point,i.outer_point)) return 0;
    //内外顶点构成的四边形应该对边平行
    std::vector<cv::Point> outer,inner;
    for(Lightbar i:point) outer.push_back(i.outer_point),inner.push_back(i.inner_point);
    std::vector<cv::Point> out_hull,in_hull;
    cv::convexHull(outer,out_hull,false,false);
    cv::convexHull(inner,in_hull,false,false);
    if(!para(out_hull[0],out_hull[1],out_hull[2],out_hull[3])||
       !para(out_hull[0],out_hull[3],out_hull[1],out_hull[2])||
       !para(in_hull[0],in_hull[1],in_hull[2],in_hull[3])||
       !para(in_hull[0],in_hull[3],in_hull[1],in_hull[2])) return 0;
    result=out_hull;
    return 1;
}
bool matchLightbar(const std::vector<Lightbar>& lightbar,std::vector<cv::Point>& result){
    std::vector<Lightbar> now(4);
    std::vector<cv::Point> res;
    for(int i=0;i<lightbar.size();i++)
    for(int j=i+1;j<lightbar.size();j++)
    for(int k=j+1;k<lightbar.size();k++)
    for(int l=k+1;l<lightbar.size();l++){
        now[0]=lightbar[i],now[1]=lightbar[j];
        now[2]=lightbar[k],now[3]=lightbar[l];
        if(is_armor(now,res)){
            result=res;
            return 1;
        }
    }
    return 0;
}
//提高饱和度
cv::Mat enhanceColor(const cv::Mat& bgr,double ratio){
    cv::Mat hsv;
    cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
    std::vector<cv::Mat> ch;
    cv::split(hsv, ch);
    ch[1].convertTo(ch[1],-1,ratio,0);
    cv::merge(ch, hsv);
    cv::Mat out;
    cv::cvtColor(hsv,out,cv::COLOR_HSV2BGR);
    return out;
}
void getContour(const cv::Mat frame,std::vector<std::vector<cv::Point>>& contours){
    cv::Mat enhance_image=enhanceColor(frame,3);
    //1.滤波
    cv::Mat denoise_image;
    cv::medianBlur(frame,denoise_image,5);
    //2.二值化
    cv::Mat gray_image;
    cv::cvtColor(denoise_image,gray_image,cv::COLOR_BGR2GRAY);
    cv::Mat binary_image;
    cv::threshold(gray_image,binary_image,0,255,cv::THRESH_BINARY|cv::THRESH_OTSU);
    //3.形态学处理
    const cv::Mat open_kernel=
        cv::getStructuringElement(cv::MORPH_RECT,cv::Size(5,5));
    cv::Mat open_image;
    cv::morphologyEx(binary_image,open_image,cv::MORPH_OPEN,open_kernel,cv::Point(-1,-1),2);
    const cv::Mat close_kernel=
        cv::getStructuringElement(cv::MORPH_RECT,cv::Size(5,5));
    cv::Mat morphology_image;
    cv::morphologyEx(open_image,morphology_image,cv::MORPH_CLOSE,close_kernel,cv::Point(-1,-1),2);
    //4.轮廓提取
    cv::findContours(morphology_image,contours,
                     cv::RETR_EXTERNAL,cv::CHAIN_APPROX_SIMPLE);
}

int main(int argc,char** argv){
    if(argc!=3){
        std::cout<<"输入格式错误，应为 ："<<argv[0]<<" <输入视频.mp4> <输出目录>\n";
        return -1;
    }
    const fs::path input_path(argv[1]);
    const fs::path output_path(argv[2]);
    if(!fs::is_regular_file(input_path)){
        std::cout<<"未找到输入文件\n";
        return -1;
    }
    cv::VideoCapture capture(input_path.string());
    if(!capture.isOpened()){
        std::cout<<"输出路径错误\n";
        return -1;
    }
    cv::VideoWriter writer;
    double fps=capture.get(cv::CAP_PROP_FPS);
    if(!std::isfinite(fps)||fps <= 0.0) fps=30;
    cv::Mat image;
    while(capture.read(image)){
        if(!writer.isOpened())
            writer.open(output_path.string(),cv::VideoWriter::fourcc('m','p','4','v'),
                        fps,image.size(),1);
        std::vector<std::vector<cv::Point>> counters;
        getContour(image,counters);
        std::vector<Lightbar> all_lightbar;
        for(const std::vector<cv::Point>& v:counters){
            if(cv::contourArea(v)<=ignore_area) continue;
            Lightbar This;
            if(processLightBar(v,This)) all_lightbar.push_back(This);
        }
        for(Lightbar l:all_lightbar){
            drawDot(image,l.outer_point);
            drawDot(image,l.inner_point);
        }
        std::vector<cv::Point> result;
        if(matchLightbar(all_lightbar,result))
            for(int i=0;i<4;i++)
                cv::line(image,result[i],result[(i+1)%4],
                         cv::Scalar(0,0,255),2,cv::LINE_AA);
        writer.write(image);
    }
    writer.release();
    return 0;
}