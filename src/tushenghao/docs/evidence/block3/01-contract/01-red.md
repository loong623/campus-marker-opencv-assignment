# 修复前回归记录（2026-10-05）

基线：`25b6cbe`，分支 `feat/corner-semantics`；与 `99bc72c` 的代码一致。
用户已有 `.gitignore` 改动未纳入修复。环境：GNU C++ 13.3.0、OpenCV 4.6.0、GTest 1.14.0。

| 命令 | 退出码 | 结果 |
|---|---:|---|
| `cmake -S src/tushenghao -B build/block3-repair -DCMAKE_BUILD_TYPE=Release` | 0 | 配置成功 |
| `cmake --build build/block3-repair -j 4` | 0 | 构建成功 |
| `ctest --test-dir build/block3-repair --output-on-failure`（新增测试前） | 0 | 原 12/12 通过 |
| `ctest --test-dir build/block3-repair -R 'block3_contract_regression_test\|detector_contract_test' --output-on-failure`（新增测试后） | 8 | 两个测试程序均失败 |

实测失败：

- `invalid_input`：灰度输入触发 OpenCV `Invalid number of channels` 异常，而非 INVALID_INPUT。
- `sequence`：合法输入返回非 NOT_READY，绕过尚未实现的稳定层。
- `near_tie`：容差内后续更小能量清除 tie。
- `nonfinite`：非有限四点被排序接受。
- `zero_semantic_threshold`：相同四点在零门限下被判冲突。
- `detector_contract_test`：新增 NOT_READY、metadata、空载荷断言失败；不再丢弃 result。

这些是代码缺陷的运行反例，不是 G3/G4 正式预算或合成/视频验收结果。
