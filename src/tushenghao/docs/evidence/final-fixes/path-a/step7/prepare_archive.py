# 只读保护／源码溯源／归档前置核查，只有本轮证据目录写新报告。
from pathlib import Path
import hashlib,json,datetime,re
root=Path.cwd();e=root/'src/tushenghao/docs/evidence/final-fixes/path-a';pre=json.loads((e/'step0/prechange.json').read_text());before={v['path']:v for v in pre['files']}
allowed={'src/tushenghao/'+p for p in ['CMakeLists.txt','README.md','docs/INDEX.md','docs/final-fixes_acceptance.md','lib/geometry/geometry_l_topology.hpp','lib/geometry/geometry_l_topology.cpp','lib/geometry/geometry_matcher.cpp','tests/geometry_l_topology_test.cpp','tests/manual_validation_check.cpp']}
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
changed=[]
for v in pre['files']:
 p=root/v['path'];assert p.is_file(),v['path']
 if sha(p)!=v['sha256']:changed.append(v['path'])
assert set(changed)<=allowed,sorted(set(changed)-allowed)
for file,marker in [('README.md','\n## Path A 锚点竞争修复'),('docs/INDEX.md','\nPath A 推荐顺序：'),('docs/final-fixes_acceptance.md','\n## Gate1／Path A 定点修复')]:
 path='src/tushenghao/'+file;data=(root/path).read_bytes();prefix=data[:data.index(marker.encode())];assert hashlib.sha256(prefix).hexdigest()==before[path]['sha256'],'original history changed '+file
for item in pre['baseline_copies']:
 for folder in [root/'new-runs/verification',e/'baseline']:
  assert sha(folder/item['file'])==item['sha256'],'old run changed '+str(folder/item['file'])
for item in json.loads((e/'step5/user-original-hashes.json').read_text())['files']:
 assert sha(root/item['path'])==item['sha256'],'user original changed '+item['path']
for path,key in [('data/raw/marker_video.avi','input_sha256'),('src/tushenghao/config/marker_geometry.yaml','model_sha256'),('src/tushenghao/config/detector_verification.yaml','config_sha256')]:assert sha(root/path)==pre[key]
# 按CMake原算法复算生产/审计源摘要，包含新增verification工具，不包含测试／文档。
base=root/'src/tushenghao';sources=set()
for folder,patterns in [('lib',['*.cpp','*.hpp']),('include',['*.hpp']),('app',['*.cpp','*.hpp']),('tools/validation',['*.cpp']),('tools/audit',['*.cpp']),('tools/common',['*.cpp','*.hpp'])]:
 for pattern in patterns:sources.update((base/folder).rglob(pattern))
sources.add(base/'CMakeLists.txt');code=hashlib.sha256(''.join(sha(p) for p in sorted(sources)).encode()).hexdigest()
for name in ['verification-release','verification-debug']:
 manifest=(e/name/'manifest.yaml').read_text();expected=re.search(r'^code_sha256: "([^"]+)"',manifest,re.M).group(1);assert expected==code,(name,expected,code)
 assert (e/name/'summary.yaml').is_file()
for path in ['check-release.json','check-debug.json','release-debug-compare.json','report-release/path_a_report.json','report-debug/path_a_report.json','frames/render_report.json','remaining-frames/render_report.json']:
 assert json.loads((e/path).read_text())['result']=='PASS',path
for build in pre['existing_build_directories']:assert (root/build).is_dir(),'previous build missing '+build
# 每一个本轮目录内文件都完整读过，PNG已由独立readback归档证明；固定build无唯一证据副本。
files=[p for p in e.rglob('*') if p.is_file()];total=0
for p in files:total+=len(p.read_bytes())
result={'result':'PASS','checked_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'protected_preexisting_files':len(pre['files'])-len(changed),'modified_authorized':changed,'original_readme_index_13_item_report_prefix':'SHA256_EXACT','old_run_original_and_copy':'SHA256_EXACT','input_config_model':'SHA256_EXACT','current_code_sha256':code,'code_source_file_count':len(sources),'archive_files_read':len(files),'archive_bytes_read':total,'previous_build_directories_preserved':pre['existing_build_directories'],'final_guard':'no forbidden file modification','user_review':json.loads((e/'step6/user_review_approval.json').read_text())['result']}
output=e/'step7/prepare_archive.json';assert not output.exists();output.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');print(json.dumps(result,ensure_ascii=False))
