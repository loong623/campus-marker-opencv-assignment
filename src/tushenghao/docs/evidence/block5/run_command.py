"""保存每次命令和退出码，失败日志也保留；不用shell，避免失败后继续依赖步骤。"""
import fcntl
import datetime
import json
import subprocess
import sys
import time
from pathlib import Path

root = Path(__file__).resolve().parents[5]
evidence = Path(__file__).resolve().parent
log = evidence / sys.argv[1]
args = sys.argv[2:]
started = datetime.datetime.now().astimezone().isoformat()
before = time.monotonic()
with log.open('x') as stream:
    process = subprocess.run(args, cwd=root, stdout=stream, stderr=subprocess.STDOUT)
record = {'argv': args, 'cwd': str(root), 'started_at': started,
          'ended_at': datetime.datetime.now().astimezone().isoformat(),
          'wall_seconds': time.monotonic() - before, 'exit_code': process.returncode,
          'log': str(log.relative_to(evidence))}
commands = evidence / 'archive/commands.json'
with (evidence / 'archive/.commands.lock').open('a') as lock:
    fcntl.flock(lock, fcntl.LOCK_EX)
    previous = json.loads(commands.read_text()) if commands.exists() else []
    previous.append(record)
    commands.write_text(json.dumps(previous, ensure_ascii=False, indent=2) + '\n')
print(json.dumps(record, ensure_ascii=False))
raise SystemExit(process.returncode)
