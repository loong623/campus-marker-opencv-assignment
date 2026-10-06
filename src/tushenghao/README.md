# MARK 检测实现（Block3 WIP）

当前内部阶段的 C、H 均为720/720；冻结v7全视频1676帧中864帧产生Detection。公共 `Detector::process()` 的稳定层尚未实现，仍返回 `NOT_READY`。视频未标注，检出数量不代表正确率；V按用户决定暂时跳过。

从[文档索引](docs/INDEX.md)、[Block3当前状态](docs/block3.md)、[适用边界与已知局限](docs/evidence/block3/05-L-observation-v5-v7/适用边界与已知局限.md)开始阅读。历史试验及失败版本见[证据索引](docs/evidence/block3/INDEX.md)，工具用法见[工具索引](tools/INDEX.md)。

## 目录结构

```text
src/tushenghao/
├── include/mark/            # Detector公开接口、输入输出类型和配置类型
├── lib/
│   ├── core/                # 共享类型、模型、基础度量及预算检查
│   ├── config/              # 配置加载、校验和导出
│   ├── preprocess/          # 工作图和原图上下文准备
│   ├── geometry/            # L观察、父生成/验证、M/S补全
│   ├── corners/             # 原图角点取证、排序、语义及输出校验
│   └── pipeline/            # Detector实现与唯一阶段调度入口
├── app/main.cpp             # 配置检查应用入口
├── test_support/            # 测试与资源工具共用fixture
├── tests/                   # 16个测试程序及固定反例data/
├── tools/
│   ├── audit/               # 当前帧/全视频阶段审计
│   ├── common/              # 工具共用摘要头
│   ├── block3_fixture/      # 生成、校准、验证、诊断、报表；独立CMake入口
│   └── synth_ref/           # 原参考生成器及其历史产物，保持原位
├── config/                  # 运行YAML和已核对模型坐标
└── docs/                    # 当前记录、冻结参考、架构与分批证据
```

库使用者包含 `<mark/detector.hpp>`。只有include是主库PUBLIC头路径；lib内部头供工程内app、测试与审计显式使用。内部头与实现共置，仍组成一个mark_detector库。详见[依赖划分](docs/architecture/include-dependency-analysis.md)和[源码迁移记录](docs/architecture/layout-reorganization.md)。

## 构建与测试

从仓库根目录执行，需要C++17、CMake、OpenCV和OpenSSL；独立fixture工具另需nlohmann_json。主检测库不直接依赖JSON/OpenSSL，不使用GTest。

```bash
cmake -S src/tushenghao -B build/tushenghao -DCMAKE_BUILD_TYPE=Release
cmake --build build/tushenghao -j 4
ctest --test-dir build/tushenghao --output-on-failure
build/tushenghao/marker_app --check-config
```

配置check只验证加载/校验/构造，不表示检测或稳定层验收完成。注册16个CTest程序；新增Block3检查在Release也主动报告失败，既有部分assert测试在Release的覆盖限制保留。

独立工具的构建入口、目标名保持不变：

```bash
cmake -S src/tushenghao/tools/block3_fixture -B build/block3-fixture -DCMAKE_BUILD_TYPE=Release
cmake --build build/block3-fixture -j 4
```

## 阶段审计

[runDecodePipeline](lib/pipeline/decode_stage.hpp)复用正式阶段模块，不读取合成真值。公共process仍由稳定层门控；离线审计通过decode_audit取得阶段Detection。

```bash
build/tushenghao/decode_audit data/raw/marker_video.avi 1200
build/tushenghao/decode_audit data/raw/marker_video.avi all src/tushenghao/docs/evidence/block3/frozen_detector_v7.yaml > build/video-audit.jsonl
```

默认配置为[detector.yaml](config/detector.yaml)，移动审计源文件后由CMake提供绝对配置路径，不依赖工作目录。冻结v1–v7的YAML和[ref/](docs/ref/)保留原路径、原内容。

输出四角由当前原图真实连续支持弧普通L2拟合求交；模型投影仅用于搜索。物理P0/P1/P2/P3对应L0/M1/L2/L3指定外角，输出顺序为屏幕LT/RT/RB/LB，bbox由当前四点计算。方向不唯一可为unknown，搜索截断不宣称唯一；marker_code和confidence保持空。

输入为BGR8，公共入口要求frame_id严格增加、timestamp_us非负且不倒退；reset清除序列状态。工作像素中心映射为 `(work+0.5)/scale-0.5`。

## 当前结果与限制

H无噪声144/144、噪声576/576，原689和前705逐ID无回退，最大真值误差1.3462912原图px。全视频864个检测帧，相对v4保留581、增加283，10个旧检测帧因新增竞争未排除而保守拒绝。未输出812帧的互斥归因及逐帧证据见[最终回归](docs/evidence/block3/05-L-observation-v5-v7/H-video-v7.md)。

白阈值严格gray>200；325暗帧未进入白色掩膜，未标注不判漏检。未验证视频召回/误检率、Q人工定位误差、一般透视与任意尺度/亮度覆盖，也未承诺硬实时。模型绑定配置化和完整§9.1仍未闭合，整体Block3保持WIP。预算依据见[预算表](docs/block3_budget.md)，系统保证与不保证的范围见[适用边界](docs/evidence/block3/05-L-observation-v5-v7/适用边界与已知局限.md)。

最新文件整理与构建检查见[整理验证](docs/architecture/docs-tools-organization.md)。提交由用户review后操作。
