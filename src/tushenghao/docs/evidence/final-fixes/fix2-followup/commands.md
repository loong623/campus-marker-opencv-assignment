# 本轮实际命令与耗时

执行目录为仓库根；全部命令由日志驱动以 argv 执行，没有 Git 调用。时间保存为 UTC，施工记录换算为 Asia/Shanghai。开发期工具安装与 GUI 授权等待独立于命令运行耗时。

版本见 [environment.json](environment.json)、[排版版本](style/versions.json)。

| 动作 | 起止 UTC | 秒 | 退出码 | 日志 |
|---|---|---:|---:|---|
| baseline-release-configure | 2026-10-06T16:32:22.930480+00:00 → 2026-10-06T16:32:23.835594+00:00 | 0.905 | 0 | [log](build-logs/baseline-release-configure.log) |
| baseline-fixture-configure | 2026-10-06T16:32:22.953917+00:00 → 2026-10-06T16:32:24.529880+00:00 | 1.576 | 0 | [log](build-logs/baseline-fixture-configure.log) |
| baseline-release-build | 2026-10-06T16:32:55.522465+00:00 → 2026-10-06T16:33:21.853165+00:00 | 26.331 | 0 | [log](build-logs/baseline-release-build.log) |
| development-tools-install | 2026-10-06T16:33:15.478903+00:00 → 2026-10-06T16:33:23.600934+00:00 | 8.122 | 1 | [log](build-logs/development-tools-install.log) |
| baseline-fixture-build | 2026-10-06T16:33:15.491184+00:00 → 2026-10-06T16:33:37.736424+00:00 | 22.245 | 0 | [log](build-logs/baseline-fixture-build.log) |
| development-tools-install-network | 2026-10-06T16:34:24.230427+00:00 → 2026-10-06T16:34:37.311161+00:00 | 13.081 | 0 | [log](build-logs/development-tools-install-network.log) |
| baseline-video | 2026-10-06T16:34:25.430525+00:00 → 2026-10-06T16:36:38.261780+00:00 | 132.831 | 0 | [log](build-logs/baseline-video.log) |
| baseline-release-tests | 2026-10-06T16:34:25.437535+00:00 → 2026-10-06T16:34:27.265383+00:00 | 1.828 | 0 | [log](build-logs/baseline-release-tests.log) |
| baseline-debug-configure | 2026-10-06T16:34:43.527905+00:00 → 2026-10-06T16:34:44.485124+00:00 | 0.957 | 0 | [log](build-logs/baseline-debug-configure.log) |
| calibration-sample-before | 2026-10-06T16:35:31.163787+00:00 → 2026-10-06T16:35:32.049957+00:00 | 0.886 | 0 | [log](build-logs/calibration-sample-before.log) |
| baseline-debug-build | 2026-10-06T16:35:31.177652+00:00 → 2026-10-06T16:35:51.257705+00:00 | 20.080 | 0 | [log](build-logs/baseline-debug-build.log) |
| calibration-measure-before | 2026-10-06T16:35:42.134587+00:00 → 2026-10-06T16:35:42.373035+00:00 | 0.238 | 0 | [log](build-logs/calibration-measure-before.log) |
| baseline-check-run | 2026-10-06T16:36:49.602780+00:00 → 2026-10-06T16:36:50.172361+00:00 | 0.570 | 0 | [log](build-logs/baseline-check-run.log) |
| baseline-debug-tests | 2026-10-06T16:36:49.614811+00:00 → 2026-10-06T16:36:58.934370+00:00 | 9.320 | 0 | [log](build-logs/baseline-debug-tests.log) |
| f01-release-build | 2026-10-06T16:38:22.243076+00:00 → 2026-10-06T16:38:23.788874+00:00 | 1.546 | 0 | [log](build-logs/f01-release-build.log) |
| f01-fixture-build | 2026-10-06T16:38:22.252739+00:00 → 2026-10-06T16:38:27.671191+00:00 | 5.418 | 0 | [log](build-logs/f01-fixture-build.log) |
| f01-python-tests | 2026-10-06T16:38:22.277409+00:00 → 2026-10-06T16:38:22.308051+00:00 | 0.031 | 0 | [log](build-logs/f01-python-tests.log) |
| f01-cpp-tests | 2026-10-06T16:38:48.346729+00:00 → 2026-10-06T16:38:48.419388+00:00 | 0.073 | 0 | [log](build-logs/f01-cpp-tests.log) |
| calibration-measure-after-count | 2026-10-06T16:38:48.354732+00:00 → 2026-10-06T16:38:48.564130+00:00 | 0.209 | 0 | [log](build-logs/calibration-measure-after-count.log) |
| f01-small-C-comparison | 2026-10-06T16:39:14.419140+00:00 → 2026-10-06T16:39:14.458271+00:00 | 0.039 | 0 | [log](build-logs/f01-small-C-comparison.log) |
| f02-release-target-build | 2026-10-06T16:40:41.751066+00:00 → 2026-10-06T16:40:47.993102+00:00 | 6.242 | 0 | [log](build-logs/f02-release-target-build.log) |
| f02-release-final-target-build | 2026-10-06T16:41:34.981188+00:00 → 2026-10-06T16:41:36.098347+00:00 | 1.117 | 0 | [log](build-logs/f02-release-final-target-build.log) |
| f02-active-tests | 2026-10-06T16:42:51.178250+00:00 → 2026-10-06T16:42:51.201259+00:00 | 0.023 | 0 | [log](build-logs/f02-active-tests.log) |
| functional-release-build | 2026-10-06T16:43:10.757112+00:00 → 2026-10-06T16:43:25.996622+00:00 | 15.240 | 0 | [log](build-logs/functional-release-build.log) |
| functional-debug-build | 2026-10-06T16:43:10.768537+00:00 → 2026-10-06T16:43:26.535965+00:00 | 15.767 | 0 | [log](build-logs/functional-debug-build.log) |
| functional-release-tests | 2026-10-06T16:44:00.725275+00:00 → 2026-10-06T16:44:02.553732+00:00 | 1.828 | 0 | [log](build-logs/functional-release-tests.log) |
| functional-debug-tests | 2026-10-06T16:44:00.734590+00:00 → 2026-10-06T16:44:09.993608+00:00 | 9.259 | 0 | [log](build-logs/functional-debug-tests.log) |
| style-format | 2026-10-06T16:48:30.535087+00:00 → 2026-10-06T16:48:30.891116+00:00 | 0.356 | 0 | [log](build-logs/style-format.log) |
| style-token-check | 2026-10-06T16:48:30.911002+00:00 → 2026-10-06T16:48:36.649018+00:00 | 5.738 | 1 | [log](build-logs/style-token-check.log) |
| style-format-preserve-order | 2026-10-06T16:49:38.064710+00:00 → 2026-10-06T16:49:38.318824+00:00 | 0.254 | 0 | [log](build-logs/style-format-preserve-order.log) |
| style-token-check-final | 2026-10-06T16:49:38.339134+00:00 → 2026-10-06T16:49:45.144808+00:00 | 6.806 | 0 | [log](build-logs/style-token-check-final.log) |
| style-token-check-sections | 2026-10-06T16:50:11.978354+00:00 → 2026-10-06T16:50:18.723521+00:00 | 6.745 | 0 | [log](build-logs/style-token-check-sections.log) |
| style-debug-build | 2026-10-06T16:50:58.492139+00:00 → 2026-10-06T16:51:22.781508+00:00 | 24.289 | 0 | [log](build-logs/style-debug-build.log) |
| style-release-build | 2026-10-06T16:50:58.499806+00:00 → 2026-10-06T16:51:26.926934+00:00 | 28.427 | 0 | [log](build-logs/style-release-build.log) |
| style-fixture-build | 2026-10-06T16:50:58.518573+00:00 → 2026-10-06T16:51:20.633137+00:00 | 22.115 | 0 | [log](build-logs/style-fixture-build.log) |
| independent-geometry-build | 2026-10-06T16:50:58.531366+00:00 → 2026-10-06T16:51:04.496833+00:00 | 5.965 | 0 | [log](build-logs/independent-geometry-build.log) |
| style-release-tests | 2026-10-06T16:52:22.757346+00:00 → 2026-10-06T16:52:24.654078+00:00 | 1.897 | 0 | [log](build-logs/style-release-tests.log) |
| style-debug-tests | 2026-10-06T16:52:22.766049+00:00 → 2026-10-06T16:52:32.093646+00:00 | 9.328 | 0 | [log](build-logs/style-debug-tests.log) |
| final-calibration-measure | 2026-10-06T16:52:22.776763+00:00 → 2026-10-06T16:52:23.083093+00:00 | 0.306 | 0 | [log](build-logs/final-calibration-measure.log) |
| readme-check-detector_verification | 2026-10-06T16:52:23.151084+00:00 → 2026-10-06T16:52:23.208219+00:00 | 0.057 | 0 | [log](build-logs/readme-check-detector_verification.log) |
| readme-check-detector | 2026-10-06T16:52:23.233544+00:00 → 2026-10-06T16:52:23.294666+00:00 | 0.061 | 0 | [log](build-logs/readme-check-detector.log) |
| readme-check-detector_debug | 2026-10-06T16:52:23.316858+00:00 → 2026-10-06T16:52:23.371401+00:00 | 0.055 | 0 | [log](build-logs/readme-check-detector_debug.log) |
| verification-after | 2026-10-06T16:52:44.063171+00:00 → 2026-10-06T16:54:57.578608+00:00 | 133.515 | 0 | [log](build-logs/verification-after.log) |
| readme-baseline | 2026-10-06T16:52:44.073053+00:00 → 2026-10-06T16:54:58.859971+00:00 | 134.787 | 0 | [log](build-logs/readme-baseline.log) |
| readme-debug-display | 2026-10-06T17:04:57.855215+00:00 → 2026-10-06T17:07:20.835750+00:00 | 142.981 | 0 | [log](build-logs/readme-debug-display.log) |
| readme-verification-export | 2026-10-06T17:04:59.106755+00:00 → 2026-10-06T17:07:29.013638+00:00 | 149.907 | 0 | [log](build-logs/readme-verification-export.log) |
| calibration-hash-audit | 2026-10-06T17:06:52.031350+00:00 → 2026-10-06T17:06:52.055340+00:00 | 0.024 | 0 | [log](build-logs/calibration-hash-audit.log) |
| final-regression | 2026-10-06T17:07:22.710788+00:00 → 2026-10-06T17:07:24.059625+00:00 | 1.349 | 0 | [log](build-logs/final-regression.log) |
| final-check-run | 2026-10-06T17:07:22.723325+00:00 → 2026-10-06T17:07:23.304978+00:00 | 0.582 | 0 | [log](build-logs/final-check-run.log) |
| independent-geometry-residuals | 2026-10-06T17:07:22.742147+00:00 → 2026-10-06T17:07:23.798034+00:00 | 1.056 | 0 | [log](build-logs/independent-geometry-residuals.log) |
| readme-export-video-readback | 2026-10-06T17:09:39.359952+00:00 → 2026-10-06T17:09:42.807910+00:00 | 3.448 | 0 | [log](build-logs/readme-export-video-readback.log) |
| readme-verification-check-run | 2026-10-06T17:11:36.967044+00:00 → 2026-10-06T17:11:37.504831+00:00 | 0.538 | 0 | [log](build-logs/readme-verification-check-run.log) |
| readme-debug-check-run | 2026-10-06T17:11:36.974632+00:00 → 2026-10-06T17:11:37.513953+00:00 | 0.539 | 0 | [log](build-logs/readme-debug-check-run.log) |
| final-comment-release-build | 2026-10-06T17:14:43.236810+00:00 → 2026-10-06T17:14:51.308562+00:00 | 8.072 | 0 | [log](build-logs/final-comment-release-build.log) |
| final-comment-debug-build | 2026-10-06T17:14:43.245864+00:00 → 2026-10-06T17:14:50.992505+00:00 | 7.747 | 0 | [log](build-logs/final-comment-debug-build.log) |
| final-comment-token-check | 2026-10-06T17:14:43.256241+00:00 → 2026-10-06T17:14:52.257667+00:00 | 9.001 | 0 | [log](build-logs/final-comment-token-check.log) |
| final-comment-fixture-build | 2026-10-06T17:14:43.268956+00:00 → 2026-10-06T17:14:52.400737+00:00 | 9.132 | 0 | [log](build-logs/final-comment-fixture-build.log) |
| final-review-release-tests | 2026-10-06T17:16:07.579741+00:00 → 2026-10-06T17:16:09.467815+00:00 | 1.888 | 0 | [log](build-logs/final-review-release-tests.log) |
| final-review-debug-tests | 2026-10-06T17:16:07.582108+00:00 → 2026-10-06T17:16:16.930604+00:00 | 9.349 | 0 | [log](build-logs/final-review-debug-tests.log) |
| final-review-measure | 2026-10-06T17:16:07.595759+00:00 → 2026-10-06T17:16:07.895907+00:00 | 0.300 | 0 | [log](build-logs/final-review-measure.log) |
| verification-final-review | 2026-10-06T17:16:39.817553+00:00 → 2026-10-06T17:18:49.798455+00:00 | 129.981 | 0 | [log](build-logs/verification-final-review.log) |
| calibration-hash-final-review | 2026-10-06T17:18:04.520483+00:00 → 2026-10-06T17:18:04.532412+00:00 | 0.012 | 0 | [log](build-logs/calibration-hash-final-review.log) |
| final-review-check-run | 2026-10-06T17:19:49.393061+00:00 → 2026-10-06T17:19:49.930260+00:00 | 0.537 | 0 | [log](build-logs/final-review-check-run.log) |
| final-review-regression | 2026-10-06T17:19:49.402264+00:00 → 2026-10-06T17:19:50.672530+00:00 | 1.270 | 0 | [log](build-logs/final-review-regression.log) |
| final-review-geometry-residuals | 2026-10-06T17:19:49.415375+00:00 → 2026-10-06T17:19:50.384442+00:00 | 0.969 | 0 | [log](build-logs/final-review-geometry-residuals.log) |

## 命令原文

### baseline-release-configure

```bash
cmake -S src/tushenghao -B build/fix2-followup-release -DCMAKE_BUILD_TYPE=Release
```

### baseline-fixture-configure

```bash
cmake -S src/tushenghao/tools/block3_fixture -B build/fix2-followup-fixture -DCMAKE_BUILD_TYPE=Release
```

### baseline-release-build

```bash
cmake --build build/fix2-followup-release -j4
```

### development-tools-install

```bash
python3 -m pip install --target /tmp/fix2-followup-development-tools clang-format==18.1.8 libclang==18.1.1 --disable-pip-version-check
```

### baseline-fixture-build

```bash
cmake --build build/fix2-followup-fixture --target block3_fixture block3_measure -j4
```

### development-tools-install-network

```bash
python3 -m pip install --target /tmp/fix2-followup-development-tools clang-format==18.1.8 libclang==18.1.1 --disable-pip-version-check --no-cache-dir
```

### baseline-video

```bash
build/fix2-followup-release/marker_app --video /home/tushenghao/projects/campus-marker-opencv-assignment/data/raw/marker_video.avi --config src/tushenghao/config/detector_verification.yaml --mode debug --run-purpose verification --expected-frames 1676 --run-dir src/tushenghao/docs/evidence/final-fixes/fix2-followup/baseline
```

### baseline-release-tests

```bash
ctest --test-dir build/fix2-followup-release --output-on-failure --output-log /home/tushenghao/projects/campus-marker-opencv-assignment/src/tushenghao/docs/evidence/final-fixes/fix2-followup/tests/baseline-release.log
```

### baseline-debug-configure

```bash
cmake -S src/tushenghao -B build/fix2-followup-debug -DCMAKE_BUILD_TYPE=Debug
```

### calibration-sample-before

```bash
build/fix2-followup-fixture/block3_fixture sample C src/tushenghao/docs/evidence/final-fixes/fix2-followup/calibration/C-sample
```

### baseline-debug-build

```bash
cmake --build build/fix2-followup-debug -j4
```

### calibration-measure-before

```bash
build/fix2-followup-fixture/block3_measure src/tushenghao/docs/evidence/final-fixes/fix2-followup/calibration/C-sample src/tushenghao/docs/evidence/final-fixes/fix2-followup/calibration/measurement-before.jsonl
```

### baseline-check-run

```bash
build/fix2-followup-release/observability_verify --check-run src/tushenghao/docs/evidence/final-fixes/fix2-followup/baseline --expected-frames 1676 --report src/tushenghao/docs/evidence/final-fixes/fix2-followup/baseline-check.json
```

### baseline-debug-tests

```bash
ctest --test-dir build/fix2-followup-debug --output-on-failure --output-log /home/tushenghao/projects/campus-marker-opencv-assignment/src/tushenghao/docs/evidence/final-fixes/fix2-followup/tests/baseline-debug.log
```

### f01-release-build

```bash
cmake --build build/fix2-followup-release --target corner_edge_fit_test -j4
```

### f01-fixture-build

```bash
cmake --build build/fix2-followup-fixture --target block3_measure -j4
```

### f01-python-tests

```bash
python3 src/tushenghao/docs/evidence/final-fixes/fix2-followup/tools/check_support_count.py
```

### f01-cpp-tests

```bash
ctest --test-dir build/fix2-followup-release -V -R '^corner_edge_fit_test$' --output-log /home/tushenghao/projects/campus-marker-opencv-assignment/src/tushenghao/docs/evidence/final-fixes/fix2-followup/tests/f01-count.log
```

### calibration-measure-after-count

```bash
build/fix2-followup-fixture/block3_measure src/tushenghao/docs/evidence/final-fixes/fix2-followup/calibration/C-sample src/tushenghao/docs/evidence/final-fixes/fix2-followup/calibration/measurement-after.jsonl
```

### f01-small-C-comparison

```bash
python3 src/tushenghao/docs/evidence/final-fixes/fix2-followup/tools/check_support_count.py compare
```

### f02-release-target-build

```bash
cmake --build build/fix2-followup-release --target geometry_matcher_test -j4
```

### f02-release-final-target-build

```bash
cmake --build build/fix2-followup-release --target geometry_matcher_test -j4
```

### f02-active-tests

```bash
ctest --test-dir build/fix2-followup-release -V -R '^geometry_matcher_test$' --output-log /home/tushenghao/projects/campus-marker-opencv-assignment/src/tushenghao/docs/evidence/final-fixes/fix2-followup/tests/f02-geometry.log
```

### functional-release-build

```bash
cmake --build build/fix2-followup-release -j4
```

### functional-debug-build

```bash
cmake --build build/fix2-followup-debug -j4
```

### functional-release-tests

```bash
ctest --test-dir build/fix2-followup-release --output-on-failure --output-log /home/tushenghao/projects/campus-marker-opencv-assignment/src/tushenghao/docs/evidence/final-fixes/fix2-followup/tests/functional-release.log
```

### functional-debug-tests

```bash
ctest --test-dir build/fix2-followup-debug --output-on-failure --output-log /home/tushenghao/projects/campus-marker-opencv-assignment/src/tushenghao/docs/evidence/final-fixes/fix2-followup/tests/functional-debug.log
```

### style-format

```bash
python3 -c 'from pathlib import Path; import subprocess; p=Path("src/tushenghao/docs/evidence/final-fixes/fix2-followup/style/files.txt"); subprocess.run(["/tmp/fix2-followup-development-tools/clang_format/data/bin/clang-format", "--style=file", "-i", *p.read_text().splitlines()], check=True)'
```

### style-token-check

```bash
python3 src/tushenghao/docs/evidence/final-fixes/fix2-followup/style/check_tokens.py
```

### style-format-preserve-order

```bash
python3 -c 'from pathlib import Path; import subprocess; p=Path("src/tushenghao/docs/evidence/final-fixes/fix2-followup/style/files.txt"); subprocess.run(["/tmp/fix2-followup-development-tools/clang_format/data/bin/clang-format", "--style=file", "-i", *p.read_text().splitlines()], check=True)'
```

### style-token-check-final

```bash
python3 src/tushenghao/docs/evidence/final-fixes/fix2-followup/style/check_tokens.py
```

### style-token-check-sections

```bash
python3 src/tushenghao/docs/evidence/final-fixes/fix2-followup/style/check_tokens.py
```

### style-debug-build

```bash
cmake --build build/fix2-followup-debug -j4
```

### style-release-build

```bash
cmake --build build/fix2-followup-release -j4
```

### style-fixture-build

```bash
cmake --build build/fix2-followup-fixture --target block3_fixture block3_measure -j4
```

### independent-geometry-build

```bash
g++ -std=c++17 -O2 src/tushenghao/docs/evidence/final-fixes/fix2-followup/geometry/check_recorded_residuals.cpp -o build/fix2-followup-release/check_recorded_residuals -I/usr/include/opencv4 -lopencv_stitching -lopencv_alphamat -lopencv_aruco -lopencv_barcode -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_cvv -lopencv_dnn_objdetect -lopencv_dnn_superres -lopencv_dpm -lopencv_face -lopencv_freetype -lopencv_fuzzy -lopencv_hdf -lopencv_hfs -lopencv_img_hash -lopencv_intensity_transform -lopencv_line_descriptor -lopencv_mcc -lopencv_quality -lopencv_rapid -lopencv_reg -lopencv_rgbd -lopencv_saliency -lopencv_shape -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_superres -lopencv_optflow -lopencv_surface_matching -lopencv_tracking -lopencv_highgui -lopencv_datasets -lopencv_text -lopencv_plot -lopencv_ml -lopencv_videostab -lopencv_videoio -lopencv_viz -lopencv_wechat_qrcode -lopencv_ximgproc -lopencv_video -lopencv_xobjdetect -lopencv_objdetect -lopencv_calib3d -lopencv_imgcodecs -lopencv_features2d -lopencv_dnn -lopencv_flann -lopencv_xphoto -lopencv_photo -lopencv_imgproc -lopencv_core
```

### style-release-tests

```bash
ctest --test-dir build/fix2-followup-release -V --output-on-failure --output-log /home/tushenghao/projects/campus-marker-opencv-assignment/src/tushenghao/docs/evidence/final-fixes/fix2-followup/tests/style-release.log
```

### style-debug-tests

```bash
ctest --test-dir build/fix2-followup-debug -V --output-on-failure --output-log /home/tushenghao/projects/campus-marker-opencv-assignment/src/tushenghao/docs/evidence/final-fixes/fix2-followup/tests/style-debug.log
```

### final-calibration-measure

```bash
build/fix2-followup-fixture/block3_measure src/tushenghao/docs/evidence/final-fixes/fix2-followup/calibration/C-sample src/tushenghao/docs/evidence/final-fixes/fix2-followup/calibration/measurement-final.jsonl
```

### readme-check-detector_verification

```bash
build/fix2-followup-release/marker_app --check-config --config src/tushenghao/config/detector_verification.yaml
```

### readme-check-detector

```bash
build/fix2-followup-release/marker_app --check-config --config src/tushenghao/config/detector.yaml
```

### readme-check-detector_debug

```bash
build/fix2-followup-release/marker_app --check-config --config src/tushenghao/config/detector_debug.yaml
```

### verification-after

```bash
build/fix2-followup-release/marker_app --video /home/tushenghao/projects/campus-marker-opencv-assignment/data/raw/marker_video.avi --config src/tushenghao/config/detector_verification.yaml --mode debug --run-purpose verification --expected-frames 1676 --run-dir src/tushenghao/docs/evidence/final-fixes/fix2-followup/verification-after
```

### readme-baseline

```bash
build/fix2-followup-release/marker_app --video /home/tushenghao/projects/campus-marker-opencv-assignment/data/raw/marker_video.avi --config src/tushenghao/config/detector.yaml --mode baseline --run-dir src/tushenghao/docs/evidence/final-fixes/fix2-followup/readme/quickstart-baseline-01
```

### readme-debug-display

```bash
build/fix2-followup-release/marker_app --video /home/tushenghao/projects/campus-marker-opencv-assignment/data/raw/marker_video.avi --config src/tushenghao/config/detector_debug.yaml --mode debug --display --run-dir src/tushenghao/docs/evidence/final-fixes/fix2-followup/readme/quickstart-debug-01
```

### readme-verification-export

```bash
build/fix2-followup-release/marker_app --video /home/tushenghao/projects/campus-marker-opencv-assignment/data/raw/marker_video.avi --config src/tushenghao/config/detector_verification.yaml --mode debug --run-purpose verification --export-video --run-dir src/tushenghao/docs/evidence/final-fixes/fix2-followup/readme/quickstart-verification-01
```

### calibration-hash-audit

```bash
cmake -P /home/tushenghao/projects/campus-marker-opencv-assignment/src/tushenghao/docs/evidence/final-fixes/fix2-followup/tools/cmake_hash_audit.cmake
```

### final-regression

```bash
build/fix2-followup-release/observability_verify --compare src/tushenghao/docs/evidence/final-fixes/fix2-followup/baseline src/tushenghao/docs/evidence/final-fixes/fix2-followup/verification-after --expected-frames 1676 --report src/tushenghao/docs/evidence/final-fixes/fix2-followup/regression.json
```

### final-check-run

```bash
build/fix2-followup-release/observability_verify --check-run src/tushenghao/docs/evidence/final-fixes/fix2-followup/verification-after --expected-frames 1676 --report src/tushenghao/docs/evidence/final-fixes/fix2-followup/check-run.json
```

### independent-geometry-residuals

```bash
build/fix2-followup-release/check_recorded_residuals src/tushenghao/docs/evidence/final-fixes/fix2-followup/baseline/frames.jsonl src/tushenghao/docs/evidence/final-fixes/fix2-followup/verification-after/frames.jsonl src/tushenghao/config/marker_geometry.yaml src/tushenghao/docs/evidence/final-fixes/fix2-followup/geometry/real-parent-residuals.jsonl src/tushenghao/docs/evidence/final-fixes/fix2-followup/geometry/real-parent-summary.json
```

### readme-export-video-readback

```bash
build/fix2-followup-release/final_fixes_verify video --input src/tushenghao/docs/evidence/final-fixes/fix2-followup/readme/quickstart-verification-01/overlay.mp4 --expected-frames 1676 --width 1440 --height 1080 --report src/tushenghao/docs/evidence/final-fixes/fix2-followup/readme/video-readback.json
```

### readme-verification-check-run

```bash
build/fix2-followup-release/observability_verify --check-run src/tushenghao/docs/evidence/final-fixes/fix2-followup/readme/quickstart-verification-01 --expected-frames 1676 --report src/tushenghao/docs/evidence/final-fixes/fix2-followup/readme/verification-check.json
```

### readme-debug-check-run

```bash
build/fix2-followup-release/observability_verify --check-run src/tushenghao/docs/evidence/final-fixes/fix2-followup/readme/quickstart-debug-01 --expected-frames 1676 --report src/tushenghao/docs/evidence/final-fixes/fix2-followup/readme/debug-check.json
```

### final-comment-release-build

```bash
cmake --build build/fix2-followup-release -j4
```

### final-comment-debug-build

```bash
cmake --build build/fix2-followup-debug -j4
```

### final-comment-token-check

```bash
python3 src/tushenghao/docs/evidence/final-fixes/fix2-followup/style/check_tokens.py
```

### final-comment-fixture-build

```bash
cmake --build build/fix2-followup-fixture --target block3_measure -j4
```

### final-review-release-tests

```bash
ctest --test-dir build/fix2-followup-release -V --output-on-failure --output-log /home/tushenghao/projects/campus-marker-opencv-assignment/src/tushenghao/docs/evidence/final-fixes/fix2-followup/tests/final-release.log
```

### final-review-debug-tests

```bash
ctest --test-dir build/fix2-followup-debug -V --output-on-failure --output-log /home/tushenghao/projects/campus-marker-opencv-assignment/src/tushenghao/docs/evidence/final-fixes/fix2-followup/tests/final-debug.log
```

### final-review-measure

```bash
build/fix2-followup-fixture/block3_measure src/tushenghao/docs/evidence/final-fixes/fix2-followup/calibration/C-sample src/tushenghao/docs/evidence/final-fixes/fix2-followup/calibration/measurement-final-review.jsonl
```

### verification-final-review

```bash
build/fix2-followup-release/marker_app --video /home/tushenghao/projects/campus-marker-opencv-assignment/data/raw/marker_video.avi --config src/tushenghao/config/detector_verification.yaml --mode debug --run-purpose verification --expected-frames 1676 --run-dir src/tushenghao/docs/evidence/final-fixes/fix2-followup/verification-final-review
```

### calibration-hash-final-review

```bash
cmake -P src/tushenghao/docs/evidence/final-fixes/fix2-followup/tools/cmake_hash_audit_final_review.cmake
```

### final-review-check-run

```bash
build/fix2-followup-release/observability_verify --check-run src/tushenghao/docs/evidence/final-fixes/fix2-followup/verification-final-review --expected-frames 1676 --report src/tushenghao/docs/evidence/final-fixes/fix2-followup/check-run-final-review.json
```

### final-review-regression

```bash
build/fix2-followup-release/observability_verify --compare src/tushenghao/docs/evidence/final-fixes/fix2-followup/baseline src/tushenghao/docs/evidence/final-fixes/fix2-followup/verification-final-review --expected-frames 1676 --report src/tushenghao/docs/evidence/final-fixes/fix2-followup/regression-final-review.json
```

### final-review-geometry-residuals

```bash
build/fix2-followup-release/check_recorded_residuals src/tushenghao/docs/evidence/final-fixes/fix2-followup/baseline/frames.jsonl src/tushenghao/docs/evidence/final-fixes/fix2-followup/verification-final-review/frames.jsonl src/tushenghao/config/marker_geometry.yaml src/tushenghao/docs/evidence/final-fixes/fix2-followup/geometry/final-parent-residuals.jsonl src/tushenghao/docs/evidence/final-fixes/fix2-followup/geometry/final-parent-summary.json
```

## 可复现说明

从当前 README 构建；旧源码快照在 source-before，配置/公共头/参考生成器受保护且未改。如重建修改前基线，在独立临时目录组合旧源码及这些未改输入，不回覆盖工作树。小样 truth/manifest/PNG、前后 jsonl、全部视频运行记录和原始输入路径/hash 已归档，原视频不复制进证据。
复核无需保留此次 build：`python3 src/tushenghao/docs/evidence/final-fixes/fix2-followup/tools/final_scope_check.py` 读取封存记录和当前源码；其 Clang 词法依赖按本页实际 pip 命令安装到临时目录。独立几何核查源码及编译命令已存档，复用新建 build 后可重跑，输出必须是新路径。
排版快照与 token 清单只含本方案范围；README 三模式在最后一行注释审校之前运行，实际旧字节变体在 source-variants，算法 token 相同。最后完整验证使用终版源码字节指纹。
hashes.json 的 artifacts 保存最终文件 hash（不含它自身），所有旧报告历史前缀另核保留。

### final-scope-before-cleanup

```bash
python3 src/tushenghao/docs/evidence/final-fixes/fix2-followup/tools/final_scope_check.py
```

退出码 0，实际耗时 4.836s，UTC 2026-10-06T17:24:22.141963+00:00 → 2026-10-06T17:24:26.977838+00:00；[日志](build-logs/final-scope-before-cleanup.log)。

### cleanup-builds

```bash
python3 src/tushenghao/docs/evidence/final-fixes/fix2-followup/tools/cleanup_builds.py
```

退出码 0，实际耗时 0.096s，UTC 2026-10-06T17:26:09.906131+00:00 → 2026-10-06T17:26:10.002464+00:00；[日志](build-logs/cleanup-builds.log)。

### final-scope-after-cleanup

```bash
python3 src/tushenghao/docs/evidence/final-fixes/fix2-followup/tools/final_scope_check.py
```

退出码 0，实际耗时 3.814s，UTC 2026-10-06T17:26:52.951988+00:00 → 2026-10-06T17:26:56.766054+00:00；[日志](build-logs/final-scope-after-cleanup.log)。
