"""只在已标识的Block5构建目录编译零帧fixture，保留输入供归档重放。"""
import json
import shlex
import subprocess
from archive_checks import ROOT, EVIDENCE

flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'opencv4'], text=True))
binary = ROOT / 'build/block5/empty_video_fixture'
argv = ['g++', '-std=c++17', str(EVIDENCE / 'empty_video_fixture.cpp'), '-o', str(binary)] + flags
print(json.dumps({'compile_argv': argv}))
subprocess.run(argv, check=True)
video = EVIDENCE / 'environment/zero-frame.avi'
subprocess.run([str(binary), str(video)], check=True)
run = EVIDENCE / 'runs/negative-zero-frame'
argv = [str(ROOT / 'build/block5/marker_app'), '--video', str(video), '--config',
        str(EVIDENCE / 'environment/debug_config.yaml'), '--mode', 'debug', '--run-dir', str(run)]
result = subprocess.run(argv, capture_output=True, text=True)
print(result.stderr)
assert result.returncode != 0 and 'zero-frame run' in result.stderr
assert (run / 'FAILED.json').exists() and not (run / 'summary.yaml').exists()
with (EVIDENCE / 'comparison/zero-frame-check.json').open('x') as stream:
    json.dump({'result': 'PASS', 'argv': argv, 'exit_code': result.returncode,
               'stderr': result.stderr, 'failed_marker': json.loads((run / 'FAILED.json').read_text())}, stream, indent=2)
    stream.write('\n')
print('PASS zero-frame EOF leaves FAILED marker')
