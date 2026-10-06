#include "pipeline/stabilize_stage.hpp"
#include <stdexcept>
namespace mark {
FrameResult finalizeDecodedFrame(const DecodeStageResult& decoded,const FrameStamp& stamp,cv::Size size,
                                 TemporalStabilizer& temporal,DisplayHistory& display) {
    FrameResult result{};result.frame_id=stamp.frame_id;result.timestamp_us=stamp.timestamp_us;
    result.status=decoded.status;result.diagnostics=decoded.diagnostics;
    if(decoded.status==Status::NOT_READY || decoded.status==Status::INVALID_INPUT) {
        temporal.reset(ResetReason::InvalidSequence);display.reset(ResetReason::InvalidSequence);return result;
    }
    // 内部编排不一致须显式失败；不能拿平滑掩盖status与原始集合的矛盾。
    if((decoded.status==Status::DETECTED)!=!decoded.detections.empty()) {
        temporal.reset(ResetReason::InvalidSequence);display.reset(ResetReason::InvalidSequence);
        throw std::logic_error("DECODE_STATUS_PAYLOAD_MISMATCH");
    }
    auto stable=temporal.update(decoded.detections,stamp,size);
    result.status=stable.status;
    result.diagnostics.push_back(stable.diagnostics.association.reason);
    result.diagnostics.push_back(stable.diagnostics.reset_or_fallback_reason);
    if(result.status==Status::INVALID_INPUT) {
        temporal.reset(ResetReason::InvalidSequence);display.reset(ResetReason::InvalidSequence);return result;
    }
    if(stable.diagnostics.reset_or_fallback_reason=="INPUT_CHANGED") display.reset(ResetReason::InputChanged);
    result.detections=decoded.detections;result.tracks=std::move(stable.tracks);
    result.display_state=display.update(std::nullopt,stamp);
    return result;
}
}
