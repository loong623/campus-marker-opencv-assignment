#include "detector.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
// sort corners as LT RT RB LB
static Quad orderCorners(Quad points){
    cv::Point2f center(0.0f,0.0f);
    for(const auto& p:points){
        center+=p;
    }
    center*=0.25f;
    std::sort(points.begin(),points.end(),[&center](const cv::Point2f& a,const cv::Point2f& b){
        double angleA=std::atan2(a.y-center.y,a.x-center.x);
        double angleB=std::atan2(b.y-center.y,b.x-center.x);
        return angleA<angleB;
    });
    auto topLeft=std::min_element(points.begin(),points.end(),[](const cv::Point2f& a,const cv::Point2f& b){
        return a.x+a.y<b.x+b.y;
    });
    std::rotate(points.begin(),topLeft,points.end());
    return points;
}
// change candidate into a square image
static cv::Mat warpMarker(const cv::Mat& frame,const Quad& corners,int size=180){
    std::vector<cv::Point2f> src(corners.begin(),corners.end());
    std::vector<cv::Point2f> dst={
        cv::Point2f(0,0),
        cv::Point2f(size-1,0),
        cv::Point2f(size-1,size-1),
        cv::Point2f(0,size-1)
    };
    cv::Mat transform=cv::getPerspectiveTransform(src,dst);
    cv::Mat warped;
    cv::warpPerspective(frame,warped,transform,cv::Size(size,size));
    return warped;
}
// check whether candidate looks like the marker
static double markerScore(const cv::Mat& marker){
    cv::Mat gray;
    cv::cvtColor(marker,gray,cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray,gray,cv::Size(3,3),0);
    cv::Scalar mean;
    cv::Scalar stddev;
    cv::meanStdDev(gray,mean,stddev);
    if(stddev[0]<18.0){
        return -1.0;
    }
    cv::Mat binary;
    cv::threshold(gray,binary,0,255,cv::THRESH_BINARY|cv::THRESH_OTSU);
    int size=binary.rows;
    int margin=size/12;
    int cornerSize=size/3;
    cv::Rect lt(margin,margin,cornerSize,cornerSize);
    cv::Rect rt(size-margin-cornerSize,margin,cornerSize,cornerSize);
    cv::Rect rb(size-margin-cornerSize,size-margin-cornerSize,cornerSize,cornerSize);
    cv::Rect lb(margin,size-margin-cornerSize,cornerSize,cornerSize);
    auto brightRatio=[&binary](const cv::Rect& roi){
        return static_cast<double>(cv::countNonZero(binary(roi)))/static_cast<double>(roi.area());
    };
    double rLT=brightRatio(lt);
    double rRT=brightRatio(rt);
    double rRB=brightRatio(rb);
    double rLB=brightRatio(lb);
    double minCorner=std::min(std::min(rLT,rRT),std::min(rRB,rLB));
    double maxCorner=std::max(std::max(rLT,rRT),std::max(rRB,rLB));
    double averageCorner=(rLT+rRT+rRB+rLB)/4.0;
    double globalBright=static_cast<double>(cv::countNonZero(binary))/static_cast<double>(binary.total());
    if(minCorner<0.03){
        return -1.0;
    }
    if(globalBright<0.04||globalBright>0.65){
        return -1.0;
    }
    double balance=1.0-(maxCorner-minCorner);
    double contrastScore=std::min(stddev[0]/60.0,2.0);
    double score=3.0*averageCorner+2.0*minCorner+balance+contrastScore;
    return score;
}
// detect marker
bool detectMarker(const cv::Mat& frame,Quad& bestCorners){
    cv::Mat gray;
    cv::cvtColor(frame,gray,cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray,gray,cv::Size(5,5),0);
    cv::Mat edges;
    cv::Canny(gray,edges,50,150);
    cv::Mat kernel=cv::getStructuringElement(cv::MORPH_RECT,cv::Size(3,3));
    cv::morphologyEx(edges,edges,cv::MORPH_CLOSE,kernel);
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges,contours,cv::RETR_LIST,cv::CHAIN_APPROX_SIMPLE);
    double frameArea=static_cast<double>(frame.rows*frame.cols);
    double bestScore=-1.0;
    for(const auto& contour:contours){
        double area=std::abs(cv::contourArea(contour));
        if(area<frameArea*0.0002){
            continue;
        }
        if(area>frameArea*0.5){
            continue;
        }
        double perimeter=cv::arcLength(contour,true);
        std::vector<cv::Point> approximation;
        cv::approxPolyDP(contour,approximation,0.03*perimeter,true);
        Quad candidate;
        bool validQuad=false;
        if(approximation.size()==4&&cv::isContourConvex(approximation)){
            for(int i=0;i<4;i++){
                candidate[i]=cv::Point2f(static_cast<float>(approximation[i].x),static_cast<float>(approximation[i].y));
            }
            validQuad=true;
        }else{
            cv::RotatedRect rect=cv::minAreaRect(contour);
            float width=rect.size.width;
            float height=rect.size.height;
            if(width<15||height<15){
                continue;
            }
            float ratio=std::max(width,height)/std::min(width,height);
            if(ratio>2.0f){
                continue;
            }
            cv::Point2f box[4];
            rect.points(box);
            for(int i=0;i<4;i++){
                candidate[i]=box[i];
            }
            validQuad=true;
        }
        if(!validQuad){
            continue;
        }
        candidate=orderCorners(candidate);
        cv::Mat warped=warpMarker(frame,candidate);
        if(warped.empty()){
            continue;
        }
        double score=markerScore(warped);
        if(score<0.0){
            continue;
        }
        std::vector<cv::Point2f> candidateVector(candidate.begin(),candidate.end());
        double candidateArea=std::abs(cv::contourArea(candidateVector));
        double areaBonus=std::min(candidateArea/(frameArea*0.02),1.0);
        score+=0.5*areaBonus;
        if(score>bestScore){
            bestScore=score;
            bestCorners=candidate;
        }
    }
    return bestScore>2.0;
}
// smooth corners between frames
void smoothCorners(Quad& current,const Quad& previous,const cv::Size& frameSize){
    double averageMovement=0.0;
    for(int i=0;i<4;i++){
        double dx=current[i].x-previous[i].x;
        double dy=current[i].y-previous[i].y;
        averageMovement+=std::sqrt(dx*dx+dy*dy);
    }
    averageMovement/=4.0;
    double threshold=0.15*std::max(frameSize.width,frameSize.height);
    if(averageMovement>threshold){
        return;
    }
    const double alpha=0.65;
    for(int i=0;i<4;i++){
        current[i].x=static_cast<float>(alpha*previous[i].x+(1.0-alpha)*current[i].x);
        current[i].y=static_cast<float>(alpha*previous[i].y+(1.0-alpha)*current[i].y);
    }
}
// draw marker and corner names
void drawDetection(cv::Mat& frame,const Quad& corners){
    const char* labels[4]={"LT","RT","RB","LB"};
    std::vector<cv::Point> polygon;
    for(const auto& point:corners){
        polygon.emplace_back(cvRound(point.x),cvRound(point.y));
    }
    for(int i=0;i<4;i++){
        cv::line(frame,polygon[i],polygon[(i+1)%4],cv::Scalar(0,255,0),3);
        cv::circle(frame,polygon[i],7,cv::Scalar(0,0,255),-1);
        cv::putText(frame,labels[i],polygon[i]+cv::Point(8,-8),cv::FONT_HERSHEY_SIMPLEX,0.7,cv::Scalar(0,255,255),2);
    }
    cv::Rect boundingBox=cv::boundingRect(polygon);
    cv::rectangle(frame,boundingBox,cv::Scalar(255,0,0),2);
    cv::putText(frame,"DETECTED",cv::Point(30,50),cv::FONT_HERSHEY_SIMPLEX,1.0,cv::Scalar(0,255,0),2);
}