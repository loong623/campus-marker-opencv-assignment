"""只读核对旧阶段记录；保持原证据、原因和时序载荷，不以数量代替逐帧比较。"""
import argparse
import hashlib
import json
import math
import struct
from pathlib import Path


def physical(d):
    return [d['corners'][i] for i in d['orientation']] if d['orientation'] is not None else None


def float_point(p):
    return [struct.unpack('<f', struct.pack('<f', x))[0] for x in p]


def compare_detection(a, b, route_b):
    if a == b:
        return None
    if not route_b:
        return 'detection changed'
    if any(a[k] != b[k] for k in a if k not in ('corners', 'orientation')):
        return 'non-permutation detection field changed'
    if sorted(a['corners']) != sorted(b['corners']) or physical(a) != physical(b):
        return 'point set or physical mapping changed'
    if (a['orientation'] is None) != (b['orientation'] is None):
        return 'orientation availability changed'
    for k in range(4):
        if a['corners'][k:] + a['corners'][:k] == b['corners']:
            return 'APPROVED_PERMUTATION'
    return 'non-cyclic permutation'


def compare(old_path, new_path, report_path, route_b):
    old = [json.loads(line) for line in old_path.open()]
    new = [json.loads(line) for line in new_path.open()]
    failures, permutations = [], []
    if len(old) != 1676 or len(new) != 1676:
        failures.append({'reason': 'expected 1676 frames', 'counts': [len(old), len(new)]})
    for i, (a, b) in enumerate(zip(old, new)):
        errors = []
        for k in ('frame_id', 'timestamp_us', 'time_source', 'timestamp_recipe', 'fps', 'original_size',
                  'input_sha256', 'config_sha256', 'model_sha256'):
            if a[k] != b[k]:
                errors.append(k)
        if a['frame_id'] != i or b['frame_id'] != i:
            errors.append('non-contiguous frame IDs')
        for k in ('status', 'search_truncated', 'measurements', 'diagnostics'):
            if a['decode'][k] != b['decode'][k]:
                errors.append('decode/' + k)
        for label, x, y in [('raw', a['decode']['detections'], b['decode']['detections']),
                            ('final raw', a['finalized']['detections'], b['finalized']['detections']),
                            ('stable', [t['result'] for t in a['finalized']['tracks']],
                             [t['result'] for t in b['finalized']['tracks']])]:
            if len(x) != len(y):
                errors.append(label + ' count changed')
            for j, (d, e) in enumerate(zip(x, y)):
                reason = compare_detection(d, e, route_b)
                if reason == 'APPROVED_PERMUTATION':
                    permutations.append({'frame_id': i, 'layer': label, 'index': j,
                                         'old_corners': d['corners'], 'new_corners': e['corners'],
                                         'old_orientation': d['orientation'], 'new_orientation': e['orientation']})
                elif reason:
                    errors.append(label + ': ' + reason)
        for k in a['finalized']:
            if k not in ('detections', 'tracks') and a['finalized'][k] != b['finalized'][k]:
                errors.append('finalized/' + k)
        if [t['detection_index'] for t in a['finalized']['tracks']] != [t['detection_index'] for t in b['finalized']['tracks']]:
            errors.append('current track reference changed')
        if a['temporal'] != b['temporal']:
            errors.append('temporal details changed')
        if b['input_sha256'] != 'aa1219a7a7b702ea1265be8752853a267c982a651afa0f7846f516f9517c9ac7':
            errors.append('wrong video')
        if errors:
            failures.append({'frame_id': i, 'reasons': errors})
    report = {'result': 'G-UNEXPECTED' if failures else 'PASS', 'records': [len(old), len(new)],
              'raw_detected': sum(bool(x['decode']['detections']) for x in new),
              'raw_empty': sum(not x['decode']['detections'] for x in new),
              'failures': failures, 'approved_permutations': permutations,
              'excluded_fields': ['commit_label', 'code_sha256', 'effective_config_sha256', 'budget_version', 'integration'],
              'input_files': [{'path': str(p), 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()} for p in (old_path, new_path)]}
    with report_path.open('x') as out:
        json.dump(report, out, ensure_ascii=False, indent=2)
        out.write('\n')
    print(report['result'], 'frames', len(new), 'failures', len(failures), 'permutations', len(permutations))
    return 1 if failures else 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('old', type=Path)
    parser.add_argument('new', type=Path)
    parser.add_argument('report', type=Path)
    parser.add_argument('--route-b', action='store_true')
    args = parser.parse_args()
    raise SystemExit(compare(args.old, args.new, args.report, args.route_b))
