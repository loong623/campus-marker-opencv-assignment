import json,hashlib,datetime
from pathlib import Path
root=Path.cwd();e=root/'src/tushenghao/docs/evidence/final-fixes/path-a';pre=json.loads((e/'step0/prechange.json').read_text())
allowed={'src/tushenghao/'+p for p in ['CMakeLists.txt','README.md','docs/INDEX.md','docs/final-fixes_acceptance.md','lib/geometry/geometry_l_topology.hpp','lib/geometry/geometry_l_topology.cpp','lib/geometry/geometry_matcher.cpp','tests/geometry_l_topology_test.cpp','tests/manual_validation_check.cpp']}
changed=[];missing=[]
for entry in pre['files']:
 p=root/entry['path']
 if not p.is_file():missing.append(entry['path'])
 elif hashlib.sha256(p.read_bytes()).hexdigest()!=entry['sha256']:changed.append(entry['path'])
unexpected=sorted(set(changed)-allowed);assert not missing and not unexpected,(missing,unexpected)
print('PASS protected files:',len(pre['files'])-len(changed),'authorized changed:',len(changed));print('\n'.join(changed))
(e/'step4/protected.json').write_text(json.dumps({'result':'PASS','checked':len(pre['files']),'changed_authorized':changed,'missing':missing,'unexpected':unexpected,'checked_utc':datetime.datetime.now(datetime.timezone.utc).isoformat()},ensure_ascii=False,indent=2)+'\n')
