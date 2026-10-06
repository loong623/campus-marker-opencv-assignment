"""Summarize read-only replay evidence and the user's category-level labels."""
import collections
import csv
import hashlib
import json
import math
from pathlib import Path

ROOT = Path.cwd()
OUT = ROOT / 'src/tushenghao/docs/evidence/final-fixes/fix2-sweep'

def read_json(name):
    return json.loads((OUT / name).read_text())

def lines(path):
    return [json.loads(x) for x in path.open()]

def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()

def write_json(name, value):
    (OUT / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n')

def main():
    source = ROOT / 'src/tushenghao/docs/evidence/final-fixes/path-a/verification-release/frames.jsonl'
    original = lines(source)
    baseline = lines(OUT / 'full_frame_replay.jsonl')
    candidate = lines(OUT / 'candidate_frame_replay.jsonl')
    sensitivities = lines(OUT / 'corner_sensitivity.jsonl')
    classifications = read_json('classification.json')
    classes = {r['frame_id']: r['category'] for r in classifications}
    assert len(original) == len(baseline) == len(candidate) == 1676
    assert len(sensitivities) == 112
    assert {r['frame_id'] for r in sensitivities} == {i for i, c in classes.items() if c in 'CD'}
    with (OUT / 'index.csv').open(encoding='utf-8-sig', newline='') as f:
        index = list(csv.DictReader(f))
    assert len(index) == 700
    assert all(r['人工结论'] == ('有（该检出）' if r['类别'] in 'CD' else '无（不该检出）') for r in index)
    deltas, orientation_changes, roots, mechanisms = [], 0, [], collections.Counter()
    for i, (logged, b, c) in enumerate(zip(original, baseline, candidate)):
        assert int(logged['frame_id']) == b['frame_id'] == c['frame_id'] == i
        old, new = b['baseline'], c['candidate']
        assert old['detected'] == bool(logged['result']['detections'])
        assert old['measurements'] == int(logged['counts']['measurements'])
        assert len(old['detections']) == len(logged['result']['detections'])
        for measured, recorded in zip(old['detections'], logged['result']['detections']):
            assert measured['corners'] == recorded['corners']
            assert measured['orientation'] == recorded['orientation']
        if classes.get(i) in ('C', 'D'):
            assert new['detected'] and not new['failures']
        if classes.get(i) in ('A', 'B'):
            assert not new['detected']
        if old['detected']:
            assert new['detected']
            delta = max(math.dist(p, q) for p, q in zip(old['detections'][0]['corners'], new['detections'][0]['corners']))
            if delta:
                deltas.append({'frame_id': i, 'max_corner_delta_px': delta})
            orientation_changes += old['detections'][0]['orientation'] != new['detections'][0]['orientation']
    for r in sensitivities:
        i = r['frame_id']
        diagnostic = next(s for s in original[i]['result']['diagnostics'] if s.startswith(f"corner/{r['hypothesis']}/NO_VALID_ADJACENT_EDGES:"))
        assert diagnostic == f"corner/{r['hypothesis']}/{r['reason']}: P{r['corner']}"
        assert not r['baseline_success']
        mechanism = ('缺少一种指定边的合格支持弧' if min(r['arc_masks'][:2]) == 0 else
                     '两种边均有候选但连接预算全部拒绝' if r['pre_final_gate_pairs'] == 0 else
                     '进入交点检查后被延伸/交点预算拒绝')
        mechanisms[mechanism] += 1
        v = next(v for v in r['sensitivity'] if v['variant'] == 'residual_0.75')
        root = 'R1：残差预算单独放宽即可恢复' if v['success'] else 'R2：模型有限边位置预算边界拒绝'
        assert v['success'] or i == 784
        roots.append({'帧号': i, '类别': classes[i], '人工结论': '有（该检出）', '首个失败候选': r['hypothesis'],
                      '首个失败角': f"P{r['corner']}", '组件ID': r['component_id'], '基线拒绝机制': mechanism,
                      '根因分组': root, '原预算指定边弧数量': json.dumps(r['arc_masks']),
                      '原预算进入交点检查的边对数': r['pre_final_gate_pairs'],
                      '只调整残差0.75的整帧检出': baseline[i]['residual_0_75']['detected'],
                      '残差0.75位置9.5的整帧检出': candidate[i]['candidate']['detected'],
                      '四角合格候选数': candidate[i]['candidate']['measurements'],
                      '图片路径': next(x['image_path'] for x in classifications if x['frame_id'] == i),
                      '原始失败原因': diagnostic})
    with (OUT / 'root_cause_by_frame.csv').open('w', encoding='utf-8-sig', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=list(roots[0]));writer.writeheader();writer.writerows(roots)
    variants = {v['variant']: sum(next(x for x in r['sensitivity'] if x['variant'] == v['variant'])['success'] for r in sensitivities)
                for v in sensitivities[0]['sensitivity']}
    baseline_ms = sum(r['baseline']['elapsed_ms'] for r in baseline)
    candidate_ms = sum(r['candidate']['elapsed_ms'] for r in candidate)
    retry_ms = sum(r['candidate']['elapsed_ms'] for r in candidate if classes.get(r['frame_id']) in ('C', 'D'))
    residuals = [max(r['sensitivity'][0]['mean_residuals']) for r in sensitivities if r['sensitivity'][0]['success']]
    summary = {'frames_checked': 1676, 'baseline_public_raw_geometry_matches_original': True,
               'baseline_detected': 976, 'manual_should_detect': 112, 'manual_should_not_detect': 588,
               'baseline_first_corner_diagnostic_matches': 112, 'residual_only_recovered': 111,
               'residual_only_remaining': [784], 'candidate_detected': 1088, 'candidate_not_detected': 588,
               'candidate_recovered_C': 106, 'candidate_recovered_D': 6, 'old_success_lost': 0,
               'negative_AB_false_positives': 0, 'root_counts': {'R1': 111, 'R2': 1},
               'baseline_rejection_mechanisms': dict(mechanisms), 'single_corner_sensitivity_success_counts': variants,
               'R1_selected_fit_max_mean_residual_range_px': [min(residuals), max(residuals)],
               'global_relaxation_old_success_corner_changes': len(deltas),
               'global_relaxation_max_old_corner_change_px': max(x['max_corner_delta_px'] for x in deltas),
               'global_relaxation_old_orientation_changes': orientation_changes,
               'changed_old_frames': deltas, 'baseline_decode_replay_sum_ms': baseline_ms,
               'candidate_decode_replay_sum_ms': candidate_ms, 'offline_global_cost_ratio': candidate_ms / baseline_ms,
               'estimated_baseline_plus_CD_retry_cost_ratio': (baseline_ms + retry_ms) / baseline_ms,
               'cost_caveat': 'Single-thread offline replay processes overlapped; ratios are illustrative, not a production benchmark.',
               'scope': 'Sequential video decoding and frozen completed hypotheses; unchanged production corner, validator, semantic and float publication functions. No upstream regeneration, temporal replay, app rendering, precision ground truth or deployed fix.'}
    write_json('analysis_summary.json', summary)
    protected = read_json('protected_inputs_before.json')
    for relative, expected in protected.items():
        assert sha(ROOT / relative) == expected, relative
    write_json('analysis_protection_check.json', {'unchanged': True, 'checked_files': len(protected),
               'path_a_unchanged': True, 'source_code_unchanged': True, 'video_unchanged': True,
               'reference': 'protected_inputs_before.json'})
    write_json('analysis_manifest.json', {'completed_steps': [1, 2, 3, 4, 5, 6, 7], 'pending_steps': [],
               'production_modifications': False, 'human_label_source': 'manual_label_authorization.json',
               'source_frames_sha256': sha(source), 'source_configuration': 'src/tushenghao/docs/evidence/final-fixes/path-a/verification-release/effective_config.yaml',
               'candidate_parameters_in_memory_only': {'max_line_fit_error': 0.75, 'max_edge_position_distance_px': 9.5},
               'replay_status': 'completed', 'implementation_status': 'proposal_for_user_review',
               'historical_initial_export_files': ['manifest.json', 'artifact_hashes.json'],
               'current_summary': 'analysis_summary.json'})
    print(json.dumps({k: v for k, v in summary.items() if k != 'changed_old_frames'}, ensure_ascii=False, indent=2))

if __name__ == '__main__':
    main()
