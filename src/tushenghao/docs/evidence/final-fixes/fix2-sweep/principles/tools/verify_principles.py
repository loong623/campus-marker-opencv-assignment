"""复算历史预算及本轮原理证据；只读生产输入，输出留在 principles。"""
import collections,csv,hashlib,json,math,statistics
from pathlib import Path
P=Path(__file__).resolve().parents[1]
def rows(f):return [json.loads(x) for x in (P/f).read_text().splitlines()]
def digest(path):
 h=hashlib.sha256()
 with Path(path).open('rb') as stream:
  for block in iter(lambda:stream.read(1024*1024),b''):h.update(block)
 return h.hexdigest()
def verify():
 checks=[]
 for version in ['v1','v5']:
  groups=collections.defaultdict(list);source=rows(f'synthetic_C_{version}_measurements.jsonl')
  assert len(source)==720 and all(x['success'] for x in source)
  for x in source:
   for kind in ['L','M','all']:
    values=[v for corner in x['corners'] if kind=='all' or corner['kind']==kind for v in corner['line_mean_residual_px']]
    groups[(x['work_size'][0],x['radius_original_px'],int(x['seed']!=0),kind)].append(max(values))
  original={}
  for x in csv.DictReader((P/f'historical_C-{version}-summary.csv').open()):
   if x['metric']=='max_line_fit_residual':original[(int(x['work_width']),int(x['radius']),int(x['noisy']),x['kind'])]=x
  for key,values in groups.items():
   ordered=sorted(values);mu=statistics.mean(values);sd=statistics.stdev(values);p99=ordered[math.ceil(.99*len(values))-1];u=max(mu+3*sd,p99)
   actual={'n':len(values),'mean':mu,'sample_std':sd,'percentile':p99,'maximum':max(values),'raw':u,'rounded':.5*math.ceil(u/.5)}
   for field,value in actual.items():assert math.isclose(value,float(original[key][field]),rel_tol=0,abs_tol=1e-12),(version,key,field)
  assert len(groups)==len(original)==36
  checks.append({'version':version,'records':720,'strata_match':36})
 real=rows('real_raw_residuals.jsonl');valid=[x for x in real if x['raw_measurement_available']]
 assert len(real)==112 and len(valid)==111 and [x['frame_id'] for x in real if not x['raw_measurement_available']]==[784]
 assert all(max(a['mean_per_visit'] for a in x['arcs'])>.5 for x in valid)
 assert sum(a['occurrences']>a['unique_pixels'] for x in valid for a in x['arcs'])==110
 assert [x['frame_id'] for x in real if x['unit_weight_frozen_gate_pass']]==[280,408,480,1108]
 assert all(max(a['mean_unique_refit'] for a in x['arcs'])>.5 for x in valid)
 for filename in ['controls-original.jsonl','controls-h264-qp0.jsonl','controls-h264-qp28.jsonl']:
  controls=rows(filename);assert len(controls)==64 and all(x['frozen_gate_pass'] and x['raw_available'] for x in controls)
  assert all(x['occurrences']==x['unique_pixels'] for x in controls)
 guard=json.loads((P.parent/'protected_inputs_before.json').read_text())
 changed=[path for path,sha in guard.items() if not Path(path).is_file() or digest(path)!=sha]
 assert not changed,changed
 previous=json.loads((P.parent/'analysis_hashes.json').read_text())
 altered=[path for path,sha in previous['files'].items() if not (P.parent/path).is_file() or digest(P.parent/path)!=sha]
 assert not altered,altered
 report=Path('src/tushenghao/docs/fix2_sweep_report.md');prefix=(P/'previous_report.md').read_bytes()
 assert report.read_bytes().startswith(prefix)
 assert hashlib.sha256(prefix).hexdigest()==previous['report_sha256']
 assert report.read_bytes()[len(prefix):]==(P/'research_append.md').read_bytes()
 out={'historical_budget_checks':checks,'real_failure_frames':112,'real_raw_available':111,'position_failure_preserved':[784],'unit_pixel_weight_corner_passes':4,'imaging_controls_per_mode':64,'protected_files_checked':len(guard),'protected_files_unchanged':not changed,'previous_sweep_files_checked':len(previous['files']),'previous_sweep_files_unchanged':not altered,'previous_report_prefix_preserved':True,'production_changes':False,'threshold_changes':False,'formal_acceptance_run':False,'report_sha256':digest(report)}
 (P/'final_check.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n')
 print(json.dumps(out,ensure_ascii=False,indent=2))
if __name__=='__main__':verify()
