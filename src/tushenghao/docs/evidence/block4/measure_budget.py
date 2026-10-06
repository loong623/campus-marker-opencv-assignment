# 仅分析已存档真值，不运行/调节Detector；保留所有分母，预算候选待用户选择。
import json,math,statistics,csv,hashlib
from pathlib import Path
from collections import defaultdict
root=Path(__file__).resolve().parent
sources={'C':Path('build/block3-C-final-v7.jsonl'),'H':Path('build/final-docs-tools-H.jsonl')}
records={};groups=defaultdict(list); failures=[]; noise=[]; provenance={}
for dataset,path in sources.items():
 rows=[json.loads(s) for s in path.open()];records[dataset]=rows
 manifest=json.loads(Path(f'build/block3-{dataset}-v1/manifest.json').read_text())
 model=Path('src/tushenghao/config/marker_geometry.yaml')
 assert hashlib.sha256(model.read_bytes()).hexdigest()==manifest['model_yaml_sha256']
 truth={r['case_id']:r for r in map(json.loads,Path(f'build/block3-{dataset}-v1/truth.jsonl').open())}
 provenance[dataset]={'record_path':str(path),'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'manifest_sha256':hashlib.sha256(Path(f'build/block3-{dataset}-v1/manifest.json').read_bytes()).hexdigest(),'producer_code_sha256':sorted(set(r['code_sha256'] for r in rows)),'config_sha256':sorted(set(r['config_sha256'] for r in rows))}
 with (root/f'{dataset}-corner-errors.csv').open('w') as out:
  writer=csv.writer(out);writer.writerow(['case_work_id','work_width','radius','noise_sigma','measurable','reason','e0','e1','e2','e3','max_error'])
  for r in rows:
   t=truth[r['case_id']];assert t['image_sha256']==r['image_sha256'] and t['physical_corners_original']==r['physical_corners_original']
   assert hashlib.sha256(Path(f'build/block3-{dataset}-v1/{r["image"]}').read_bytes()).hexdigest()==r['image_sha256']
   key=(dataset,r['work_size'][0],r['radius_original_px'],r['noise_sigma']);ds=r['detections'];errors=None
   if len(ds)==1 and ds[0]['orientation'] is not None:
    d=ds[0];o=d['orientation'];assert sorted(o)==[0,1,2,3]
    signed=[(d['corners'][o[i]][0]-r['physical_corners_original'][i][0],d['corners'][o[i]][1]-r['physical_corners_original'][i][1]) for i in range(4)]
    errors=[math.hypot(*p) for p in signed];assert abs(max(errors)-d['truth_error_px'])<1e-9
    if dataset=='C' and r['work_size'][0]==1440 and r['radius_original_px']==2 and r['noise_sigma']==1:noise.append((r['case_work_id'],signed))
   else:failures.append((r['case_work_id'],r['diagnostics']))
   groups[key].append(None if errors is None else max(errors));writer.writerow([r['case_work_id'],*key[1:],bool(errors),'' if errors else '|'.join(r['diagnostics']),*(errors or ['']*4),max(errors) if errors else ''])
def percentile(v,p):return sorted(v)[math.ceil(len(v)*p)-1]
stats=[]
for key,values in sorted(groups.items()):
 valid=[v for v in values if v is not None]
 row={'dataset':key[0],'work_width':key[1],'radius':key[2],'noise_sigma':key[3],'total':len(values),'measurable':len(valid),'failures':len(values)-len(valid),'mean':statistics.mean(valid),'sd':statistics.stdev(valid),'p95':percentile(valid,.95),'p99':percentile(valid,.99),'max':max(valid)}
 row['formula_raw']=max(row['mean']+3*row['sd'],row['p99']);row['rounded_candidate']=math.ceil(row['formula_raw']*2)/2;stats.append(row)
r=max(row['rounded_candidate'] for row in stats if row['dataset']=='C')
assert not failures
for row in stats:
 if row['dataset']=='H':
  v=groups[('H',row['work_width'],row['radius'],row['noise_sigma'])];row['exceed_r']=sum(x>r for x in v);row['exceed_ratio']=row['exceed_r']/len(v)
(root/'localization-statistics.json').write_text(json.dumps({'status':'EXPERIMENTAL_NOT_APPROVED','r_candidate_px':r,'statistics':stats,'provenance':provenance,'failures':failures},indent=2))
with (root/'localization-statistics.csv').open('w') as out:
 keys=list(stats[-1]);w=csv.DictWriter(out,fieldnames=keys);w.writeheader();w.writerows(stats)
noise=sorted(noise)[:32]
with (root/'C-fixed-noise.csv').open('w') as out:
 w=csv.writer(out);w.writerow(['case_work_id','dx0','dy0','dx1','dy1','dx2','dy2','dx3','dy3']);w.writerows([[id,*[v for p in signed for v in p]] for id,signed in noise])
production=Path('src/tushenghao/config/detector.yaml').read_text();model=str(Path('src/tushenghao/config/marker_geometry.yaml').resolve())
production=production.replace('marker_geometry_path: "marker_geometry.yaml"',f'marker_geometry_path: "{model}"')
# 使实验配置相对于自己的目录也指向同一模型，绝不改生产。
lines=[line for line in production.splitlines() if not line.strip().startswith(('correspondence_uncertainty_px:', 'max_smoothing_deviation_px:'))]
for i,line in enumerate(lines):
 if line.startswith('marker_geometry_path:'):lines[i]=f'marker_geometry_path: "{model}"'
text='\n'.join(lines)+'\n';text=text.replace('temporal:\n',f'temporal:\n  # EXPERIMENTAL 未批准，不替换生产配置\n  correspondence_uncertainty_px: {r}\n  max_smoothing_deviation_px: 2.0\n')
(root/'EXPERIMENTAL-detector.yaml').write_text(text)
print(json.dumps({'C_total':len(records['C']),'H_total':len(records['H']),'r_candidate_px':r,'H_exceed':sum(row.get('exceed_r',0) for row in stats),'failures':failures},indent=2))
