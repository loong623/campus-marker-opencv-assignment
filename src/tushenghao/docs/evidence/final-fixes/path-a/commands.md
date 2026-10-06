# Path A 实际执行命令

全部从仓库根目录执行；完整输出与退出码见同目录 commands.json 的记录。

- 2026-10-06T11:29:35.816637+00:00；退出 0；实测 1.024 s：

  `cmake -S src/tushenghao -B build/final-fixes-path-a-release -DCMAKE_BUILD_TYPE=Release`

  [stdout](tests/release/configure-baseline.stdout) / [stderr](tests/release/configure-baseline.stderr)

- 2026-10-06T11:29:36.910490+00:00；退出 0；实测 0.83 s：

  `cmake -S src/tushenghao -B build/final-fixes-path-a-debug -DCMAKE_BUILD_TYPE=Debug`

  [stdout](tests/debug/configure-baseline.stdout) / [stderr](tests/debug/configure-baseline.stderr)

- 2026-10-06T11:29:45.222557+00:00；退出 0；实测 1.132 s：

  `ffmpeg -v error -i data/raw/marker_video.avi -progress pipe:1 -f null -`

  [stdout](step0/video-decode.stdout) / [stderr](step0/video-decode.stderr)

- 2026-10-06T11:29:45.210553+00:00；退出 0；实测 22.258 s：

  `cmake --build build/final-fixes-path-a-debug -j4`

  [stdout](tests/debug/build-baseline.stdout) / [stderr](tests/debug/build-baseline.stderr)

- 2026-10-06T11:29:45.198346+00:00；退出 0；实测 25.328 s：

  `cmake --build build/final-fixes-path-a-release -j4`

  [stdout](tests/release/build-baseline.stdout) / [stderr](tests/release/build-baseline.stderr)

- 2026-10-06T11:32:58.633279+00:00；退出 0；实测 1.692 s：

  `ctest --test-dir build/final-fixes-path-a-release --output-on-failure`

  [stdout](tests/release/ctest-baseline.stdout) / [stderr](tests/release/ctest-baseline.stderr)

- 2026-10-06T11:32:58.637728+00:00；退出 0；实测 8.226 s：

  `ctest --test-dir build/final-fixes-path-a-debug --output-on-failure`

  [stdout](tests/debug/ctest-baseline.stdout) / [stderr](tests/debug/ctest-baseline.stderr)

- 2026-10-06T11:33:13.062317+00:00；退出 0；实测 0.6 s：

  `build/final-fixes-path-a-release/observability_verify --check-run src/tushenghao/docs/evidence/final-fixes/path-a/baseline --expected-frames 1676 --report src/tushenghao/docs/evidence/final-fixes/path-a/step0/check-baseline.json`

  [stdout](step0/check-baseline.stdout) / [stderr](step0/check-baseline.stderr)

- 2026-10-06T11:34:25.392364+00:00；退出 0；实测 0.085 s：

  `cmake -S src/tushenghao -B build/final-fixes-path-a-release -DCMAKE_BUILD_TYPE=Release`

  [stdout](tests/release/configure-step1.stdout) / [stderr](tests/release/configure-step1.stderr)

- 2026-10-06T11:34:25.555886+00:00；退出 0；实测 2.842 s：

  `cmake --build build/final-fixes-path-a-release --target geometry_anchor_evidence_test -j4`

  [stdout](tests/release/build-step1.stdout) / [stderr](tests/release/build-step1.stderr)

- 2026-10-06T11:34:51.917959+00:00；退出 0；实测 0.006 s：

  `build/final-fixes-path-a-release/geometry_anchor_evidence_test --helper-only`

  [stdout](tests/release/helper-step1.stdout) / [stderr](tests/release/helper-step1.stderr)

- 2026-10-06T11:34:52.073355+00:00；退出 0；实测 1.57 s：

  `cmake --build build/final-fixes-path-a-release --target geometry_anchor_evidence_test geometry_matcher_test -j4`

  [stdout](tests/release/build-step2.stdout) / [stderr](tests/release/build-step2.stderr)

- 2026-10-06T11:36:02.079541+00:00；退出 8；实测 0.027 s：

  `ctest --test-dir build/final-fixes-path-a-release -R '^(geometry_anchor_evidence_test|geometry_matcher_test)$' --output-on-failure -V`

  [stdout](tests/release/ctest-step2.stdout) / [stderr](tests/release/ctest-step2.stderr)

- 2026-10-06T11:36:02.347099+00:00；退出 2；实测 3.271 s：

  `cmake --build build/final-fixes-path-a-release -j4`

  [stdout](tests/release/build-step3.stdout) / [stderr](tests/release/build-step3.stderr)

- 2026-10-06T11:36:35.897670+00:00；退出 0；实测 0.092 s：

  `cmake -S src/tushenghao -B build/final-fixes-path-a-debug -DCMAKE_BUILD_TYPE=Debug`

  [stdout](tests/debug/configure-step3.stdout) / [stderr](tests/debug/configure-step3.stderr)

- 2026-10-06T11:36:34.732624+00:00；退出 0；实测 3.164 s：

  `cmake --build build/final-fixes-path-a-release -j4`

  [stdout](tests/release/build-step3-corrected.stdout) / [stderr](tests/release/build-step3-corrected.stderr)

- 2026-10-06T11:36:36.082352+00:00；退出 0；实测 7.348 s：

  `cmake --build build/final-fixes-path-a-debug -j4`

  [stdout](tests/debug/build-step3.stdout) / [stderr](tests/debug/build-step3.stderr)

- 2026-10-06T11:36:48.869052+00:00；退出 8；实测 1.959 s：

  `ctest --test-dir build/final-fixes-path-a-release --output-on-failure -V`

  [stdout](tests/release/ctest-step3.stdout) / [stderr](tests/release/ctest-step3.stderr)

- 2026-10-06T11:36:48.876078+00:00；退出 8；实测 9.592 s：

  `ctest --test-dir build/final-fixes-path-a-debug --output-on-failure -V`

  [stdout](tests/debug/ctest-step3.stdout) / [stderr](tests/debug/ctest-step3.stderr)

- 2026-10-06T11:37:44.162094+00:00；退出 0；实测 2.324 s：

  `cmake --build build/final-fixes-path-a-release --target path_a_competition_test -j4`

  [stdout](tests/release/build-a11-corrected.stdout) / [stderr](tests/release/build-a11-corrected.stderr)

- 2026-10-06T11:37:45.322978+00:00；退出 0；实测 1.629 s：

  `cmake --build build/final-fixes-path-a-debug --target path_a_competition_test -j4`

  [stdout](tests/debug/build-a11-corrected.stdout) / [stderr](tests/debug/build-a11-corrected.stderr)

- 2026-10-06T11:38:21.496198+00:00；退出 8；实测 1.742 s：

  `ctest --test-dir build/final-fixes-path-a-release --output-on-failure -V`

  [stdout](tests/release/ctest-final-blocked.stdout) / [stderr](tests/release/ctest-final-blocked.stderr)

- 2026-10-06T11:38:23.893537+00:00；退出 0；实测 1.021 s：

  `python3 src/tushenghao/docs/evidence/final-fixes/path-a/failures/check-proposal.py`

  [stdout](failures/proposal-check.stdout) / [stderr](failures/proposal-check.stderr)

- 2026-10-06T11:38:22.656438+00:00；退出 8；实测 9.225 s：

  `ctest --test-dir build/final-fixes-path-a-debug --output-on-failure -V`

  [stdout](tests/debug/ctest-final-blocked.stdout) / [stderr](tests/debug/ctest-final-blocked.stderr)

- 2026-10-06T11:40:33.047281+00:00；退出 0；实测 0.893 s：

  `python3 src/tushenghao/docs/evidence/final-fixes/path-a/failures/check-proposal.py`

  [stdout](failures/proposal-check-archived.stdout) / [stderr](failures/proposal-check-archived.stderr)

- 2026-10-06T11:40:52.554032+00:00；退出 0；实测 1.021 s：

  `cmake --build build/final-fixes-path-a-debug --target manual_validation_check -j4`

  [stdout](tests/debug/build-approved-fixture.stdout) / [stderr](tests/debug/build-approved-fixture.stderr)

- 2026-10-06T11:40:52.563514+00:00；退出 0；实测 1.02 s：

  `cmake --build build/final-fixes-path-a-release --target manual_validation_check -j4`

  [stdout](tests/release/build-approved-fixture.stdout) / [stderr](tests/release/build-approved-fixture.stderr)

- 2026-10-06T11:41:00.517583+00:00；退出 0；实测 1.75 s：

  `ctest --test-dir build/final-fixes-path-a-release --output-on-failure -V`

  [stdout](tests/release/ctest-approved-fixture.stdout) / [stderr](tests/release/ctest-approved-fixture.stderr)

- 2026-10-06T11:41:00.526060+00:00；退出 0；实测 9.547 s：

  `ctest --test-dir build/final-fixes-path-a-debug --output-on-failure -V`

  [stdout](tests/debug/ctest-approved-fixture.stdout) / [stderr](tests/debug/ctest-approved-fixture.stderr)

- 2026-10-06T11:41:10.761947+00:00；退出 0；实测 1.807 s：

  `build/final-fixes-path-a-release/decode_audit data/raw/marker_video.avi 2,1039,1045,1500,1573 src/tushenghao/config/detector_verification.yaml --run-dir src/tushenghao/docs/evidence/final-fixes/path-a/representatives`

  [stdout](step4/representatives.stdout) / [stderr](step4/representatives.stderr)

- 2026-10-06T11:44:55.756413+00:00；退出 0；实测 4.934 s：

  `cmake --build build/final-fixes-path-a-debug -j4`

  [stdout](tests/debug/build-verifier.stdout) / [stderr](tests/debug/build-verifier.stderr)

- 2026-10-06T11:44:55.746904+00:00；退出 0；实测 7.158 s：

  `cmake --build build/final-fixes-path-a-release -j4`

  [stdout](tests/release/build-verifier.stdout) / [stderr](tests/release/build-verifier.stderr)

- 2026-10-06T11:46:07.373277+00:00；退出 1；实测 0.047 s：

  `build/final-fixes-path-a-release/path_a_verify render --candidate-run src/tushenghao/docs/evidence/final-fixes/path-a/representatives --video data/raw/marker_video.avi --review-list src/tushenghao/docs/evidence/final-fixes/path-a/step4/representative_review.csv --output-dir src/tushenghao/docs/evidence/final-fixes/path-a/representative-frames`

  [stdout](step4/render-representatives.stdout) / [stderr](step4/render-representatives.stderr)

- 2026-10-06T11:46:07.499573+00:00；退出 0；实测 0.046 s：

  `build/final-fixes-path-a-release/marker_app --check-config --config src/tushenghao/config/detector_verification.yaml`

  [stdout](step5/check-config.stdout) / [stderr](step5/check-config.stderr)

- 2026-10-06T11:46:07.687938+00:00；退出 1；实测 0.444 s：

  `python3 src/tushenghao/docs/evidence/final-fixes/path-a/step4/negative_cli.py`

  [stdout](step4/negative-cli.stdout) / [stderr](step4/negative-cli.stderr)

- 2026-10-06T11:46:54.248124+00:00；退出 0；实测 5.254 s：

  `cmake --build build/final-fixes-path-a-release --target path_a_verify -j4`

  [stdout](tests/release/build-verifier-manifest.stdout) / [stderr](tests/release/build-verifier-manifest.stderr)

- 2026-10-06T11:47:23.366873+00:00；退出 1；实测 0.931 s：

  `python3 src/tushenghao/docs/evidence/final-fixes/path-a/step4/negative_cli_attempt_02.py`

  [stdout](step4/negative-cli-attempt-02.stdout) / [stderr](step4/negative-cli-attempt-02.stderr)

- 2026-10-06T11:47:22.198495+00:00；退出 0；实测 2.346 s：

  `build/final-fixes-path-a-release/path_a_verify render --candidate-run src/tushenghao/docs/evidence/final-fixes/path-a/representatives --video data/raw/marker_video.avi --review-list src/tushenghao/docs/evidence/final-fixes/path-a/step4/representative_review.csv --output-dir src/tushenghao/docs/evidence/final-fixes/path-a/representative-frames-attempt-02`

  [stdout](step4/render-representatives-attempt-02.stdout) / [stderr](step4/render-representatives-attempt-02.stderr)

- 2026-10-06T11:47:48.315913+00:00；退出 0；实测 0.702 s：

  `python3 src/tushenghao/docs/evidence/final-fixes/path-a/step4/negative_cli_attempt_03.py`

  [stdout](step4/negative-cli-attempt-03.stdout) / [stderr](step4/negative-cli-attempt-03.stderr)

- 2026-10-06T11:48:40.151182+00:00；退出 0；实测 2.539 s：

  `python3 src/tushenghao/docs/evidence/final-fixes/path-a/step4/check_protected.py`

  [stdout](step4/protected.stdout) / [stderr](step4/protected.stderr)

- 2026-10-06T11:48:42.767727+00:00；退出 0；实测 4.657 s：

  `cmake --build build/final-fixes-path-a-debug -j4`

  [stdout](tests/debug/build-formal.stdout) / [stderr](tests/debug/build-formal.stderr)

- 2026-10-06T11:48:42.757895+00:00；退出 0；实测 4.688 s：

  `cmake --build build/final-fixes-path-a-release -j4`

  [stdout](tests/release/build-formal.stdout) / [stderr](tests/release/build-formal.stderr)

- 2026-10-06T11:49:44.904739+00:00；退出 0；实测 1.792 s：

  `ctest --test-dir build/final-fixes-path-a-release --output-on-failure -V`

  [stdout](tests/release/ctest-formal.stdout) / [stderr](tests/release/ctest-formal.stderr)

- 2026-10-06T11:49:46.064068+00:00；退出 0；实测 9.746 s：

  `ctest --test-dir build/final-fixes-path-a-debug --output-on-failure -V`

  [stdout](tests/debug/ctest-formal.stdout) / [stderr](tests/debug/ctest-formal.stderr)

- 2026-10-06T11:49:00.666583+00:00；退出 0；实测 135.381 s：

  `build/final-fixes-path-a-release/marker_app --video data/raw/marker_video.avi --config src/tushenghao/config/detector_verification.yaml --mode debug --run-purpose verification --expected-frames 1676 --run-dir src/tushenghao/docs/evidence/final-fixes/path-a/verification-release`

  [stdout](step5/run-release.stdout) / [stderr](step5/run-release.stderr)

- 2026-10-06T11:51:34.628541+00:00；退出 0；实测 0.541 s：

  `build/final-fixes-path-a-release/observability_verify --check-run src/tushenghao/docs/evidence/final-fixes/path-a/verification-release --expected-frames 1676 --report src/tushenghao/docs/evidence/final-fixes/path-a/check-release.json`

  [stdout](step5/check-release.stdout) / [stderr](step5/check-release.stderr)

- 2026-10-06T11:51:35.233511+00:00；退出 0；实测 2.083 s：

  `build/final-fixes-path-a-release/path_a_verify --baseline-run src/tushenghao/docs/evidence/final-fixes/path-a/baseline --candidate-run src/tushenghao/docs/evidence/final-fixes/path-a/verification-release --config src/tushenghao/config/detector_verification.yaml --expected-frames 1676 --report-dir src/tushenghao/docs/evidence/final-fixes/path-a/report-release`

  [stdout](step5/verify-release.stdout) / [stderr](step5/verify-release.stderr)

- 2026-10-06T11:51:57.779105+00:00；退出 0；实测 8.29 s：

  `build/final-fixes-path-a-release/path_a_verify render --candidate-run src/tushenghao/docs/evidence/final-fixes/path-a/verification-release --video data/raw/marker_video.avi --review-list src/tushenghao/docs/evidence/final-fixes/path-a/report-release/review_required.csv --output-dir src/tushenghao/docs/evidence/final-fixes/path-a/frames`

  [stdout](step6/render-all-review.stdout) / [stderr](step6/render-all-review.stderr)

- 2026-10-06T11:53:20.783143+00:00；退出 0；实测 3.303 s：

  `build/final-fixes-path-a-release/path_a_verify render --candidate-run src/tushenghao/docs/evidence/final-fixes/path-a/verification-release --video data/raw/marker_video.avi --review-list src/tushenghao/docs/evidence/final-fixes/path-a/step6/remaining_review.csv --output-dir src/tushenghao/docs/evidence/final-fixes/path-a/remaining-frames`

  [stdout](step6/render-remaining.stdout) / [stderr](step6/render-remaining.stderr)

- 2026-10-06T11:55:42.164533+00:00；退出 0；实测 10.293 s：

  `python3 src/tushenghao/docs/evidence/final-fixes/path-a/step6/check_png.py`

  [stdout](step6/png-readback.stdout) / [stderr](step6/png-readback.stderr)

- 2026-10-06T11:49:00.676512+00:00；退出 0；实测 1932.289 s：

  `build/final-fixes-path-a-debug/marker_app --video data/raw/marker_video.avi --config src/tushenghao/config/detector_verification.yaml --mode debug --run-purpose verification --expected-frames 1676 --run-dir src/tushenghao/docs/evidence/final-fixes/path-a/verification-debug`

  [stdout](step5/run-debug.stdout) / [stderr](step5/run-debug.stderr)

- 2026-10-06T13:03:59.673666+00:00；退出 0；实测 1.571 s：

  `build/final-fixes-path-a-debug/observability_verify --check-run src/tushenghao/docs/evidence/final-fixes/path-a/verification-debug --expected-frames 1676 --report src/tushenghao/docs/evidence/final-fixes/path-a/check-debug.json`

  [stdout](step5/check-debug.stdout) / [stderr](step5/check-debug.stderr)

- 2026-10-06T13:04:02.001770+00:00；退出 0；实测 1.272 s：

  `build/final-fixes-path-a-release/observability_verify --compare src/tushenghao/docs/evidence/final-fixes/path-a/verification-release src/tushenghao/docs/evidence/final-fixes/path-a/verification-debug --expected-frames 1676 --report src/tushenghao/docs/evidence/final-fixes/path-a/release-debug-compare.json`

  [stdout](step5/release-debug-compare.stdout) / [stderr](step5/release-debug-compare.stderr)

- 2026-10-06T13:04:00.826050+00:00；退出 0；实测 5.476 s：

  `build/final-fixes-path-a-debug/path_a_verify --baseline-run src/tushenghao/docs/evidence/final-fixes/path-a/baseline --candidate-run src/tushenghao/docs/evidence/final-fixes/path-a/verification-debug --config src/tushenghao/config/detector_verification.yaml --expected-frames 1676 --report-dir src/tushenghao/docs/evidence/final-fixes/path-a/report-debug`

  [stdout](step5/verify-debug.stdout) / [stderr](step5/verify-debug.stderr)

- 2026-10-06T13:07:48.894736+00:00；退出 0；实测 1.478 s：

  `python3 src/tushenghao/docs/evidence/final-fixes/path-a/step7/prepare_archive.py`

  [stdout](step7/prepare-archive.stdout) / [stderr](step7/prepare-archive.stderr)

- 2026-10-06T13:10:25.097852+00:00；退出 0；实测 0.122 s：

  `python3 src/tushenghao/docs/evidence/final-fixes/path-a/step7/cleanup_builds.py`

  [stdout](step7/cleanup-builds.stdout) / [stderr](step7/cleanup-builds.stderr)

- 2026-10-06T13:17:24.388122+00:00；退出0；实测检查/完整读取 2.699 s（最终日志/封存序列化不计）；

  `python3 src/tushenghao/docs/evidence/final-fixes/path-a/step7/seal_archive.py`

  [stdout](step7/seal.stdout) / [stderr](step7/seal.stderr)
