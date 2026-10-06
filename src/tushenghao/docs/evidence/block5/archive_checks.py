"""归档补充核验：链接、来源和采样覆盖；C++ verify另核JSON语法/schema/hash。"""
import argparse
import hashlib
import json
import re
from pathlib import Path

EVIDENCE = Path(__file__).resolve().parent
ROOT = EVIDENCE.parents[4]


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def report_yaml(path):
    """只读取本项目生成的标量报告，配置合法性由C++严格loader核验。"""
    result, stack = {}, []
    for line in path.read_text().splitlines():
        if not line.strip() or line.lstrip().startswith(('#', '%', '---')):
            continue
        indent = len(line) - len(line.lstrip())
        # 引号key中无冒号；仅受控报告，非通用YAML解析器。
        key, value = line.strip().split(':', 1)
        key = json.loads(key) if key.startswith('"') else key
        while stack and stack[-1][0] >= indent:
            stack.pop()
        if not value.strip():
            stack.append((indent, key))
            continue
        text = value.strip()
        result['.'.join([p[1] for p in stack] + [key])] = (
            json.loads(text) if text.startswith('"') else text)
    return result


def validate():
    snapshot = json.loads((EVIDENCE / 'environment/final_source_manifest.json').read_text())
    mismatches = [p for p, value in snapshot['source_sha256'].items()
                  if digest(ROOT / p) != value]
    assert not mismatches, mismatches
    sources = set()
    src = ROOT / 'src/tushenghao'
    for directory, patterns in [('lib', ['*.cpp', '*.hpp']), ('include', ['*.hpp']),
                                ('app', ['*.cpp', '*.hpp']), ('tools/validation', ['*.cpp']),
                                ('tools/audit', ['*.cpp']), ('tools/common', ['*.cpp', '*.hpp'])]:
        for pattern in patterns:
            sources.update((src / directory).rglob(pattern))
    sources.add(src / 'CMakeLists.txt')
    code = hashlib.sha256(''.join(digest(p) for p in sorted(sources)).encode()).hexdigest()
    assert code == report_yaml(EVIDENCE / 'runs/app-baseline-03/manifest.yaml')['code_sha256']
    # 冻结ref/历史证据不修改；核查本轮新增导航及其直接本地链接。
    docs = [ROOT / 'src/tushenghao/README.md', ROOT / 'src/tushenghao/docs/INDEX.md',
            ROOT / 'src/tushenghao/tools/INDEX.md', ROOT / 'src/tushenghao/docs/evidence/INDEX.md',
            ROOT / 'src/tushenghao/docs/diagnostics_schema.md',
            ROOT / 'src/tushenghao/docs/block5_acceptance.md', EVIDENCE / 'INDEX.md']
    broken, links = [], 0
    for doc in docs:
        for target in re.findall(r'\]\(([^)]+)\)', doc.read_text()):
            if '://' in target or target.startswith('#'):
                continue
            target = target.strip('<>').split('#', 1)[0]
            if not target:
                continue
            links += 1
            if not (doc.parent / target).exists():
                broken.append({'document': str(doc.relative_to(ROOT)), 'target': target})
    assert not broken, broken
    runs = []
    for name in ['app-baseline-03', 'app-debug-03', 'geometry-all-01',
                 'decode-all-01', 'temporal-all-01', 'grid-01']:
        run = EVIDENCE / 'runs' / name
        manifest = report_yaml(run / 'manifest.yaml')
        summary = report_yaml(run / 'summary.yaml')
        n = int(summary['submitted'])
        assert n == (15360 if name == 'grid-01' else 1676), (name, n)
        assert summary['failed'] == summary['incomplete'] == 'false', name
        assert not (run / 'FAILED.json').exists(), name
        ids = []
        records = run / 'frames.jsonl'
        if records.exists():
            with records.open() as stream:
                for line in stream:
                    record = json.loads(line)
                    ids.append(int(record['frame_id']))
            assert ids == list(range(n)), (name, 'missing/reordered frame')
        else:
            assert name == 'app-baseline-03' and summary['selected'] == '0', name
        assert manifest['code_sha256'] == report_yaml(
            EVIDENCE / 'runs/app-baseline-03/manifest.yaml')['code_sha256'], name
        runs.append({'run': name, 'submitted': n, 'selected': len(ids)})
    baseline = report_yaml(EVIDENCE / 'runs/app-baseline-03/summary.yaml')
    debug = report_yaml(EVIDENCE / 'runs/app-debug-03/summary.yaml')
    temporal = report_yaml(EVIDENCE / 'runs/temporal-all-01/summary.yaml')
    assert baseline['result_fingerprint'] == debug['result_fingerprint'] == temporal['result_fingerprint']
    assert baseline['result_status_counts.3'] == '864' and baseline['result_status_counts.2'] == '812'
    return {'result': 'PASS', 'source_files': len(snapshot['source_sha256']),
            'local_links_checked': links, 'runs': runs,
            'build_unique_evidence_dependency': False,
            'note': 'build路径仅为可重建命令；实际证据均位于本目录；H输入另有无损归档'}


def manifest():
    commands = json.loads((EVIDENCE / 'archive/commands.json').read_text())
    artifacts = []
    final_code = report_yaml(EVIDENCE / 'runs/app-baseline-03/manifest.yaml')['code_sha256']
    for path in sorted(EVIDENCE.rglob('*')):
        if not path.is_file() or path.name.startswith('.') or path.name == 'artifact-manifest.json':
            continue
        relative = path.relative_to(EVIDENCE).as_posix()
        if relative.startswith('archive/final-archive-check'):
            continue  # 独立核验manifest最终hash，不形成自引用链。
        command = next((c for c in reversed(commands) if c.get('log') == relative
                        or ('--report' in c.get('argv', []) and any(str(path).endswith(a) for a in c['argv'] if a.endswith('.json')))), None)
        if command is None and relative.startswith('runs/'):
            run_name = relative.split('/')[1]
            command = next((c for c in reversed(commands) if '--run-dir' in c.get('argv', [])
                            and any(a.endswith('/'+run_name) for a in c['argv'])), None)
        source = 'intermediate source; exact snapshot unavailable, retained failure/progress evidence'
        if relative.startswith('step0/original') or relative == 'step0/baseline.json':
            source = 'b9cccd4a86ac5959a8c67559cde1d399ba29e046'
        elif relative.startswith('step0/patched') or 'route_b' in relative:
            source = 'step0/patched_source_manifest.json'
        elif relative.startswith('runs/'):
            source = report_yaml(path.parents[0] / 'manifest.yaml').get('code_sha256', final_code) if (path.parents[0] / 'manifest.yaml').exists() else final_code
        elif relative.startswith(('environment/', 'archive/')) or 'final' in relative or 'frozen' in relative:
            source = final_code
        if command and command.get('started_at') and command['started_at'] >= '2026-10-06T13:03:13.800432+08:00':
            source = final_code
        if path.suffix in ['.py', '.cpp'] and relative.count('/') == 0:
            source = 'archived tool source: SHA256=' + digest(path) + '; final worktree=' + final_code
        artifacts.append({'path': relative, 'size': str(path.stat().st_size), 'sha256': digest(path),
                          'generation': command or {'command_registry': 'archive/commands.json',
                            'condition': 'run files: matching --run-dir command; docs/scripts/reports: recorded Codex filesystem edit or validation'},
                          'source_version': source})
    return {'schema_version': 1, 'artifacts': artifacts,
            'excluded_self_reference': ['archive/artifact-manifest.json', 'archive/final-archive-check*.json/log'],
            'final_code_sha256': final_code}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('operation', choices=['validate', 'manifest'])
    parser.add_argument('--report', required=True)
    args = parser.parse_args()
    output = Path(args.report)
    result = validate() if args.operation == 'validate' else manifest()
    with output.open('x') as stream:
        json.dump(result, stream, ensure_ascii=False, indent=2)
        stream.write('\n')
    print('PASS', args.operation)
