"""Create the unlabelled sweep once. Run from the repository root.

Only writes the new fix2-sweep directory and the explicitly requested report.
Does not invoke project code or alter the sealed Path A evidence.
"""
import collections
import csv
import hashlib
import json
from pathlib import Path
import shlex
import subprocess

ROOT = Path.cwd()
OUT = ROOT / 'src/tushenghao/docs/evidence/final-fixes/fix2-sweep'
PATH_A = ROOT / 'src/tushenghao/docs/evidence/final-fixes/path-a'
SOURCE = PATH_A / 'verification-release/frames.jsonl'
VIDEO = ROOT / 'data/raw/marker_video.avi'
REPORT = ROOT / 'src/tushenghao/docs/fix2_sweep_report.md'
D_IDS = {153, 280, 382, 648, 1108, 1573}

def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()

def protected_snapshot():
    paths = set(p for p in PATH_A.rglob('*') if p.is_file())
    for extension in ('*.cpp', '*.hpp', '*.h', '*.cc', 'CMakeLists.txt', '*.cmake'):
        paths.update(p for p in (ROOT / 'src').rglob(extension) if p.is_file() and OUT not in p.parents)
    paths.add(VIDEO)
    return {str(p.relative_to(ROOT)): sha(p) for p in sorted(paths)}

def write_json(name, value):
    (OUT / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')

def main():
    if REPORT.exists() or (OUT / 'index.csv').exists():
        raise RuntimeError('refusing to overwrite existing report or manual index')
    protected = protected_snapshot()
    write_json('protected_inputs_before.json', protected)
    lines = SOURCE.read_text(encoding='utf-8').splitlines(keepends=True)
    rows = [json.loads(line) for line in lines]
    assert len(rows) == 1676
    assert [int(r['frame_id']) for r in rows] == list(range(1676))
    selected = [(line, row) for line, row in zip(lines, rows) if row['result_status'] == 2]
    assert len(selected) == 700
    entries = []
    for _, row in selected:
        frame_id = int(row['frame_id'])
        assert row['result']['status'] == 2 and not row['counts']['truncated']
        assert row['original_size'] == [1440, 1080]
        details = row['details']
        counts = {k: len(details[k]['hypotheses']) for k in ('generated', 'validated', 'completed')}
        for key, count in counts.items():
            assert count == int(row['counts'][key])
            assert not details[key]['truncated']
        diagnostics = row['result']['diagnostics']
        corner = [s for s in diagnostics if 'NO_VALID_ADJACENT_EDGES' in s]
        assignment = [s for s in diagnostics if s.startswith('assignment/') and 'INVALID_PARENT' in s]
        if frame_id in D_IDS:
            category = 'D'
            assert corner
            failures = corner + [s for s in diagnostics if 'UNRESOLVED_COMPETING_GEOMETRY' in s]
            reason = 'Path A 残留；' + '；'.join(failures)
        elif counts['generated'] == 0:
            category = 'A'
            assert counts['validated'] == counts['completed'] == 0
            assert 'no valid geometry hypothesis generated' in diagnostics
            reason = '无几何假设；no valid geometry hypothesis generated'
        elif counts['completed'] == 0:
            category = 'B'
            assert counts['validated'] > 0 and assignment
            assert all(f'assignment/{key}=0' in diagnostics for key in ('expansions', 'candidates_examined', 'output_branches'))
            reason = 'M/S 补全阶段未产出完整假设；' + '；'.join(assignment)
        elif corner:
            category = 'C'
            reason = '角点证据不足；' + '；'.join(corner)
        else:
            raise RuntimeError(f'unclassified frame {frame_id}')
        entries.append({'frame_id': frame_id, 'category': category, 'failure_reason': reason,
                        'image_path': f'frames/{category}/frame-{frame_id:06d}.png',
                        'hypothesis_counts': counts, 'diagnostics': diagnostics})
    counts = dict(sorted(collections.Counter(e['category'] for e in entries).items()))
    assert counts == {'A': 564, 'B': 24, 'C': 106, 'D': 6}
    assert {e['frame_id'] for e in entries if e['category'] == 'D'} == D_IDS
    write_json('classification.json', entries)
    (OUT / 'not_detected_frames.jsonl').write_text(''.join(line for line, _ in selected), encoding='utf-8')
    with (OUT / 'index.csv').open('x', encoding='utf-8-sig', newline='') as stream:
        writer = csv.writer(stream)
        writer.writerow(['帧号', '类别', '失败原因', '图片路径', '人工结论'])
        for e in entries:
            writer.writerow([e['frame_id'], e['category'], e['failure_reason'], e['image_path'], ''])
    targets = OUT / 'tools/export_targets.tsv'
    targets.write_text(''.join(f"{e['frame_id']}\t{e['image_path']}\n" for e in entries), encoding='utf-8')
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'opencv4'], text=True))
    binary = OUT / 'tools/export_frames'
    compile_run = subprocess.run(['g++', '-std=c++17', '-O2', str(OUT / 'tools/export_frames.cpp'), '-o', str(binary), *flags], capture_output=True, text=True)
    (OUT / 'tools/compile.log').write_text(compile_run.stdout + compile_run.stderr, encoding='utf-8')
    compile_run.check_returncode()
    export_run = subprocess.run([str(binary), str(VIDEO), str(targets), str(OUT)], capture_output=True, text=True)
    (OUT / 'tools/export.log').write_text(export_run.stdout + export_run.stderr, encoding='utf-8')
    export_run.check_returncode()
    export = json.loads(export_run.stdout)
    write_json('export_verification.json', export)
    binary.unlink()
    with (OUT / 'index.csv').open(encoding='utf-8-sig', newline='') as stream:
        index = list(csv.DictReader(stream))
    assert len(index) == 700 and all(r['人工结论'] == '' for r in index)
    assert len({r['帧号'] for r in index}) == 700
    expected_images = {e['image_path'] for e in entries}
    assert {str(p.relative_to(OUT)) for p in (OUT / 'frames').rglob('*.png')} == expected_images
    for category, count in counts.items():
        assert len(list((OUT / 'frames' / category).glob('*.png'))) == count
    REPORT.write_text('''# fix2-sweep：700 帧人工标注与根因分析

## 当前状态与依据

步骤 1–5 已完成；人工结论尚未填写，步骤 6–7 等用户完成标注后触发。
本报告当前只记录失败阶段和导出验证结果，不给出漏检真值、根因判断或修复结论。

诊断来源：`evidence/final-fixes/path-a/verification-release/frames.jsonl`，全视频 1676 帧，筛选 `result_status == 2`（NOT_DETECTED）得到 700 个唯一帧号。
图像来源：`data/raw/marker_video.avi`。按零基帧号从头顺序解码，导出原尺寸 1440×1080 PNG，无缩放、裁切或叠加；700 张 PNG 均重新读取并与对应解码原帧逐像素一致。

## 分类数量

| 类别 | 阻断阶段 / 分组定义 | 帧数 | 人工有（该检出） | 人工无（不该检出） | 人工结论 |
| --- | --- | ---: | --- | --- | --- |
| A | 无几何假设 | 564 | | | |
| B | M/S 补全阶段未产出完整假设 | 24 | | | |
| C | 角点证据不足：NO_VALID_ADJACENT_EDGES | 106 | | | |
| D | Path A 历史残留帧 | 6 | | | |
| 合计 | 互斥且覆盖全部 NOT_DETECTED 帧 | 700 | | | |

实际日志没有 `details.geometry` 字段，分类读取 `details.generated/validated/completed.hypotheses` 并与 `counts` 交叉核对。分类顺序为 D → A → B → C。

- D：固定帧号 153、280、382、648、1108、1573，优先从其他类别中排除；原始失败诊断保留在索引中。
- A：generated 假设数为 0；validated、completed 也为 0，日志包含 `no valid geometry hypothesis generated`。
- B：generated、validated 均有假设，completed 为 0。24 帧均记录 `assignment/0/INVALID_PARENT`，补全 expansions、candidates_examined、output_branches 均为 0。这是日志记录的阻断位置；具体为什么父假设无效、与 M/S 有何关系，留待人工标注后的代码分析。
- C：排除 D 后，已有完整假设，诊断包含 `NO_VALID_ADJACENT_EDGES`。索引保留具体角点及各证据计数。

## 人工标注入口

[打开 index.csv](evidence/final-fixes/fix2-sweep/index.csv)。五列为：帧号、类别、失败原因、图片路径、人工结论。

只填写“人工结论”列：`有（该检出）` 或 `无（不该检出）`；当前全部 700 行为空。图片路径相对于 index.csv 所在目录，按 `frames/A/`、`frames/B/`、`frames/C/`、`frames/D/` 存放。帧号从 0 开始，与诊断日志一致。保留帧号、类别和图片路径，便于逐帧对照。

原始选中诊断在 [not_detected_frames.jsonl](evidence/final-fixes/fix2-sweep/not_detected_frames.jsonl)，分类与计数在 [classification.json](evidence/final-fixes/fix2-sweep/classification.json)，导出核验在 [export_verification.json](evidence/final-fixes/fix2-sweep/export_verification.json)。

## 人工标注汇总（标注后填写）

## A 类根因分析（标注后填写）

## B 类根因分析（标注后填写）

## C 类根因分析（标注后填写）

## D 类根因分析（标注后填写）

## 全局代码只读核对（标注后填写）

分析时逐帧对齐人工真值、原图、候选与各阶段诊断；对“该检出”的帧追踪最早错误决策和代码位置，按真实根因细分，每个结论提供具体帧证据，并核对“不该检出”帧的误检风险。对证据不足的判断明确保留不确定性。

## 具体有效可执行修改方案（标注后填写，供用户审核）

''', encoding='utf-8')
    after = protected_snapshot()
    assert after == protected, 'protected Path A / code / video changed'
    write_json('protection_check.json', {'unchanged': True, 'checked_files': len(protected),
               'protected_inputs_manifest': 'protected_inputs_before.json',
               'path_a_unchanged': True, 'source_code_unchanged': True, 'video_unchanged': True})
    write_json('manifest.json', {'task': 'fix2-sweep', 'completed_steps': [1, 2, 3, 4, 5],
               'pending_steps': [6, 7], 'class_counts': counts, 'selected_frames': 700,
               'manual_labels_filled': 0, 'source_frames': str(SOURCE.relative_to(ROOT)),
               'source_frames_sha256': sha(SOURCE), 'video': str(VIDEO.relative_to(ROOT)),
               'video_sha256': sha(VIDEO), 'index_encoding': 'UTF-8 with BOM',
               'image_paths_relative_to': 'index.csv parent', 'category_precedence': ['D', 'A', 'B', 'C'],
               'report': str(REPORT.relative_to(ROOT)), 'export_verification': export,
               'protected_files_checked': len(protected)})
    artifacts = {str(p.relative_to(OUT)): sha(p) for p in sorted(OUT.rglob('*')) if p.is_file() and p.name != 'artifact_hashes.json'}
    write_json('artifact_hashes.json', {'files': artifacts, 'report_sha256': sha(REPORT),
               'note': 'Initial export hashes; index.csv is intentionally editable for human labels.'})
    print(json.dumps({'counts': counts, 'images': 700, 'labels_filled': 0,
                      'protected_files_unchanged': len(protected)}, ensure_ascii=False))

if __name__ == '__main__':
    main()
