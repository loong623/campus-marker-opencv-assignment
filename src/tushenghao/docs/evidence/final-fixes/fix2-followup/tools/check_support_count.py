import importlib.util,json,math,sys
from pathlib import Path
sys.dont_write_bytecode=True
OUT=Path(__file__).resolve().parents[1]
reporting=Path('src/tushenghao/tools/block3_fixture/reporting');sys.path.insert(0,str(reporting))
spec=importlib.util.spec_from_file_location('verification',reporting/'report_verification.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
count=module.count_unique_support_pixels
assert count([])==0
assert count([[1,1],[2,1],[1,1],[3,1],[2,1],[3,1]])==3
assert count([[i,0] for i in range(9)]+[[0,0]])==9
assert count([[i,0] for i in range(10)])==10
assert count([[1,2],[math.nextafter(1.,2.),2]])==2
for bad in [float('nan'),float('inf'),-float('inf')]:
 try:count([[0,0],[bad,1]])
 except ValueError:pass
 else:raise AssertionError('nonfinite point accepted')
result={'empty':True,'retraces':True,'ten_visits_nine_pixels_rejected':True,'ten_unique_pixels_accepted':True,'finite_coordinates_not_merged':True,'nonfinite_rejected':True,'threshold':10}
(OUT/'calibration/python-count-check.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
if len(sys.argv)>1:
 load=lambda f:[json.loads(s) for s in (OUT/'calibration'/f).read_text().splitlines()]
 before=load('measurement-before.jsonl');after=load('measurement-after.jsonl')
 assert len(before)==len(after)==12
 arcs=0;visits=0;points=0
 for old,new in zip(before,after):
  assert old['measurement_case_id']==new['measurement_case_id'] and old['success'] and new['success']
  new=json.loads(json.dumps(new));old=json.loads(json.dumps(old));old.pop('code_sha256');new.pop('code_sha256')
  for a,b in zip(old['corners'],new['corners']):
   unique=[count(arc) for arc in b['support_arcs']];lengths=[len(arc) for arc in b['support_arcs']]
   assert b['support_points']==unique and b['support_visits']==lengths and b['support_points_counting']=='unique_pixel_coordinates'
   assert a['support_arcs']==b['support_arcs'] and a['support_points']==lengths
   assert unique==lengths
   arcs+=2;visits+=sum(lengths);points+=sum(unique)
   b.pop('support_visits');b.pop('support_points_counting')
  assert old==new,'non-counting measurement changed'
 result={'cases':12,'arcs':arcs,'visits':visits,'unique_pixels':points,'no_repeats_in_small_C':True,'original_support_sequences_unchanged':True,'all_noncount_fields_exactly_unchanged_excluding_source_hash':True}
 (OUT/'calibration/measurement-comparison.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
