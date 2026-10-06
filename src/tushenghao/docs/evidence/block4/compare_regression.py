import json,hashlib
from pathlib import Path
root=Path(__file__).resolve().parent
old=[json.loads(s) for s in Path('build/block3-video-final-v7.jsonl').open()]
new=[json.loads(s) for s in (root/'temporal-production-pending.jsonl').open()]
assert len(old)==len(new)==1676
changes=[];empty=detected=0
for a,b in zip(old,new):
 assert a['frame_id']==b['frame_id']
 if a['input_sha256']!=b['input_sha256']:raise RuntimeError('input changed')
 d=b['decode'];
 for field in ['status','search_truncated','measurements','diagnostics']:
  if a[field]!=d[field]:changes.append({'frame_id':a['frame_id'],'field':field})
 # 新记录只增加raw类别/质量/confidence；旧角/框/方向/编码逐项完全对比。
 if len(a['detections'])!=len(d['detections']):changes.append({'frame_id':a['frame_id'],'field':'detection_count'})
 for x,y in zip(a['detections'],d['detections']):
  for field in ['corners','bbox','orientation','marker_code']:
   if x[field]!=y[field]:changes.append({'frame_id':a['frame_id'],'field':field})
  assert y['confidence'] is None and y['marker_code'] is None
 r=b['finalized'];assert r['status']==0 and not r['detections'] and not r['tracks'] and r['display'] is None
 if d['detections']:detected+=1
 else:empty+=1
report={'total':len(new),'raw_detected':detected,'raw_empty':empty,'raw_differences':changes,'pending_gb_notready':len(new),'production_tracks':0,'production_display_nonnull':0,'input_sha256':new[0]['input_sha256'],'baseline_record_sha256':hashlib.sha256(Path('build/block3-video-final-v7.jsonl').read_bytes()).hexdigest(),'current_record_sha256':hashlib.sha256((root/'temporal-production-pending.jsonl').read_bytes()).hexdigest(),'formal_approved_stability_regression':'BLOCKED_G-B_PENDING','video_code_sha256':new[0]['code_sha256']}
(root/'video-comparison.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2));assert not changes
