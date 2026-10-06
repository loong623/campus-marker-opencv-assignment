# 独立逐帧检查raw不变、当前引用、double递推、float发布、几何、偏离、恢复及语义隔离。
import json,hashlib,math,struct,statistics
from pathlib import Path
from collections import Counter
root=Path(__file__).resolve().parent
old=[json.loads(s) for s in Path('build/block3-video-final-v7.jsonl').open()]
new=[json.loads(s) for s in (root/'temporal-approved.jsonl').open()]
assert len(old)==len(new)==1676
f32=lambda x:struct.unpack('f',struct.pack('f',x))[0]
def same_payload(a,b):return all(a[k]==b[k] for k in ['corners','bbox','orientation','marker_code','confidence','category','quality_flags'])
changes=[];detected=empty=smooth=recovery=unknown=0;dev=[];reasons=Counter();previous_stable=None;previous_raw=None;previous_stamp=None;last_empty=True
for a,b in zip(old,new):
 assert a['frame_id']==b['frame_id'] and a['input_sha256']==b['input_sha256']
 d=b['decode'];out=b['finalized'];diag=b['temporal'];size=b['original_size']
 for field in ['status','search_truncated','measurements','diagnostics']:
  if a[field]!=d[field]:changes.append({'id':a['frame_id'],'field':field})
 if len(a['detections'])!=len(d['detections']):changes.append({'id':a['frame_id'],'field':'count'})
 for x,y in zip(a['detections'],d['detections']):
  for field in ['corners','bbox','orientation','marker_code']:
   if x[field]!=y[field]:changes.append({'id':a['frame_id'],'field':field})
 assert out['status']==d['status'] and out['detections']==d['detections'] and out['display'] is None
 if not d['detections']:
  empty+=1;assert not out['tracks'] and not diag['used_smoothing'];previous_stable=previous_raw=previous_stamp=None;last_empty=True;continue
 detected+=1;assert len(out['tracks'])==1
 index=out['tracks'][0]['detection_index'];assert 0<=index<len(d['detections'])
 raw=d['detections'][index];stable=out['tracks'][0]['result'];mapping=diag['output_slot_mapping'] if diag['used_smoothing'] else [0,1,2,3]
 assert sorted(mapping)==[0,1,2,3]
 assert stable['confidence'] is None and stable['marker_code'] is None
 assert stable['category']==raw['category'] and stable['quality_flags']==raw['quality_flags']
 if raw['orientation'] is None:unknown+=1;assert stable['orientation'] is None
 else:assert stable['orientation']==[mapping[i] for i in raw['orientation']]
 by_current=[stable['corners'][mapping[i]] for i in range(4)]
 if last_empty:recovery+=1;assert not diag['used_smoothing'] and same_payload(raw,stable)
 expected=raw['corners']
 if diag['used_smoothing']:
  smooth+=1;assert previous_stable is not None and diag['correspondence']['valid'] and diag['association']['matched_history']
  cm=diag['correspondence']['mapping'];assert sorted(cm)==[0,1,2,3]
  if raw['orientation'] is not None and previous_raw['orientation'] is not None:
   assert all(cm[raw['orientation'][p]]==previous_raw['orientation'][p] for p in range(4))
  else:
   assert all(cm[i]==(i+cm[0])%4 for i in range(4));lo=diag['correspondence']['lower'];hi=diag['correspondence']['upper'];k=cm[0]
   assert all(hi[k]<lo[j] for j in range(4) if j!=k)
  dt=(b['timestamp_us']-previous_stamp)/1e6;alpha=-math.expm1(-dt/(-.014/math.log1p(-.7)))
  assert abs(diag['dt_seconds']-dt)<1e-12 and abs(diag['alpha']-alpha)<1e-12
  expected=[[alpha*raw['corners'][i][k]+(1-alpha)*previous_stable[cm[i]][k] for k in range(2)] for i in range(4)]
  assert all(abs(by_current[i][k]-f32(expected[i][k]))<1e-5 for i in range(4) for k in range(2))
 else:
  assert diag['alpha'] is None and same_payload(raw,stable);reasons[diag['reset_or_fallback_reason']]+=1
 for i in range(4):
  p,q=by_current[i],raw['corners'][i];distance=math.dist(p,q);dev.append(distance);assert distance<=2
  assert 0<=p[0]<size[0] and 0<=p[1]<size[1] and all(math.isfinite(x) for x in p)
 pts=stable['corners']
 for i in range(4):
  p,q,r=pts[i],pts[(i+1)%4],pts[(i+2)%4];assert (q[0]-p[0])*(r[1]-q[1])-(q[1]-p[1])*(r[0]-q[0])>0
 xs=[p[0] for p in pts];ys=[p[1] for p in pts];bbox=[min(xs),min(ys),f32(max(xs)-min(xs)),f32(max(ys)-min(ys))]
 assert stable['bbox']==bbox
 previous_stable=expected;previous_raw=raw;previous_stamp=b['timestamp_us'];last_empty=False
report={'total':len(new),'raw_detected':detected,'raw_empty':empty,'raw_differences':changes,'smoothing_frames':smooth,'raw_fallback_frames':detected-smooth,'fallback_reasons':dict(reasons),'recovery_first_frames_raw':recovery,'unknown_outputs_not_filled':unknown,'current_deviation_mean_px':statistics.mean(dev),'current_deviation_p99_px':sorted(dev)[math.ceil(.99*len(dev))-1],'current_deviation_max_px':max(dev),'invalid_geometry':0,'deleted_current_measurements':0,'empty_with_track':0,'production_display_nonnull':0,'confidence_or_code_nonnull':0,'wrong_physical_or_cyclic_mapping':0,'input_sha256':new[0]['input_sha256'],'code_sha256':new[0]['code_sha256'],'config_sha256':new[0]['config_sha256'],'effective_config_sha256':new[0]['effective_config_sha256'],'model_sha256':new[0]['model_sha256'],'record_sha256':hashlib.sha256((root/'temporal-approved.jsonl').read_bytes()).hexdigest(),'G-B-approval_sha256':hashlib.sha256((root/'G-B-approval.json').read_bytes()).hexdigest(),'result':'PASS'}
(root/'approved-comparison.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2));assert not changes
