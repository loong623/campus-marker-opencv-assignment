#include "detector.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
struct WhitePart{
    std::vector<cv::Point> contour;
    cv::Rect box;
    cv::Point2f center;
    double size;
};
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
// distance between two points
static double pointDistance(const cv::Point2f& a,const cv::Point2f& b){
    double dx=a.x-b.x;
    double dy=a.y-b.y;
    return std::sqrt(dx*dx+dy*dy);
}
// check shape
static bool validGeometry(const Quad& q){
    double top=pointDistance(q[0],q[1]);
    double right=pointDistance(q[1],q[2]);
    double bottom=pointDistance(q[2],q[3]);
    double left=pointDistance(q[3],q[0]);
    if(top<15||right<15||bottom<15||left<15){
        return false;
    }
    double width=(top+bottom)/2.0;
    double height=(left+right)/2.0;
    double ratio=std::max(width,height)/std::min(width,height);
    if(ratio>1.8){
        return false;
    }
    double horizontalRatio=std::max(top,bottom)/std::min(top,bottom);
    double verticalRatio=std::max(left,right)/std::min(left,right);
    if(horizontalRatio>2.5||verticalRatio>2.5){
        return false;
    }
    return true;
}
// get white marker areas
static cv::Mat getWhiteMask(const cv::Mat& frame){
    cv::Mat hsv;
    cv::cvtColor(frame,hsv,cv::COLOR_BGR2HSV);
    cv::Mat mask;
    cv::inRange(
        hsv,
        cv::Scalar(0,0,150),
        cv::Scalar(180,130,255),
        mask
    );
    cv::Mat kernel=cv::getStructuringElement(cv::MORPH_RECT,cv::Size(3,3));
    cv::morphologyEx(mask,mask,cv::MORPH_OPEN,kernel);
    cv::morphologyEx(mask,mask,cv::MORPH_CLOSE,kernel);
    return mask;
}
// check whether two white parts belong together
static bool closeParts(const WhitePart& a,const WhitePart& b){
    double distance=pointDistance(a.center,b.center);
    double scale=std::max(a.size,b.size);
    double threshold=std::max(25.0,4.0*scale);
    return distance<threshold;
}
// make one quadrilateral from a group
static bool makeQuad(const std::vector<WhitePart>& parts,const std::vector<int>& ids,Quad& quad){
    std::vector<cv::Point> points;
    for(int id:ids){
        for(const auto& p:parts[id].contour){
            points.push_back(p);
        }
    }
    if(points.size()<4){
        return false;
    }
    std::vector<cv::Point> hull;
    cv::convexHull(points,hull);
    double perimeter=cv::arcLength(hull,true);
    bool found=false;
    for(double ratio=0.02;ratio<=0.08;ratio+=0.01){
        std::vector<cv::Point> approximation;
        cv::approxPolyDP(hull,approximation,ratio*perimeter,true);
        if(approximation.size()==4&&cv::isContourConvex(approximation)){
            for(int i=0;i<4;i++){
                quad[i]=cv::Point2f(
                    static_cast<float>(approximation[i].x),
                    static_cast<float>(approximation[i].y)
                );
            }
            found=true;
            break;
        }
    }
    if(!found){
        cv::RotatedRect rect=cv::minAreaRect(points);
        if(rect.size.width<15||rect.size.height<15){
            return false;
        }
        cv::Point2f box[4];
        rect.points(box);
        for(int i=0;i<4;i++){
            quad[i]=box[i];
        }
    }
    quad=orderCorners(quad);
    return validGeometry(quad);
}
// warp binary marker
static cv::Mat warpMarker(const cv::Mat& mask,const Quad& corners,int size=200){
    std::vector<cv::Point2f> src(corners.begin(),corners.end());
    std::vector<cv::Point2f> dst={
        cv::Point2f(0,0),
        cv::Point2f(size-1,0),
        cv::Point2f(size-1,size-1),
        cv::Point2f(0,size-1)
    };
    cv::Mat transform=cv::getPerspectiveTransform(src,dst);
    cv::Mat warped;
    cv::warpPerspective(mask,warped,transform,cv::Size(size,size),cv::INTER_NEAREST);
    return warped;
}
// score marker structure
static double markerScore(const cv::Mat& marker){
    if(marker.empty()){
        return -1.0;
    }
    double whiteRatio=static_cast<double>(cv::countNonZero(marker))/static_cast<double>(marker.total());
    if(whiteRatio<0.05||whiteRatio>0.45){
        return -1.0;
    }
    int size=marker.rows;
    int cornerSize=static_cast<int>(size*0.45);
    cv::Rect lt(0,0,cornerSize,cornerSize);
    cv::Rect rt(size-cornerSize,0,cornerSize,cornerSize);
    cv::Rect rb(size-cornerSize,size-cornerSize,cornerSize,cornerSize);
    cv::Rect lb(0,size-cornerSize,cornerSize,cornerSize);
    auto ratio=[&marker](const cv::Rect& area){
        return static_cast<double>(cv::countNonZero(marker(area)))/static_cast<double>(area.area());
    };
    double rLT=ratio(lt);
    double rRT=ratio(rt);
    double rRB=ratio(rb);
    double rLB=ratio(lb);
    if(rLT<0.04||rRT<0.04||rRB<0.04||rLB<0.04){
        return -1.0;
    }
    int centerStart=static_cast<int>(size*0.35);
    int centerSize=static_cast<int>(size*0.30);
    cv::Rect centerArea(centerStart,centerStart,centerSize,centerSize);
    double centerRatio=ratio(centerArea);
    if(centerRatio>0.20){
        return -1.0;
    }
    cv::Mat clean=marker.clone();
    cv::Mat kernel=cv::getStructuringElement(cv::MORPH_RECT,cv::Size(3,3));
    cv::morphologyEx(clean,clean,cv::MORPH_OPEN,kernel);
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(clean,contours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_SIMPLE);
    int partCount=0;
    for(const auto& contour:contours){
        if(cv::contourArea(contour)>30){
            partCount++;
        }
    }
    if(partCount<3||partCount>10){
        return -1.0;
    }
    double cornerAverage=(rLT+rRT+rRB+rLB)/4.0;
    double minCorner=std::min(std::min(rLT,rRT),std::min(rRB,rLB));
    double maxCorner=std::max(std::max(rLT,rRT),std::max(rRB,rLB));
    double balance=1.0-(maxCorner-minCorner);
    double darkCenter=1.0-centerRatio;
    double countScore=1.0-std::min(std::abs(partCount-6)/6.0,1.0);
    double score=2.0*cornerAverage+minCorner+balance+darkCenter+countScore;
    return score;
}
// detect marker
bool detectMarker(const cv::Mat& frame,Quad& bestCorners){
    if(frame.empty()){
        return false;
    }
    cv::Mat whiteMask=getWhiteMask(frame);
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(whiteMask.clone(),contours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_SIMPLE);
    double frameArea=static_cast<double>(frame.rows*frame.cols);
    std::vector<WhitePart> parts;
    for(const auto& contour:contours){
        double area=cv::contourArea(contour);
        if(area<frameArea*0.00001||area>frameArea*0.03){
            continue;
        }
        cv::Rect box=cv::boundingRect(contour);
        if(box.width<4||box.height<4){
            continue;
        }
        double ratio=static_cast<double>(std::max(box.width,box.height))/std::min(box.width,box.height);
        if(ratio>5.0){
            continue;
        }
        WhitePart part;
        part.contour=contour;
        part.box=box;
        part.center=cv::Point2f(
            box.x+box.width/2.0f,
            box.y+box.height/2.0f
        );
        part.size=std::max(box.width,box.height);
        parts.push_back(part);
    }
    if(parts.size()<3){
        return false;
    }
    std::vector<int> group(parts.size(),-1);
    int groupId=0;
    for(int i=0;i<static_cast<int>(parts.size());i++){
        if(group[i]!=-1){
            continue;
        }
        group[i]=groupId;
        bool changed=true;
        while(changed){
            changed=false;
            for(int j=0;j<static_cast<int>(parts.size());j++){
                if(group[j]!=-1){
                    continue;
                }
                for(int k=0;k<static_cast<int>(parts.size());k++){
                    if(group[k]==groupId&&closeParts(parts[j],parts[k])){
                        group[j]=groupId;
                        changed=true;
                        break;
                    }
                }
            }
        }
        groupId++;
    }
    double bestScore=-1.0;
    for(int g=0;g<groupId;g++){
        std::vector<int> ids;
        for(int i=0;i<static_cast<int>(group.size());i++){
            if(group[i]==g){
                ids.push_back(i);
            }
        }
        if(ids.size()<3||ids.size()>10){
            continue;
        }
        Quad candidate;
        if(!makeQuad(parts,ids,candidate)){
            continue;
        }
        std::vector<cv::Point2f> polygon(candidate.begin(),candidate.end());
        double area=std::abs(cv::contourArea(polygon));
        if(area<frameArea*0.0005||area>frameArea*0.35){
            continue;
        }
        cv::Mat warped=warpMarker(whiteMask,candidate);
        double score=markerScore(warped);
        if(score<0){
            continue;
        }
        double areaBonus=std::min(area/(frameArea*0.03),1.0);
        score+=0.2*areaBonus;
        if(score>bestScore){
            bestScore=score;
            bestCorners=candidate;
        }
    }
    return bestScore>2.7;
}
// smooth corners
void smoothCorners(Quad& current,const Quad& previous,const cv::Size& frameSize){
    double averageMovement=0.0;
    for(int i=0;i<4;i++){
        double dx=current[i].x-previous[i].x;
        double dy=current[i].y-previous[i].y;
        averageMovement+=std::sqrt(dx*dx+dy*dy);
    }
    averageMovement/=4.0;
    double threshold=0.12*std::max(frameSize.width,frameSize.height);
    if(averageMovement>threshold){
        return;
    }
    const double alpha=0.60;
    for(int i=0;i<4;i++){
        current[i].x=static_cast<float>(alpha*previous[i].x+(1.0-alpha)*current[i].x);
        current[i].y=static_cast<float>(alpha*previous[i].y+(1.0-alpha)*current[i].y);
    }
}
// draw result
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