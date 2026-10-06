"""仅清理本方案允许且开工时不存在的三个 build；先核对已归档缓存。"""
from pathlib import Path
import datetime,hashlib,json,shutil
ROOT=Path.cwd();OUT=ROOT/'src/tushenghao/docs/evidence/final-fixes/fix2-followup'
start=json.loads((OUT/'start.json').read_text());expected={f'build/fix2-followup-{x}' for x in ['release','debug','fixture']}
assert set(start['build_dirs_absent_at_start'])==expected
prereq=json.loads((OUT/'build-logs/cleanup-prerequisites.json').read_text());assert {x['build'] for x in prereq['entries']}==expected
results=[]
for entry in prereq['entries']:
 path=ROOT/entry['build'];kind=path.name.removeprefix('fix2-followup-')
 assert path.parent==ROOT/'build' and path.exists() and not path.is_symlink()
 cache=path/'CMakeCache.txt';archived=OUT/'build-logs'/f'archive-{kind}'/'CMakeCache.txt'
 assert cache.read_bytes()==archived.read_bytes()
 assert sum(f.is_file() for f in path.rglob('*'))==entry['file_count'],'unexpected build content change'
 # 从零创建的本轮构建仅含 CMake/编译/测试产物及本轮独立核查可执行文件。
 results.append({'path':entry['build'],'owned_this_task':True,'absent_at_start':True,'cache_archived_sha256':hashlib.sha256(archived.read_bytes()).hexdigest(),'files_removed':entry['file_count']})
 shutil.rmtree(path)
 assert not path.exists()
result={'completed_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'deleted':results,'other_build_dirs_untouched':True,'only_allowed_three_builds_deleted':True,'pass':True}
(OUT/'cleanup.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');print(json.dumps(result,ensure_ascii=False))
