# 归档 CLI 负例：受控 fixture 明确验证目标拒绝原因，不能以任意非零冒充覆盖。
import json,subprocess,shutil,csv,re
from pathlib import Path
root=Path.cwd();e=root/'src/tushenghao/docs/evidence/final-fixes/path-a';base=e/'baseline';rep=e/'representatives';out=e/'step4/negative-cases-attempt-03';out.mkdir(exist_ok=False)
binary=root/'build/final-fixes-path-a-release/path_a_verify';config=root/'src/tushenghao/config/detector_verification.yaml'
r=json.loads(rep.joinpath('frames.jsonl').read_text().splitlines()[0]);old=json.loads(base.joinpath('frames.jsonl').read_text().splitlines()[2])
# 单帧只为拒绝路径fixture；不冒充正式1676运行，正式报告会因118/864门槛拒绝。
for record in [r,old]:
 record['frame_id']='0'
 for m in record['details']['decode']['measurements']:
  for corner in m:corner['frame_id']='0'
rows=[]
def run(name,oldrecords,newrecords,expected,match,edit_config=False,extra=None,existing=False):
 folder=out/name;folder.mkdir();oldrun=folder/'old';newrun=folder/'new'
 for target,source,records in [(oldrun,base,oldrecords),(newrun,rep,newrecords)]:
  target.mkdir()
  for filename in ['manifest.yaml','effective_config.yaml']:shutil.copyfile(source/filename,target/filename)
  m=target/'manifest.yaml';s=m.read_text().replace('"decode"','"full"').replace('"EXPLICIT_SUBSET"','"COMPLETE"').replace('"explicit_subset"','"full_input"');m.write_text(s)
  if edit_config and target==newrun:
   p=target/'effective_config.yaml';p.write_text(p.read_text().replace('max_hypothesis_count: 1000','max_hypothesis_count: 999'))
  (target/'frames.jsonl').write_text(''.join(json.dumps(item,ensure_ascii=False,separators=(',',':'))+'\n' for item in records))
 report=folder/'report'
 if existing:report.mkdir()
 args=[str(binary),'--baseline-run',str(oldrun),'--candidate-run',str(newrun),'--config',str(config),'--expected-frames',str(expected),'--report-dir',str(report)]+(extra or [])
 p=subprocess.run(args,capture_output=True,text=True);(folder/'stdout').write_text(p.stdout);(folder/'stderr').write_text(p.stderr)
 result=p.returncode!=0 and match in p.stderr
 rows.append({'case':name,'argv':args,'exit_code':p.returncode,'expected_error':match,'matched':result,'stderr':p.stderr})
 print(name,'PASS' if result else 'FAIL',p.returncode,p.stderr.strip(),flush=True)
 return folder
clone=lambda item:json.loads(json.dumps(item))
run('both-empty',[],[],1,'empty run')
run('missing-frame',[old],[r],2,'record frame count mismatch')
run('duplicate-frame',[old,old],[r,r],2,'duplicate frame_id')
x=clone(r);x['details']=None;run('missing-details',[old],[x],1,'EVIDENCE_DETAILS_MISSING')
x=clone(r);h=x['details']['generated']['hypotheses'][0];idx=next(i for i,s in enumerate(h['evidence']) if s.startswith('anchor_topology/'));h['evidence'][idx]=re.sub(r'supports=[0-9]+:[0-9]+','supports=99999:0',h['evidence'][idx],count=1);run('forged-support',[old],[x],1,'forged or reordered topology support')
run('changed-budget',[old],[r],1,'algorithm config mismatch',edit_config=True)
x=clone(r);x['result']['status']=2;x['result']['detections']=[];x['result']['tracks']=[];x['result_status']=2;x['details']['decode']['status']=2;x['details']['decode']['detections']=[];x['details']['decode']['measurements']=[];x['counts']['measurements']='0';x['counts']['detections']='0';x['counts']['tracks']='0';o=clone(r)
f=run('old-success-to-empty',[o],[x],1,'hard gate failed');j=json.loads((f/'report/path_a_report.json').read_text());assert j['old_success_losses']==1
f=run('outside-a-status-change',[o],[x],1,'hard gate failed');j=json.loads((f/'report/path_a_report.json').read_text());assert j['outside_a_status_changes']==1
run('existing-report',[old],[r],1,'refuse existing output directory',existing=True)
run('unknown-option',[old],[r],1,'unknown/missing option',extra=['--unknown','x'])
run('duplicate-option',[old],[r],1,'duplicate option',extra=['--expected-frames','1'])
run('missing-option-value',[old],[r],1,'unknown/missing option',extra=['--candidate-run'])
args=[str(binary),'render','--candidate-run',str(rep),'--video',str(root/'data/raw/marker_video.avi'),'--review-list',str(e/'step4/representative_review.csv'),'--output-dir',str(e/'representative-frames-attempt-02')]
p=subprocess.run(args,capture_output=True,text=True);rows.append({'case':'existing-render-output','argv':args,'exit_code':p.returncode,'matched':p.returncode!=0 and 'refuse existing output directory' in p.stderr,'stderr':p.stderr});print('existing-render-output',rows[-1]['matched'],flush=True)
(out/'results.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n');assert all(x['matched'] for x in rows),'CLI negative mismatch'
