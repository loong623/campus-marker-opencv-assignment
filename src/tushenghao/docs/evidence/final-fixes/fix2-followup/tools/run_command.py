"""记录本轮实际命令、日志和耗时，不调用 shell/Git。"""
import datetime,json,os,shlex,subprocess,sys,time
from pathlib import Path
OUT=Path(__file__).resolve().parents[1]
label=sys.argv[1];command=sys.argv[2:];log=OUT/'build-logs'/f'{label}.log'
if log.exists():raise SystemExit('refusing overwrite log: '+str(log))
start=datetime.datetime.now(datetime.timezone.utc).isoformat();tick=time.monotonic()
with log.open('w') as f:code=subprocess.run(command,stdout=f,stderr=subprocess.STDOUT).returncode
record={'label':label,'command':shlex.join(command),'argv':command,'cwd':os.getcwd(),'start_utc':start,'end_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'elapsed_seconds':time.monotonic()-tick,'exit_code':code,'log':str(log.relative_to(OUT))}
with (OUT/'commands.jsonl').open('a') as f:f.write(json.dumps(record,ensure_ascii=False)+'\n')
print(json.dumps(record,ensure_ascii=False),flush=True)
if code:print(log.read_text()[-18000:])
raise SystemExit(code)
