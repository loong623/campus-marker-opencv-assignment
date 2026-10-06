"""先核全部归档，再限定四个任务目录清理，最后用临时验证程序重核同一manifest。"""
import datetime
import json
import shutil
import subprocess
import tempfile
import sys
from pathlib import Path
import time
from archive_checks import ROOT, EVIDENCE

independent = EVIDENCE / 'archive/final-archive-check-commands.json'
entries = []


def command(argv, log_name):
    """最终完整性命令独立记录，避免写commands.json导致manifest自引用hash失效。"""
    start = time.monotonic()
    now = datetime.datetime.now().astimezone().isoformat()
    with (EVIDENCE / 'archive' / log_name).open('x') as stream:
        run = subprocess.run(argv, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT)
    entries.append({'argv': argv, 'cwd': str(ROOT), 'started_at': now,
                    'ended_at': datetime.datetime.now().astimezone().isoformat(),
                    'wall_seconds': time.monotonic()-start, 'exit_code': run.returncode,
                    'log': 'archive/' + log_name})
    independent.write_text(json.dumps(entries, ensure_ascii=False, indent=2)+'\n')
    assert run.returncode == 0, (argv, run.returncode)


command(['python3', '-B', str(EVIDENCE / 'archive_checks.py'), 'manifest', '--report',
         str(EVIDENCE / 'archive/artifact-manifest.json')], 'final-archive-check-manifest.log')
verify = ROOT / 'build/block5/observability_verify'
command([str(verify), '--check-archive', str(EVIDENCE), '--report',
         str(EVIDENCE / 'archive/final-archive-check-before-cleanup.json')],
        'final-archive-check-before-cleanup.log')
checked = json.loads((EVIDENCE / 'archive/final-archive-check-before-cleanup.json').read_text())
assert checked['result'] == 'PASS'
# 临时复制仅用于清理后核验，不作为归档证据、没有新建额外build目录。
with tempfile.NamedTemporaryFile(prefix='block5-verifier-', dir='/tmp', delete=False) as temp:
    temporary = temp.name
shutil.copy2(verify, temporary)
paths = []
for name in ['block5-baseline', 'block5', 'block5-debug', 'block5-fixture']:
    path = ROOT / 'build' / name
    assert path.resolve() == path and path.parent == (ROOT / 'build').resolve()
    marker = json.loads((path / '.block5-task.json').read_text())
    assert marker['task'] == 'Block5-2026-10-06' and marker['directory'] == 'build/' + name
    assert marker['baseline'] == 'b9cccd4a86ac5959a8c67559cde1d399ba29e046'
    paths.append(path)
for path in paths:
    shutil.rmtree(path)
receipt = {'result': 'PASS', 'removed': [str(p.relative_to(ROOT)) for p in paths],
           'task_markers_validated': True, 'archive_before_cleanup': checked,
           'old_builds_retained': (ROOT / 'build/block3-H-v1').is_dir(),
           'input_retained': (ROOT / 'data/raw/marker_video.avi').is_file(),
           'legacy_debug_failure_preserved': 'comparison/archived-legacy-replay.json',
           'cleaned_at': datetime.datetime.now().astimezone().isoformat()}
(EVIDENCE / 'archive/final-archive-check-cleanup.json').write_text(
    json.dumps(receipt, ensure_ascii=False, indent=2)+'\n')
command([temporary, '--check-archive', str(EVIDENCE), '--report',
         str(EVIDENCE / 'archive/final-archive-check-after-cleanup.json')],
        'final-archive-check-after-cleanup.log')
Path(temporary).unlink()
entries.append({'argv': ['python3', '-B', str(EVIDENCE / 'finalize_archive.py')], 'exit_code': 0, 'condition': 'all above checks passed; task-scoped filesystem cleanup receipt preserved'})
independent.write_text(json.dumps(entries, ensure_ascii=False, indent=2)+'\n')
print('PASS archive checked before/after cleanup; four task-owned build directories removed')
