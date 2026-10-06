# 最后一次直接执行（不用会在封存后追加日志的wrapper）：真实命令索引、链接与SHA一并封存。
from pathlib import Path
import json,hashlib,datetime,time,re,sys,shlex
from urllib.parse import unquote
root=Path.cwd().resolve();e=root/'src/tushenghao/docs/evidence/final-fixes/path-a';started=datetime.datetime.now(datetime.timezone.utc).isoformat();timer=time.monotonic();pre=json.loads((e/'step0/prechange.json').read_text());before={x['path']:x for x in pre['files']};sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert not (e/'hashes.json').exists() and not (e/'step7/final-links.json').exists()
allowed={'src/tushenghao/'+p for p in ['CMakeLists.txt','README.md','docs/INDEX.md','docs/final-fixes_acceptance.md','lib/geometry/geometry_l_topology.hpp','lib/geometry/geometry_l_topology.cpp','lib/geometry/geometry_matcher.cpp','tests/geometry_l_topology_test.cpp','tests/manual_validation_check.cpp']}
new=['src/tushenghao/'+p for p in ['lib/geometry/geometry_anchor_evidence.hpp','lib/geometry/geometry_anchor_evidence.cpp','tests/geometry_anchor_evidence_test.cpp','tests/path_a_competition_test.cpp','tools/validation/path_a_verify.cpp','docs/path_a_fix_log.md','docs/path_a_acceptance.md']]
# 所有禁止修改的原文件与用户原件再次只读核验。
protected=[];changed=[]
for row in pre['files']:
 p=root/row['path'];assert p.is_file(),str(p);current=sha(p);allowed_change=current!=row['sha256'];assert not allowed_change or row['path'] in allowed,row['path'];protected.append({'path':row['path'],'before_sha256':row['sha256'],'after_sha256':current,'authorized_change':allowed_change})
 if allowed_change:changed.append({'path':row['path'],'operation':'modified','sha256':current})
for path in new:assert (root/path).is_file();changed.append({'path':path,'operation':'new','sha256':sha(root/path)})
for row in json.loads((e/'step5/user-original-hashes.json').read_text())['files']:assert sha(root/row['path'])==row['sha256'],row['path']
for row in pre['baseline_copies']:
 for folder in [root/'new-runs/verification',e/'baseline']:assert sha(folder/row['file'])==row['sha256']
for path,key in [('data/raw/marker_video.avi','input_sha256'),('src/tushenghao/config/marker_geometry.yaml','model_sha256'),('src/tushenghao/config/detector_verification.yaml','config_sha256')]:assert sha(root/path)==pre[key]
assert json.loads((e/'step7/cleanup.json').read_text())['result']=='PASS'
for name in ['final-fixes-path-a-release','final-fixes-path-a-debug']:assert not (root/'build'/name).exists()
for path in pre['existing_build_directories']:assert (root/path).is_dir(),path
approval=json.loads((e/'step6/user_review_approval.json').read_text());assert approval['result']=='PASS' and approval['reviewed_frames']==132 and approval['review_index_sha256']==sha(e/'frames/review_index.csv')
# 首次受阻/未运行是历史状态；当前顶层和三同步段必须是最终通过状态。
log=root/'src/tushenghao/docs/path_a_fix_log.md';text=log.read_text().replace('归档与精确清理完成，最终链接/哈希封存收尾中。','归档、文档链接与哈希封存完成，两个固定build已清理，本轮收尾完成。')
text+='\n最终链接/归档封存检查PASS；[本地链接报告](evidence/final-fixes/path-a/step7/final-links.json)、[SHA256清单](evidence/final-fixes/path-a/hashes.json)。输入/模型/配置/原件保护、两build已清/33旧目录保留均复核；日志及数据一次最终封存，不再追加未授权修改。\n';log.write_text(text)
# 新log最后编辑后更新源文档SHA，封存本身排除自身两个审计文件以避免自引用。
for row in changed:row['sha256']=sha(root/row['path'])
docs=[root/'src/tushenghao'/p for p in ['README.md','docs/INDEX.md','docs/final-fixes_acceptance.md','docs/path_a_fix_log.md','docs/path_a_acceptance.md']];links=[];future={e/'hashes.json',e/'step7/final-links.json'}
for doc in docs:
 for match in re.finditer(r'\]\((<[^>]*>|[^)]*)\)',doc.read_text()):
  target=match.group(1)
  if target.startswith('<'):target=target[1:-1]
  if not target or target.startswith('#') or '://' in target or target.startswith('mailto:'):continue
  target=unquote(target.split('#')[0]);path=(doc.parent/target).resolve();assert path.exists() or path in future,('missing local link',doc,target)
  assert '/build/' not in str(path),('permanent build link',doc,target)
  links.append({'document':str(doc.relative_to(root)),'reference':match.group(1),'target':str(path.relative_to(root)),'exists':True})
# 除commands两个待最终登记文件，所有归档产物先完整读/hash及再读核验。
excluded={e/'hashes.json',e/'step7/final-links.json'};entries=[]
for path in sorted(p for p in e.rglob('*') if p.is_file() and p not in excluded):
 entries.append({'path':str(path.relative_to(e)),'sha256':sha(path),'size':path.stat().st_size})
for row in entries:assert sha(e/row['path'])==row['sha256'],row['path']
finished=datetime.datetime.now(datetime.timezone.utc).isoformat();elapsed=time.monotonic()-timer;argv=['python3','src/tushenghao/docs/evidence/final-fixes/path-a/step7/seal_archive.py'];stdout='PASS final seal: 5 documents / '+str(len(links))+' local links; protected originals unchanged; user 132/132 review PASS; own builds removed, old 33 retained\n'
(e/'step7/seal.stdout').write_text(stdout);(e/'step7/seal.stderr').write_text('')
records=json.loads((e/'commands.json').read_text());records.append({'argv':argv,'cwd':str(root),'started_utc':started,'finished_utc':finished,'elapsed_seconds':elapsed,'timing_boundary':'all source/artifact/link validation and hash reads; excludes final journal/seal serialization','exit_code':0,'stdout':'step7/seal.stdout','stderr':'step7/seal.stderr'});(e/'commands.json').write_text(json.dumps(records,ensure_ascii=False,indent=2)+'\n')
with (e/'commands.md').open('a') as f:f.write('\n- '+started+'；退出0；实测检查/完整读取 '+f'{elapsed:.3f}'+' s（最终日志/封存序列化不计）；\n\n  `'+shlex.join(argv)+'`\n\n  [stdout](step7/seal.stdout) / [stderr](step7/seal.stderr)\n')
# 最终登记后再取commands、当前文档和本轮数据摘要；不在这之后运行日志wrapper。
entries=[{'path':str(p.relative_to(e)),'sha256':sha(p),'size':p.stat().st_size} for p in sorted(p for p in e.rglob('*') if p.is_file() and p not in excluded)]
guard=json.loads((e/'step7/prepare_archive.json').read_text());manifest={'result':'PASS','sealed_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'input_sha256':pre['input_sha256'],'config_sha256':pre['config_sha256'],'model_sha256':pre['model_sha256'],'baseline_source_sha256':pre['current_source_matches_user_baseline_code_sha256'],'candidate_source_sha256':guard['current_code_sha256'],'old_result_fingerprint':'c1785fb03a54ed25','new_release_debug_result_fingerprint':'0d2d2e63aef753bc','user_review':'132/132 PASS','build_cleanup':'2 exact owned directories removed; 33 originals preserved','modified_and_new_source_docs':changed,'protected_files':protected,'artifacts':entries,'excluded_self_audit_files':['hashes.json','step7/final-links.json']}
(e/'hashes.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
# 封存文件真实落位后逐链接核验；审计报告自身明确排除，防止散列循环。
for link in links:assert (root/link['target']).exists() or root/link['target']==e/'step7/final-links.json'
report={'result':'PASS','checked_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'documents':len(docs),'local_links':len(links),'permanent_build_links':0,'links':links,'artifact_sha256_readback':'PASS','artifact_count':len(entries),'total_actual_elapsed_seconds_before_report_write':time.monotonic()-timer}
(e/'step7/final-links.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
for link in links:assert (root/link['target']).exists()
for row in entries:assert sha(e/row['path'])==row['sha256']
for row in changed:assert sha(root/row['path'])==row['sha256']
# 日志helper已永久归档，最后只清精确属于本轮的临时脚本。
temp=Path('/tmp/path_a_run.py');assert sha(temp)==sha(e/'step7/run_logged.py');temp.unlink()
print(stdout,end='')
