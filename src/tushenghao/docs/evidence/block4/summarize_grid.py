import json,csv,math,statistics
from pathlib import Path
from collections import defaultdict,Counter
root=Path(__file__).resolve().parent
segments=defaultdict(list)
for r in map(json.loads,(root/'grid-final.jsonl').open()):segments[r['segment']].append(r)
def quantile(v,p):return sorted(v)[math.ceil(len(v)*p)-1] if v else None
def rms(v):return math.sqrt(statistics.mean(x*x for x in v))
summary=[]
for id,rows in sorted(segments.items()):
 assert len(rows)==32
 raw_errors=[];stable_errors=[];dev=[];lag=[];raw_positions=[];stable_positions=[];fallback=Counter();wrong=deleted=leaks=raw_changed=0;previous=None
 for r in rows:
  raw=r['raw'];diag=r['temporal'];truth=r['truth_physical'];physical=r['raw_physical'];out=r['result'];tracks=out['tracks'];
  current_map=[raw['corners'].index(p) for p in physical]
  raw_changed+=int(raw!=out['detections'][0]);deleted+=int(not tracks)
  if diag['correspondence']['valid'] and previous is not None:
   mapping=diag['correspondence']['mapping'];wrong+=int(any(mapping[current_map[p]]!=previous[p] for p in range(4)))
  previous=current_map
  if not diag['used_smoothing']:fallback[diag['reset_or_fallback_reason']]+=1
  if not tracks:continue
  stable=tracks[0]['result'];leaks+=int(not r['known'] and stable['orientation'] is not None)
  output_map=diag['output_slot_mapping'] if diag['used_smoothing'] else list(range(4))
  published=[stable['corners'][output_map[current_map[p]]] for p in range(4)]
  raw_positions.append(physical);stable_positions.append(published)
  for p in range(4):
   raw_errors.append(math.dist(physical[p],truth[p]));stable_errors.append(math.dist(published[p],truth[p]));dev.append(math.dist(published[p],physical[p]))
   if r['mode']=='translation':lag.append(truth[p][0]-published[p][0])
   elif r['rate']:
    dx=truth[p][0]-720;dy=truth[p][1]-540;norm=math.hypot(dx,dy)
    lag.append(((truth[p][0]-published[p][0])*(-dy)+(truth[p][1]-published[p][1])*dx)/norm)
 raw_jitter=stable_jitter=None;ratio=None
 if rows[0]['rate']==0:
  def jitter(positions):
   means=[[statistics.mean(row[p][k] for row in positions) for k in range(2)] for p in range(4)]
   centered=rms([math.dist(row[p],means[p]) for row in positions for p in range(4)])
   diff=rms([math.dist(positions[n][p],positions[n-1][p]) for n in range(1,len(positions)) for p in range(4)])
   return centered,diff
  raw_jitter,raw_diff=jitter(raw_positions);stable_jitter,stable_diff=jitter(stable_positions);ratio=stable_jitter/raw_jitter if raw_jitter else None
 first=rows[0]
 summary.append({'segment':id,'dt_ms':first['dt_ms'],'mode':first['mode'],'rate':first['rate'],'known':first['known'],'noisy':first['noisy'],'r_experimental_px':first['r_experimental_px'],'deviation_experimental_px':first['deviation_experimental_px'],
 'raw_truth_rmse':rms(raw_errors),'stable_truth_rmse':rms(stable_errors),'static_raw_jitter':raw_jitter,'static_stable_jitter':stable_jitter,'jitter_ratio':ratio,'static_raw_diff_rms':raw_diff if raw_jitter is not None else None,'static_stable_diff_rms':stable_diff if raw_jitter is not None else None,
 'deviation_mean':statistics.mean(dev),'deviation_p95':quantile(dev,.95),'deviation_p99':quantile(dev,.99),'deviation_max':max(dev),'lag_mean_px':statistics.mean(lag) if lag else None,'lag_p95_px':quantile(lag,.95),'lag_max_px':max(lag) if lag else None,
 'smoothing_calls':sum(r['temporal']['used_smoothing'] for r in rows),'fallback_calls':sum(fallback.values()),'fallback_reasons':dict(fallback),'wrong_correspondence':wrong,'deleted_legal_measurements':deleted,'unknown_leaks':leaks,'raw_changed':raw_changed})
(root/'grid-summary.json').write_text(json.dumps(summary,indent=2))
with (root/'grid-summary.csv').open('w') as out:
 w=csv.DictWriter(out,fieldnames=list(summary[0]));w.writeheader();w.writerows(summary)
print('segments',len(summary),'calls',sum(len(v) for v in segments.values()),'wrong',sum(s['wrong_correspondence'] for s in summary),'deleted',sum(s['deleted_legal_measurements'] for s in summary),'leaks',sum(s['unknown_leaks'] for s in summary),'raw_changed',sum(s['raw_changed'] for s in summary))
print('14ms known noisy translation')
for s in summary:
 if s['dt_ms']==14 and s['known'] and s['noisy'] and s['mode']=='translation' and s['rate'] in [0,1,4,8]:print(json.dumps(s))
