"""实际CLI错误回归，记录预期失败的真实退出码；不删除失败运行现场。"""
import json
import os
import subprocess
from pathlib import Path
from archive_checks import ROOT, EVIDENCE, digest

app = str(ROOT / 'build/block5/marker_app')
verify = str(ROOT / 'build/block5/observability_verify')
config = EVIDENCE / 'environment/debug_config.yaml'
video = ROOT / 'data/raw/marker_video.avi'
report = EVIDENCE / 'comparison/negative-cases.json'
cases = []


def reject(name, args, reason=None, environment=None):
    """断言明确非零，而非把失败命令省掉或当作完整视频成功。"""
    result = subprocess.run(args, cwd=ROOT, env=environment,
                            capture_output=True, text=True)
    cases.append({'case': name, 'argv': args, 'exit_code': result.returncode,
                  'stdout': result.stdout, 'stderr': result.stderr,
                  'expected': 'nonzero' + (': ' + reason if reason else '')})
    assert result.returncode != 0, name
    if reason:
        assert reason in result.stderr, (name, result.stderr)


reject('no_arguments', [app])
reject('unknown_option', [app, '--unknown'])
reject('duplicate_option', [app, '--config', str(config), '--config', str(config)])
reject('missing_value', [app, '--video'])
reject('unsupported_purpose', [app, '--video', str(video), '--config', str(config),
                              '--run-purpose', 'fake'])
reject('missing_input', [app, '--video', str(EVIDENCE / 'does-not-exist.avi'),
                         '--config', str(config), '--mode', 'debug',
                         '--run-dir', str(EVIDENCE / 'runs/negative-missing')])
assert not (EVIDENCE / 'runs/negative-missing').exists()
existing = EVIDENCE / 'runs/app-baseline-02/summary.yaml'
old = digest(existing)
reject('existing_run', [app, '--video', str(video), '--config', str(config),
                        '--mode', 'debug', '--run-dir', str(existing.parent)],
       'RUN_DIRECTORY_EXISTS')
assert digest(existing) == old
env = os.environ.copy()
env.pop('DISPLAY', None)
env.pop('WAYLAND_DISPLAY', None)
gui = EVIDENCE / 'environment/negative_gui.yaml'
gui.write_text(config.read_text().replace('show_window: 0', 'show_window: 1'))
reject('explicit_gui_without_display', [app, '--video', str(video), '--config', str(gui),
                                        '--run-dir', str(EVIDENCE / 'runs/negative-gui')],
       'GUI requested but display unavailable', env)
for name, source, target in [
        ('invalid_boolean', 'timing_enabled: 1', 'timing_enabled: 2'),
        ('negative_first', 'detail_first: 0', 'detail_first: -1'),
        ('zero_interval', 'detail_interval: 1', 'detail_interval: 0'),
        ('overflow_first', 'detail_first: 0', 'detail_first: 4294967296'),
        ('float_first', 'detail_first: 0', 'detail_first: 1.5'),
        ('video_not_implemented', 'export_video: 0', 'export_video: 1'),
        ('negative_fps', 'playback_fps: 0.0', 'playback_fps: -1.0')]:
    path = EVIDENCE / 'environment' / ('negative_' + name + '.yaml')
    assert source in config.read_text(), name
    path.write_text(config.read_text().replace(source, target))
    reject(name, [app, '--check-config', '--config', str(path)])
reject('verify_unknown', [verify, '--unknown'])
reject('verify_duplicate', [verify, '--check-run', str(existing.parent),
                             '--check-run', str(existing.parent)])
reject('verify_existing_report', [verify, '--check-run', str(existing.parent),
                                   '--report', str(existing)])
assert digest(existing) == old
with report.open('x') as stream:
    json.dump({'result': 'PASS', 'cases': cases}, stream, ensure_ascii=False, indent=2)
    stream.write('\n')
print('PASS', len(cases), 'expected nonzero CLI cases')
