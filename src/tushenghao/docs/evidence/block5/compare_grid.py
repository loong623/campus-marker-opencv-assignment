"""只读对照Block4原480×32网格，共同实际结果与真值逐字段exact，不放epsilon。"""
import json
from itertools import zip_longest
from archive_checks import ROOT, EVIDENCE


def canonical(value):
    if isinstance(value, str) and value.isdecimal():
        return int(value)
    if isinstance(value, list):
        return [canonical(v) for v in value]
    if isinstance(value, dict):
        return {k: canonical(v) for k, v in value.items()}
    return value


old = ROOT / 'src/tushenghao/docs/evidence/block4/grid-final.jsonl'
new = EVIDENCE / 'runs/grid-01/frames.jsonl'
failures, count = [], 0
with old.open() as first, new.open() as second:
    for index, pair in enumerate(zip_longest(first, second)):
        assert None not in pair, 'grid frame counts differ'
        a, record = map(json.loads, pair)
        b = record['details']['experimental']
        fields = []
        for key in ['segment', 'sample', 'dt_ms', 'rate', 'known', 'noisy',
                    'r_experimental_px', 'deviation_experimental_px', 'truth_physical', 'raw_physical']:
            if canonical(a[key]) != canonical(b[key]):
                fields.append(key)
        if a['mode'] != ['translation', 'rotation'][b['mode']]:
            fields.append('mode')
        # 旧grid的FrameResult diagnostics固定为空，真实原因只在temporal载荷；
        # 新grid把同一两项原因镜像到结果。逐项核镜像来自旧实际原因，不能笼统排除diagnostics。
        first_result, second_result = dict(a['result']), dict(record['result'])
        expected = [a['temporal']['association']['reason'], a['temporal']['reset_or_fallback_reason']]
        if first_result['diagnostics'] == [] and second_result['diagnostics'] == expected:
            first_result['diagnostics'] = expected
        if canonical(first_result) != canonical(second_result):
            fields.append('result')
        if canonical(a['temporal']) != canonical(record['details']['temporal']):
            fields.append('temporal')
        if canonical(a['raw']) != canonical(record['result']['detections'][0]):
            fields.append('raw')
        if fields:
            failures.append({'index': index, 'fields': fields})
        count += 1
result = {'result': 'PASS' if not failures and count == 15360 else 'G-UNEXPECTED',
          'frames': count, 'segments': 480, 'failure_indices': failures,
          'excluded_fields': ['new schema identity, timing, metadata, events'],
          'adapter': 'old translation/rotation labels to enum0/1; decimal-string integers; old empty result.diagnostics adapted only when new two strings exactly equal original temporal reasons'}
with (EVIDENCE / 'comparison/grid-comparison.json').open('x') as stream:
    json.dump(result, stream, ensure_ascii=False, indent=2)
    stream.write('\n')
print(result['result'], count, 'frames', len(failures), 'differences')
raise SystemExit(bool(failures) or count != 15360)
