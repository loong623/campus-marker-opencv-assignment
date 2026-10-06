# 测试侧只读归档检查，不新增CTest或生产依赖；记录完整编译argv。
from pathlib import Path
import subprocess
root=Path.cwd();e=root/'src/tushenghao/docs/evidence/final-fixes/path-a';flags=subprocess.check_output(['pkg-config','--cflags','--libs','opencv4'],text=True).split()
cmd=['g++','-std=c++17','-O2',str(e/'step6/check_png.cpp'),*flags,'-o','/tmp/path-a-check-png'];print('compile argv:',cmd,flush=True);subprocess.run(cmd,check=True)
for name in ['frames','representative-frames-attempt-02','remaining-frames']:
 subprocess.run(['/tmp/path-a-check-png',str(root/'data/raw/marker_video.avi'),str(e/name),str(e/'step6'/('png-'+name+'.json'))],check=True)
