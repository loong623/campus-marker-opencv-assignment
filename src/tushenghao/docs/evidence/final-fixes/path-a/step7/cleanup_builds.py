# 用户已明确要求清理；只删除本轮两个精确路径并验证任务标记，保留所有原build。
from pathlib import Path
import json,shutil,datetime
root=Path.cwd().resolve();e=root/'src/tushenghao/docs/evidence/final-fixes/path-a';pre=json.loads((e/'step0/prechange.json').read_text());started=datetime.datetime.now(datetime.timezone.utc).isoformat();result=[]
for name in ['final-fixes-path-a-release','final-fixes-path-a-debug']:
 path=root/'build'/name;resolved=path.resolve();assert resolved==root/'build'/name and path.parent.resolve()==root/'build' and not path.is_symlink(),str(path)
 assert (path/'.path-a-owned').read_text().strip()==str(resolved),'ownership marker mismatch'
 kind=name.rsplit('-',1)[1]
 for log in ['CMakeCache.txt','CTestTestfile.cmake','LastTest.log']:assert (e/'tests'/kind/log).is_file()
 for log in ['ctest-formal.stdout','ctest-formal.stderr']:assert (e/'tests'/kind/log).is_file()
 shutil.rmtree(path);assert not path.exists();result.append({'absolute_path':str(resolved),'ownership_marker':'EXACT_MATCH','removed':True})
for original in pre['existing_build_directories']:assert (root/original).is_dir(),original
# 只清本轮已归档的临时cpp/可执行程序；不碰旧任务或用户临时路径。
removed=[]
for name in ['path-a-manual-proposed.cpp','path-a-manual-proposed','path-a-check-png']:
 path=Path('/tmp')/name
 if path.exists():assert path.is_file() and not path.is_symlink();path.unlink();removed.append(str(path))
empty=e/'frames-pending'
if empty.is_dir() and not any(empty.iterdir()):empty.rmdir()
p=e/'step7/cleanup.json';assert not p.exists();p.write_text(json.dumps({'result':'PASS','started_utc':started,'finished_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'removed_builds':result,'previous_build_directories_preserved':pre['existing_build_directories'],'removed_task_temp_files':removed,'user_new_runs_untouched':True,'old_evidence_untouched':True},ensure_ascii=False,indent=2)+'\n');print('PASS removed only the 2 owned fixed builds; preserved',len(pre['existing_build_directories']),'original build directories; temp files:',removed)
