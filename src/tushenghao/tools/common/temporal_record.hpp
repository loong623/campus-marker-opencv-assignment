// Block4专用JSONL，不引入Detector库；不存在的数值显式null，raw证据独立保留。
#pragma once
#include "pipeline/stabilize_stage.hpp"
#include <ostream>
#include <iomanip>
#include <sstream>
namespace audit {
inline std::string quote(const std::string& s) {
    std::ostringstream out;out<<'"';for(unsigned char c:s) {
        if(c=='"'||c=='\\')out<<'\\'<<c;
        else if(c<32)out<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<int(c);
        else out<<c;
    }out<<'"';return out.str();
}
template<class T> void optionalNumber(std::ostream& out,const std::optional<T>& n) {if(n)out<<*n;else out<<"null";}
template<class T,size_t N> void array(std::ostream& out,const std::array<T,N>& a) {out<<'[';for(size_t i=0;i<N;++i){if(i)out<<',';out<<a[i];}out<<']';}
template<class T,size_t N> void optionalArray(std::ostream& out,const std::optional<std::array<T,N>>& a) {if(a)array(out,*a);else out<<"null";}
inline void point(std::ostream& out,cv::Point2d p) {out<<'['<<p.x<<','<<p.y<<']';}
inline void detection(std::ostream& out,const mark::Detection& d) {
    out<<"{\"category\":"<<int(d.category)<<",\"quality_flags\":"<<d.quality_flags<<",\"corners\":[";
    for(size_t i=0;i<4;++i){if(i)out<<',';point(out,d.corners[i]);}
    out<<"],\"bbox\":["<<d.bbox.x<<','<<d.bbox.y<<','<<d.bbox.width<<','<<d.bbox.height<<"],\"orientation\":";
    optionalArray(out,d.attributes.orientation);out<<",\"confidence\":";optionalNumber(out,d.confidence);
    out<<",\"marker_code\":";
    if(d.attributes.marker_code)out<<"{\"scheme\":"<<quote(d.attributes.marker_code->scheme)<<",\"value\":"<<d.attributes.marker_code->value<<'}';
    else out<<"null";out<<'}';
}
inline void strings(std::ostream& out,const std::vector<std::string>& s) {out<<'[';for(size_t i=0;i<s.size();++i){if(i)out<<',';out<<quote(s[i]);}out<<']';}
inline void temporalDiagnostics(std::ostream& out,const mark::TemporalDiagnostics& d) {
    out<<"{\"dt_seconds\":";optionalNumber(out,d.dt_seconds);out<<",\"alpha\":";optionalNumber(out,d.alpha);
    out<<",\"used_smoothing\":"<<(d.used_smoothing?"true":"false")<<",\"fell_back\":"<<(d.fell_back?"true":"false")
       <<",\"reset_or_fallback_reason\":"<<quote(d.reset_or_fallback_reason)<<",\"association\":{\"current_index\":";
    optionalNumber(out,d.association.current_index);out<<",\"matched_history\":"<<(d.association.matched_history?"true":"false")
       <<",\"ambiguous\":"<<(d.association.ambiguous?"true":"false")<<",\"reason\":"<<quote(d.association.reason)
       <<"},\"correspondence\":{\"valid\":"<<(d.correspondence.valid?"true":"false")<<",\"reason\":"<<quote(d.correspondence.reason)<<",\"mapping\":";
    if(d.correspondence.valid)array(out,d.correspondence.current_to_previous);else out<<"null";
    out<<",\"energy\":";optionalArray(out,d.correspondence.energy);out<<",\"lower\":";optionalArray(out,d.correspondence.lower);
    out<<",\"upper\":";optionalArray(out,d.correspondence.upper);out<<"},\"output_slot_mapping\":";optionalArray(out,d.output_slot_mapping);out<<'}';
}
inline void frameResult(std::ostream& out,const mark::FrameResult& r) {
    out<<"{\"status\":"<<int(r.status)<<",\"detections\":[";
    for(size_t i=0;i<r.detections.size();++i){if(i)out<<',';detection(out,r.detections[i]);}
    out<<"],\"tracks\":[";for(size_t i=0;i<r.tracks.size();++i){if(i)out<<',';out<<"{\"detection_index\":"<<r.tracks[i].detection_index<<",\"result\":";detection(out,r.tracks[i].result);out<<'}';}
    out<<"],\"display\":";
    if(r.display_state)out<<"{\"value\":"<<quote(r.display_state->value)<<",\"source_frame_id\":"<<r.display_state->source_frame_id<<",\"age\":"<<r.display_state->age<<",\"held\":"<<(r.display_state->is_held?"true":"false")<<'}';
    else out<<"null";out<<",\"diagnostics\":";strings(out,r.diagnostics);out<<'}';
}
// 完整记录同次decode的原始观测证据，稳定输出不覆盖交点或支持弧。
inline void decodePayload(std::ostream& out,const mark::DecodeStageResult& r) {
    out<<"{\"status\":"<<int(r.status)<<",\"search_truncated\":"<<(r.search_truncated?"true":"false")<<",\"detections\":[";
    for(size_t i=0;i<r.detections.size();++i){if(i)out<<',';detection(out,r.detections[i]);}
    out<<"],\"measurements\":[";
    for(size_t m=0;m<r.measurements.size();++m) {
        if(m) out<<',';out<<'[';
        for(int c=0;c<4;++c){if(c)out<<',';const auto& e=r.measurements[m].evidence_[c];
            out<<"{\"physical\":"<<c<<",\"frame_id\":"<<e.frame_id_<<",\"component_id\":"<<e.component_id_
              <<",\"model_vertex_id\":"<<e.model_vertex_id_<<",\"model_edge_ids\":["<<e.model_edge_ids_[0]<<','<<e.model_edge_ids_[1]
              <<"],\"evidence_id\":"<<quote(e.stable_id_)<<",\"original_observation\":"<<(e.original_observation_?"true":"false")
              <<",\"truncated\":"<<(e.truncated_?"true":"false")<<",\"observed_segment_ids\":[";
            for(size_t i=0;i<e.observed_segment_ids_.size();++i){if(i)out<<',';out<<e.observed_segment_ids_[i];}
            out<<"],\"observed_segment_end_ids\":["<<e.observed_segment_end_ids_[0]<<','<<e.observed_segment_end_ids_[1]<<"],\"fitted_lines\":[";
            for(int a=0;a<2;++a){if(a)out<<',';auto fit=a==0?e.line_a_:e.line_b_;out<<'[';for(int i=0;i<4;++i){if(i)out<<',';out<<fit[i];}out<<']';}
            out<<"],\"finite_segments\":[";
            for(int a=0;a<2;++a){if(a)out<<',';const auto& segment=a==0?e.edge_segment_a_:e.edge_segment_b_;out<<'[';point(out,segment[0]);out<<',';point(out,segment[1]);out<<']';}
            out<<"],\"intersection\":";point(out,e.intersection_);
            out<<",\"line_mean_residual_px\":["<<e.line_mean_residual_px_[0]<<','<<e.line_mean_residual_px_[1]<<"],\"line_max_residual_px\":["<<e.line_max_residual_px_[0]<<','<<e.line_max_residual_px_[1]
              <<"],\"support_extension_px\":["<<e.support_extension_px_[0]<<','<<e.support_extension_px_[1]<<"],\"corner_error_px\":"<<e.corner_error_px_<<",\"support_arcs\":[";
            for(int a=0;a<2;++a){if(a)out<<',';out<<'[';for(size_t p=0;p<e.original_support_arcs_[a].size();++p){if(p)out<<',';point(out,e.original_support_arcs_[a][p]);}out<<']';}
            out<<"],\"turn_arc\":[";for(size_t p=0;p<e.original_turn_arc_.size();++p){if(p)out<<',';point(out,e.original_turn_arc_[p]);}out<<"]}";
        }out<<']';
    }
    out<<"],\"diagnostics\":";strings(out,r.diagnostics);out<<'}';
}
} // namespace audit
