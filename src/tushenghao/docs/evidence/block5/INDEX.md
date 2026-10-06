# Block5 实施与验收证据

基线分支`feat/observability`，起点`b9cccd4a86ac5959a8c67559cde1d399ba29e046`。完整Debug旧失败保留；不以当前Release通过冒称所有旧assert通过。
总体结论见[验收页](../../block5_acceptance.md)，字段语义见[schema](../../diagnostics_schema.md)，运行入口见[README](../../../README.md)。

|证据|入口|
|---|---|
|原始/补丁源与金样|[step0目录](step0/)、[Route B比较](step0/route_b_comparison.json)、[C++只读复核](comparison/route-b-cpp-check.json)|
|最终源/输入/环境|[源hash](environment/final_source_manifest.json)、[工具链](environment/toolchain.json)、[视频身份](environment/video_identity.json)、[最终HEAD直接读取](environment/final_branch_state.json)|
|测试及所有失败日志|[tests](tests/)、[命令与退出码](archive/commands.json)、[主动34组及旧30组](tests/step7-final-active-02.log)|
|正式baseline|[summary](runs/app-baseline-03/summary.yaml)、[manifest](runs/app-baseline-03/manifest.yaml)、[有效配置](runs/app-baseline-03/effective_config.yaml)、[schema核验](comparison/baseline-check-03.json)|
|公共debug验证|[完整run](runs/app-debug-03/)、[逐帧对照](comparison/patched-debug-03.json)、[输出约束](comparison/video-invariants.json)、[schema核验](comparison/debug-check-03.json)|
|geometry/decode/temporal all|[geometry](runs/geometry-all-01/)、[decode](runs/decode-all-01/)、[temporal](runs/temporal-all-01/)、[共同raw](comparison/patched-decode-all.json)、[共同stable](comparison/patched-temporal-all.json)|
|网格原实验|[15360条run](runs/grid-01/)、[逐字段对照](comparison/grid-comparison.json)|
|H720|[最终逐例记录](comparison/H-final-regression.jsonl)、[旧金样对照](comparison/H-final-comparison.json)、[无损H输入](archive/H-input-manifest.json)|
|既有Debug失败|[原基线复现](comparison/legacy_debug_failure.json)、[归档二进制实际复现](comparison/archived-legacy-replay.json)、[完整Debug日志](tests/step7-evidence-debug-full-03.log)|
|实际错误用例|[19类CLI](comparison/negative-cases.json)、[零帧EOF](comparison/zero-frame-check.json)|
|完整性与清理|[archive目录](archive/)：artifact-manifest.json、final-archive-check-before/after-cleanup.json、final-archive-check-cleanup.json；最终命令另在final-archive-check-commands.json避免自引用|

01/02 App运行、首次缺产物H、首次错误verify与编译失败均保留作过程证据；正式App以03为准。性能只用baseline公共调用，Step0旧共享装配与audit无public总耗时。所有JSONL保留原文，不以摘要替代逐帧。

重放（先按README重新构建，报告路径必须新建）：

```bash
build/block5/observability_verify --compare src/tushenghao/docs/evidence/block5/step0/patched-baseline.jsonl src/tushenghao/docs/evidence/block5/runs/app-debug-03 --report /tmp/block5-replay-new.json
build/block5/observability_verify --check-run src/tushenghao/docs/evidence/block5/runs/geometry-all-01 --report /tmp/block5-geometry-new.json
build/block5/observability_verify --check-archive src/tushenghao/docs/evidence/block5 --report /tmp/block5-archive-new.json
```

[只读归档核查脚本](archive_checks.py)验证链接、当前源码、实际采样及全帧覆盖；[网格比较](compare_grid.py)、[视频约束](video_invariants.py)、[CLI错误](negative_cases.py)均为工具侧核验，不进入生产算法。重跑脚本需改新报告/运行名称，不覆盖本证据。[命令记录器](run_command.py)、[阶段顺序执行](run_stage_audits.py)、[零帧fixture](make_io_fixture.py)、[已有输入归档](archive_inputs.py)、[交付页生成](write_delivery.py)、[最终归档/限定清理](finalize_archive.py)全部登记，构建目录中无唯一必要脚本。

旧基线两个Linux平台二进制是不可从修改后源码直接重建的来源例外；普通新二进制不归档。H原输入在archive/H-existing-input.tar.gz，无损逐项SHA校验，原用户H目录保留。最终核验实际解析所有JSONL并校验manifest路径、大小与SHA；仅四个任务标识匹配的Block5目录清理，旧build、视频、Block3/4证据不清理。完整Debug失败仍在归档中并可复现。
