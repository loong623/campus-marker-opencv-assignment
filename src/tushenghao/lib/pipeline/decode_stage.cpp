// 唯一阶段编排路径：原三L生成/验证→窄范围M/S补全→原图四角→语义→当前Detection。
// 原集成用工作图size校验原图角、无条件填方向且bbox空；这里分别修正这些输出约束。
#include "pipeline/decode_stage.hpp"
#include "preprocess/preprocess.hpp"
#include "geometry/geometry_observation.hpp"
#include "geometry/geometry_matcher.hpp"
#include "geometry/geometry_validation.hpp"
#include "geometry/geometry_assignment_completion.hpp"
#include "corners/corner_resolver.hpp"
#include "corners/semantic_resolver.hpp"
#include "corners/detection_validator.hpp"
#include "core/corner_budget.hpp"
#include <algorithm>
#include <set>
#include <sstream>
#include <iomanip>
namespace mark {
DecodeStageResult decodeStage(const PreparedFrame& frame,const GeometryBatch& batch,const MarkerGeometry& model,const CornerConfig& config) {
    DecodeStageResult result;result.diagnostics=batch.diagnostics_;result.search_truncated=batch.resource_truncated_;
    if(!batch.segmented_assignments_ready_||!config.observation_budget_) {
        result.diagnostics.push_back("DECODE_NOT_READY: M/S验证或G3/G4预算尚未就绪");return result;
    }
    validateCornerObservationBudget(config);
    result.status=Status::NOT_DETECTED;bool unresolved=false;
    for(size_t h=0;h<batch.hypotheses_.size();++h) {
        const auto& hypothesis=batch.hypotheses_[h];
        auto log=[&](const std::string& stage,const std::string& reason){result.diagnostics.push_back(stage+"/"+std::to_string(h)+"/"+reason);};
        if(hypothesis.completeness_==GeometryCompleteness::CLEARLY_INCOMPLETE) {log("corner","CLEARLY_INCOMPLETE");continue;}
        std::set<std::string> parts;
        for(const auto& a:hypothesis.assignments_) parts.insert(a.model_part_id_);
        if(parts!=std::set<std::string>{"L0","L2","L3","M1","S1a","S1b"}) {
            log("assignment","SEGMENTED_SUPPORT_INSUFFICIENT");unresolved=true;continue;
        }
        auto corner=resolveObservedCorners(frame,hypothesis,model,config);
        if(!corner.measurement_) {log("corner",corner.rejection_reason_);unresolved=true;continue;}
        bool current_frame=true;
        for(const auto& e:corner.measurement_->evidence_) current_frame=current_frame&&e.frame_id_==frame.frame_id_;
        if(!current_frame) {log("validator","EVIDENCE_FRAME_MISMATCH");unresolved=true;continue;}
        auto order=orderScreenCorners(corner.measurement_->physical_corners_,config);
        if(!order.screen_order_) {log("screen",order.rejection_reason_);unresolved=true;continue;}
        auto valid=validateDetectionGeometry(*corner.measurement_,*order.screen_order_,frame.original_image_.size(),config);
        if(!valid.valid_) {log("validator",valid.rejection_reason_);unresolved=true;continue;}
        result.measurements.push_back(*corner.measurement_);
    }
    // 不能因一个竞争假设取证失败，就将剩下一个强行声明为唯一方向/几何。
    if(unresolved&&!result.measurements.empty()) {
        result.diagnostics.push_back("UNRESOLVED_COMPETING_GEOMETRY: 存在无法排除的失败候选");return result;
    }
    auto semantics=resolveSemantics(result.measurements,result.search_truncated,config);
    if(!semantics.geometry_consistent_) {result.diagnostics.push_back("semantic/"+semantics.rejection_reason_);return result;}
    for(const auto& measurement:semantics.retained_measurements_) {
        auto order=orderScreenCorners(measurement.physical_corners_,config);
        if(!order.screen_order_) throw std::logic_error("validated measurement lost screen order");
        Detection detection{};detection.category=MarkCategory::Unknown;
        float xmin=INFINITY,ymin=INFINITY,xmax=-INFINITY,ymax=-INFINITY;
        for(int i=0;i<4;++i) {
            detection.corners[i]=cv::Point2f(order.screen_order_->screen_points_[i]);
            xmin=std::min(xmin,detection.corners[i].x);xmax=std::max(xmax,detection.corners[i].x);
            ymin=std::min(ymin,detection.corners[i].y);ymax=std::max(ymax,detection.corners[i].y);
        }
        // 浮点min/max外接框不额外增加一个像素，保持原图四点边界语义。
        detection.bbox={xmin,ymin,xmax-xmin,ymax-ymin};
        if(semantics.orientation_unique_) detection.attributes.orientation=order.screen_order_->physical_to_screen_;
        result.detections.push_back(detection);
    }
    if(!result.detections.empty()) result.status=Status::DETECTED;
    return result;
}
DecodeStageResult runDecodePipeline(const FrameInput& input,const DetectorConfig& config,const MarkerGeometry& model) {
    DecodeStageResult result;
    if(input.image.empty()||input.image.dims!=2||input.image.type()!=CV_8UC3||input.timestamp_us<0||input.time_source!=TimestampSource::Unknown) {result.status=Status::INVALID_INPUT;result.diagnostics.push_back("INPUT_FORMAT");return result;}
    if(!config.assignment_completion_||!config.corner_.observation_budget_) {
        result.diagnostics.push_back("PIPELINE_NOT_READY: assignment及原图定位预算未配置");return result;
    }
    auto prepared=preprocess(input,config.preprocess);
    auto components=extractWhiteComponents(prepared,config.geometry_);prepared.components_=components;
    auto observations=observeShapes(components,config.geometry_);
    auto batch=generateGeometryHypotheses(observations,model,config.geometry_);
    batch=validateGeometryBatch(batch,model,components,config.geometry_);
    batch.diagnostics_.push_back("geometry/components="+std::to_string(components.size())+"/hypotheses="+std::to_string(batch.hypotheses_.size()));
    // 保存实际三L父变换/绑定以定位上游反例；不重估、不改变其算法或参数。
    for(size_t i=0;i<batch.hypotheses_.size();++i) {
        const auto& h=batch.hypotheses_[i];std::ostringstream trace;trace<<std::setprecision(17)<<"geometry/parent="<<i<<"/affine=";
        if(h.affine_transform_.rows==2&&h.affine_transform_.cols==3&&h.affine_transform_.type()==CV_64F)
            for(int row=0;row<2;++row)for(int col=0;col<3;++col)trace<<h.affine_transform_.at<double>(row,col)<<',';
        trace<<"/assignments=";for(const auto& a:h.assignments_)trace<<a.model_part_id_<<':'<<a.component_id_<<',';
        trace<<"/residual="<<h.validation_residual_<<"/completeness="<<int(h.completeness_);batch.diagnostics_.push_back(trace.str());
    }
    batch=completeSegmentedAssignments(batch,components,model,*config.assignment_completion_);
    return decodeStage(prepared,batch,model,config.corner_);
}
}
