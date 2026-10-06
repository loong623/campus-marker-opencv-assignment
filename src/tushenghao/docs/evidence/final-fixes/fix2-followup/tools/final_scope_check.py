"""只读核对保护文件、严格诊断差异、源码范围、测试及历史；不依赖已清理 build。"""
import collections,difflib,hashlib,importlib.util,json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
OUT=Path(__file__).resolve().parents[1];BASE=Path('src/tushenghao')
def sha(path):
 h=hashlib.sha256()
 with Path(path).open('rb') as f:
  for block in iter(lambda:f.read(1024*1024),b''):h.update(block)
 return h.hexdigest()
def load(path):return [json.loads(s) for s in path.read_text().splitlines()]
def normalize(value):
 if isinstance(value,dict):return {k:normalize(v) for k,v in value.items()}
 if isinstance(value,list):return [normalize(x) for x in value]
 if isinstance(value,str) and value.startswith('geometry/parent='):return re.sub(r'(/residual=)[^/]+',r'\1<measured>',value)
 return value
before=json.loads((OUT/'hashes-before.json').read_text())
assert all(Path(f).is_file() and sha(f)==h for f,h in before['protected'].items())
style=json.loads((OUT/'style/token-check.json').read_text());assert style['pass'] and style['checked']==83
assert all(sha(row['file'])==row['after_file_sha256'] for row in style['files'])
functional=['src/tushenghao/'+x for x in ['tools/block3_fixture/calibration/measure.cpp','tools/block3_fixture/reporting/report_verification.py','tools/block3_fixture/CMakeLists.txt','lib/geometry/geometry_validation.cpp','lib/geometry/geometry_matcher.cpp','lib/core/geometry_types.hpp','tests/corner_edge_fit_test.cpp','tests/geometry_matcher_test.cpp','CMakeLists.txt']]
docs=['src/tushenghao/'+x for x in ['README.md','docs/INDEX.md','docs/fix2_sweep_report.md','docs/final-fixes_acceptance.md']]
changed=[f for f,h in before['source'].items() if sha(f)!=h]
assert set(changed)<=set(functional+docs+[row['file'] for row in style['files']])
for phase in ['baseline','functional','style','final']:
 for kind in ['release','debug']:
  text=(OUT/'tests'/f'{phase}-{kind}.log').read_text();assert '100% tests passed, 0 tests failed out of 25' in text
  if phase=='final':
   assert 'PASS UniqueSupportPixelCount' in text
   for case in ['G01_actual_zero','G02_one_work_pixel','G03_maximum_not_global_mean','G04_comparison_boundary','G05_input_unchanged','G06_completeness_branches','G07_unmeasurable_inputs','G08_noncontiguous_ids']:assert 'PASS '+case in text
spec=importlib.util.spec_from_file_location('token_audit',OUT/'style/check_tokens.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
p=BASE/'lib/core/geometry_types.hpp';assert m.signature(p,(OUT/'source-before/lib/core/geometry_types.hpp').read_bytes())[1:]==m.signature(p,p.read_bytes())[1:]
p=BASE/'lib/geometry/geometry_matcher.cpp';a=m.signature(p,(OUT/'source-before/lib/geometry/geometry_matcher.cpp').read_bytes())[1];b=m.signature(p,p.read_bytes())[1];d=[]
for tag,i,j,k,l in difflib.SequenceMatcher(a=a,b=b,autojunk=False).get_opcodes():
 if tag!='equal':d.append({'old':a[i:j],'new':b[k:l]})
assert d==[{'old':[('LITERAL','0.0')],'new':[('PUNCTUATION','-'),('LITERAL','1.0')]}]
old=load(OUT/'baseline/frames.jsonl');new=load(OUT/'verification-final-review/frames.jsonl');assert len(old)==len(new)==1676;changes=collections.Counter()
for a,b in zip(old,new):
 assert a['frame_id']==b['frame_id'] and a['source_timestamp_us']==b['source_timestamp_us'] and a['counts']==b['counts']
 assert normalize(a['result'])==normalize(b['result']) and normalize(a['events'])==normalize(b['events'])
 ad=json.loads(json.dumps(a['details']));bd=json.loads(json.dumps(b['details']))
 for stage in ['generated','validated','completed']:
  assert len(ad[stage]['hypotheses'])==len(bd[stage]['hypotheses'])
  for h,k in zip(ad[stage]['hypotheses'],bd[stage]['hypotheses']):
   changes[stage]+=h['validation_residual']!=k['validation_residual'];h.pop('validation_residual');k.pop('validation_residual')
 assert normalize(ad)==normalize(bd)
# 历史迁移完全可逆，原代码块/段落字节保留。
original=(OUT/'readme/README-before.md').read_bytes();migration=json.loads((OUT/'readme/history-migration.json').read_text());body=(BASE/'docs/history/README_before_fix2_followup.md').read_text();assert body.startswith(migration['added_notice']);body=body[len(migration['added_notice']):]
for row in migration['relative_links_changed']:body=body.replace(']('+row['after']+')',']('+row['before']+')')
assert body.encode()==original==(OUT/'source-before/README.md').read_bytes()
for name in ['fix2_sweep_report.md','final-fixes_acceptance.md']:
 assert (BASE/'docs'/name).read_bytes().startswith((OUT/'source-before/docs'/name).read_bytes())
links=[]
for path in [BASE/'README.md',BASE/'docs/INDEX.md',BASE/'docs/fix2_followup_log.md',BASE/'docs/fix2_followup_acceptance.md',BASE/'docs/history/README_before_fix2_followup.md',BASE/'docs/fix2_sweep_report.md',BASE/'docs/final-fixes_acceptance.md']:
 for target in re.findall(r'\]\(([^)]+)\)',path.read_text()):
  if '://' in target or target.startswith('#'):continue
  target=target.split('#')[0];assert (path.parent/target).exists(),(str(path),target);links.append((str(path),target))
result={'protected_files_checked':len(before['protected']),'protected_unchanged':True,'changed_original_files':changed,'format_files':83,'cpp_tokens_and_preprocessor_structure_equal':True,'matcher_only_logic_token_change':d,'geometry_types_layout_and_default_unchanged':True,'all_four_phases_release_debug_25_pass':True,'strict_video_frames':1676,'public_algorithm_payloads_counts_stamps_equal':True,'all_details_equal_except_declared_residual_fields_and_parent_residual_number':True,'diagnostic_changes':dict(changes),'history_restored_exactly':True,'previous_reports_preserved_as_prefix':True,'local_links_checked':len(links),'pass':True}
(OUT/'scope-check.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n');(OUT/'changed-files.txt').write_text(''.join(f+'\n' for f in changed));print(json.dumps({k:v for k,v in result.items() if k!='changed_original_files'},ensure_ascii=False))
