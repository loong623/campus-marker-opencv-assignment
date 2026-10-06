# 目录重组实施记录

按用户批准的[依赖分析及文件清单](include-dependency-analysis.md)实施。完成48个文件移动，未commit。历史分析图/边表保留为审批快照；新旧路径、源文件hash与依赖等价检查见[迁移manifest](layout-reorganization-manifest.json)，正式回归摘要与原始hash见[验证JSON](layout-reorganization-verification.json)。

## 实际布局

- `include/mark/`：3个Detector公开头。
- `lib/`：core9、config3、preprocess2、geometry11、corners14、pipeline3个文件，共6个内部模块。
- `app/main.cpp`：原应用入口，默认配置项目根改用两级parent_path。
- `test_support/block3_fixture.hpp`：测试与资源工具共用fixture；原两级模型路径仍正确。
- `tools/common/file_digest.hpp`：审计/父诊断共用摘要头。
- 测试.cpp和data、其余工具、运行config与旧synth_ref、历史evidence均保留原位置。

## 构建和依赖

仍只有一个编译库mark_detector。PUBLIC头搜索路径为include，PRIVATE为lib；内部头由app/tests/tools显式选择INTERFACE路径目标，非普通Detector消费者的公共路径。test_support也只供测试/资源工具使用。公开头使用mark/前缀，内部跨模块头使用core/config/preprocess/geometry/corners/pipeline前缀。

根CMake与独立工具CMake更新源列表及SHA范围，覆盖移动后的lib/include/app和所需工具/support头。没有将JSON/OpenSSL加入Detector链接依赖。主工程保持原16个CTest名称；独立工具7个可执行目标全部构建通过。README源链接/目录导航已更新。

对迁移前工作区做独立快照并逐文件检查：除include路径与app默认配置根层数外，活动源码的代码内容一致；201条活动本地include边按新旧路径映射完全相等，没有新增耦合。历史诊断源与旧合成器源/头字节不变。运行YAML和模型未编辑；之前未提交的修复及用户.gitignore保留。

## 验证

| 项目 | 结果 |
|---|---|
| 主工程干净Release全量构建 | PASS |
| 独立fixture工具干净Release全量构建 | PASS |
| CTest | 16/16 |
| 仅公开include路径的Detector消费者编译 | PASS |
| 默认配置check，仓库根与/tmp运行 | PASS |
| C正式/隔离 | 720/720，错误有效输出0 |
| H正式/隔离 | 720/720，PASS，错误有效输出0 |
| 全视频 | 1676/1676记录，864个Detection帧，截断0 |
| 与冻结v7逐条对照 | C720、H720、视频1676全部一致，仅忽略耗时和新代码hash |

对照比较全部其余JSON字段，包括四角、方向、bbox、measurement/support evidence、诊断、状态、输入/配置hash和真值评价，未只比较数量。未改变已有视频无标注限制和公共NOT_READY行为。

构建/配置/测试/runner日志与全C/H/video压缩原始结果归档于[evidence/layout-reorganization](../evidence/layout-reorganization)。历史Block3证据及hash不改写。

```sh
cmake -S src/tushenghao -B build/layout-reorg -DCMAKE_BUILD_TYPE=Release
cmake --build build/layout-reorg -j 4
ctest --test-dir build/layout-reorg --output-on-failure
build/layout-reorg/marker_app --check-config
cmake -S src/tushenghao/tools/block3_fixture -B build/layout-reorg-tools -DCMAKE_BUILD_TYPE=Release
cmake --build build/layout-reorg-tools -j 4
```

未git add/commit，留工作区供用户review。CMake路径/摘要更新属于本轮搬迁；算法、公共签名、配置数值和模型坐标保持迁移前状态。
