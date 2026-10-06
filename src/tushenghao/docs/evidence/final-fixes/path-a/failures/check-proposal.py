# 仅临时 mock 对照，不改仓库测试；源码来自相邻 proposal.patch。
import subprocess
flags=subprocess.check_output(['pkg-config','--cflags','--libs','opencv4'],text=True).split()
cmd=['g++','-std=c++17','-O2','-Isrc/tushenghao/lib','-Isrc/tushenghao/include','src/tushenghao/docs/evidence/final-fixes/path-a/failures/manual-fixture-proposed.cpp','build/final-fixes-path-a-release/libmark_detector.a',*flags,'-o','/tmp/path-a-manual-proposed']
print('compile argv:',cmd,flush=True)
subprocess.run(cmd,check=True)
subprocess.run(['/tmp/path-a-manual-proposed'],check=True)
