"""全帧检查empty/unknown隔离、稳定当前索引、原图偏离及bbox，不把864计数当正确率。"""
import json
import math
import struct
from archive_checks import EVIDENCE


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def geometry(detection):
    corners = detection['corners']
    assert len(corners) == 4 and len(set(map(tuple, corners))) == 4
    assert all(math.isfinite(x) and 0 <= x < 1440 and math.isfinite(y) and 0 <= y < 1080
               for x, y in corners)
    for i in range(4):
        a, b, c = [corners[(i + k) % 4] for k in range(3)]
        assert (b[0]-a[0])*(c[1]-b[1])-(b[1]-a[1])*(c[0]-b[0]) > 0
    xs, ys = zip(*corners)
    assert detection['bbox'] == [min(xs), min(ys), f32(max(xs)-min(xs)), f32(max(ys)-min(ys))]
    assert detection['confidence'] is None and detection['marker_code'] is None
    orientation = detection['orientation']
    if orientation is not None:
        assert sorted(orientation) == [0, 1, 2, 3]


counts = {'detected': 0, 'empty': 0, 'smoothing': 0}
maximum = 0.0
fps, dimensions = None, None
with (EVIDENCE / 'runs/app-debug-03/frames.jsonl').open() as stream:
    for index, line in enumerate(stream):
        record = json.loads(line)
        assert int(record['frame_id']) == index
        assert record['original_size'] == [1440, 1080]
        result = record['result']
        assert result['display'] is None
        if result['status'] == 2:
            counts['empty'] += 1
            assert result['detections'] == result['tracks'] == []
            continue
        assert result['status'] == 3 and len(result['tracks']) == 1
        counts['detected'] += 1
        for raw in result['detections']:
            geometry(raw)
        track = result['tracks'][0]
        raw = result['detections'][int(track['detection_index'])]
        stable = track['result']
        geometry(stable)
        for key in ['category', 'quality_flags', 'confidence', 'marker_code']:
            assert stable[key] == raw[key]
        diag = record['details']['temporal']
        mapping = diag['output_slot_mapping'] or list(range(4))
        for slot in range(4):
            maximum = max(maximum, math.dist(raw['corners'][slot], stable['corners'][mapping[slot]]))
        if raw['orientation'] is None:
            assert stable['orientation'] is None
        else:
            assert stable['orientation'] == [mapping[i] for i in raw['orientation']]
        if diag['used_smoothing']:
            counts['smoothing'] += 1
        else:
            assert stable == raw
assert index + 1 == 1676 and counts['detected'] == 864 and counts['empty'] == 812
assert maximum <= 2.0
report = {'result': 'PASS', 'frames': 1676, 'counts': counts,
          'max_current_deviation_px': maximum, 'approved_deviation_px': 2.0,
          'unknown_backfill': 0, 'empty_tracks': 0, 'confidence_and_marker_code': 'null',
          'original_size': [1440, 1080], 'size_evidence': 'every selected actual FrameRecord',
          'accuracy': None, 'note': '无人工标签，不报告召回/误检率'}
with (EVIDENCE / 'comparison/video-invariants.json').open('x') as stream:
    json.dump(report, stream, ensure_ascii=False, indent=2)
    stream.write('\n')
print('PASS video invariants', counts, maximum)
