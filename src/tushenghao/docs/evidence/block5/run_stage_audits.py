"""三个all审计和原网格依次运行，保留每个独立命令/退出码，不并行争抢算法性能。"""
import subprocess
import sys
from archive_checks import ROOT, EVIDENCE

base = [sys.executable, str(EVIDENCE / 'run_command.py')]
video = 'data/raw/marker_video.avi'
config = 'src/tushenghao/config/detector.yaml'
prefix = 'src/tushenghao/docs/evidence/block5'
commands = [
    ('geometry-all-01', ['build/block5/geometry_audit', '--video', video, '--config', config]),
    ('decode-all-01', ['build/block5/decode_audit', video, 'all', config]),
    ('temporal-all-01', ['build/block5/temporal_audit', '--video', video, '--config', config]),
    ('grid-01', ['build/block5/temporal_audit', '--experiment-grid', '--noise-csv',
                 'src/tushenghao/docs/evidence/block4/C-fixed-noise.csv', '--config', config])]
for name, argv in commands:
    subprocess.run(base + ['tests/step7-' + name + '.log'] + argv +
                   ['--run-dir', prefix + '/runs/' + name], cwd=ROOT, check=True)
    subprocess.run(base + ['tests/step7-' + name + '-check.log', 'build/block5/observability_verify',
                          '--check-run', prefix + '/runs/' + name, '--report',
                          prefix + '/comparison/' + name + '-check.json'], cwd=ROOT, check=True)
