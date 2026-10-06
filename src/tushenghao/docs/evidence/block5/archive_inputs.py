"""无损保留已有H输入和不可重建的原基线失败复现；旧目录与旧证据保持只读。"""
import json
import shutil
import tarfile
from archive_checks import ROOT, EVIDENCE, digest

source = ROOT / 'build/block3-H-v1'
archive = EVIDENCE / 'archive/H-existing-input.tar.gz'
assert source.is_dir() and not archive.exists()
files = []
for path in sorted(source.rglob('*')):
    if path.is_file():
        files.append({'path': path.relative_to(source).as_posix(),
                      'size': str(path.stat().st_size), 'sha256': digest(path)})
with tarfile.open(archive, 'w:gz') as stream:
    stream.add(source, arcname='H-existing-input')
# 逐项读取无损归档内容并核SHA，不仅检查压缩文件自身能打开。
import hashlib
with tarfile.open(archive, 'r:gz') as stream:
    for entry in files:
        with stream.extractfile('H-existing-input/' + entry['path']) as data:
            assert hashlib.file_digest(data, 'sha256').hexdigest() == entry['sha256']
with (EVIDENCE / 'archive/H-input-manifest.json').open('x') as stream:
    json.dump({'source': 'build/block3-H-v1', 'condition': '已有H720输入；未生成新网格',
               'archive': archive.relative_to(EVIDENCE).as_posix(), 'sha256': digest(archive),
               'entries': files, 'result': 'PASS'}, stream, ensure_ascii=False, indent=2)
    stream.write('\n')
binary = EVIDENCE / 'step0/original-geometry-matcher-assert-linux-x86_64'
shutil.copy2(ROOT / 'build/block5-debug/geometry_matcher_original_baseline', binary)
with (EVIDENCE / 'step0/original_geometry_binary_manifest.json').open('x') as stream:
    json.dump({'path': binary.relative_to(EVIDENCE).as_posix(), 'size': str(binary.stat().st_size),
               'sha256': digest(binary), 'platform': 'Linux x86_64; GCC13.3; OpenCV4.6',
               'source': 'unchanged geometry_matcher_test.cpp; original baseline static library',
               'purpose': '保留原基线同一Debug assert的实际失败复现；不是普通新构建二进制',
               'compile_and_failure': 'comparison/legacy_debug_failure.json'}, stream, ensure_ascii=False, indent=2)
    stream.write('\n')
print('PASS archived input entries', len(files), 'and original baseline assertion binary')
