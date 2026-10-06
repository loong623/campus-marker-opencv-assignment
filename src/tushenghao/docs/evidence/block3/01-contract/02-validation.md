# 2026-10-05 最终修复批次验证（阶段验收未完成）

起点/仍在HEAD：`25b6cbebb1afba6fad89a0daba088684f83ea4a3`，分支feat/corner-semantics；用户自行commit，本批次无提交。代码hash由审计构建时计算，包含个人cpp/hpp及CMake；未提交修改不能只凭HEAD复现，应同时保存工作区和hash。

环境：GNU13.3.0、CMake3.28.3、OpenCV4.6.0、OpenSSL3.0.13；合成工具nlohmann_json3.11.3。未安装额外依赖。

| 命令（仓库根目录） | 实际退出码与结果 |
|---|---|
| `cmake -S src/tushenghao -B build/block3-repair -DCMAKE_BUILD_TYPE=Release` | 0 |
| `cmake --build build/block3-repair -j 4` | 0；新增负例后相关两个target再构建0 |
| `ctest --test-dir build/block3-repair --output-on-failure` | 0；最终15/15，0.37s |
| `cmake -S src/tushenghao -B build/block3-clean -DCMAKE_BUILD_TYPE=Release` | 0；独立新目录 |
| `cmake --build build/block3-clean -j 4` | 0；新增负例后相关两个target再构建0 |
| `ctest --test-dir build/block3-clean --output-on-failure` | 0；最终15/15，0.37s |
| `build/block3-repair/marker_app --check-config` | 0；Config check passed |
| `git diff --check` | 0 |
| `ctest --test-dir build/block3-synth-ref --output-on-failure` | 0；旧核心7/7，另见复用检查 |
| C-v1生成、隔离measure、summarize | 均0；720次，结构失败0，216行统计，另见预算/复用检查 |

新增主要检查：真实原图物理角/八边支持、黑原图伪工作证据、缺M/换S/矩形替M、切除指定上边与凸角、粘连、连续帧不复用；三工作尺寸/各向异性resize/只读；六片一对一/乱序ID/既有合法与错误绑定/竞争/资源K边界；近/反平行及圆角连接；凸四点伪证据/错标签/截断/线角不一致/非有限误差；近能量平局/稳定ID/方向unknown；配置finite/未知重复字段/导出往返；公共输入/序列/reset/元数据/NOT_READY。

最终补测先出现两项红结果（两个目录均CTest退出8），原因和处理保留：

- 480×360正例在assignment补全前被fixture预算拒绝，诊断 `assignment/0/SEGMENTED_SUPPORT_INSUFFICIENT`、expansions=0。未放宽0.08相对面积测试预算，新增断言要求它保持保守拒绝；D09原图右下边界测试改为显式已知六片身份输入decodeStage，以隔离坐标契约。故480阶段坐标测试成功不能证明480生产assignment成功。
- 最初上边缺口仅删中间12px，角附近还保留实际连续上边与右边，resolver有足够局部证据成功；该样例不应被称为“指定边缺失”。保留拒绝断言，修正输入为删除整条M上边与凸角；粘连、普通矩形与下一帧黑图另独立拒绝。

完整§9.1负例目录尚未逐条闭合（例如更复杂内外平行竞争、ROI断弧、远交点与真实边界截断）。15/15是测试程序数，不等于全部验收要求已通过。旧Block1/2部分assert在Release下不生效；不夸大其证明力度。

## 单帧工具smoke

```bash
build/block3-repair/decode_audit data/raw/marker_video.avi 1200 > build/block3-frame1200-final.jsonl 2> build/block3-frame1200-final.log
```

视频SHA-256 `aa1219a7a7b702ea1265be8752853a267c982a651afa0f7846f516f9517c9ac7` 已核对。单帧请求仍顺序解码全1676帧，只写1200一条JSON；NOT_READY、无Detection，诊断 `PIPELINE_NOT_READY: assignment及原图定位预算未配置`。这是预算缺失状态检查，不是正式全视频验收。

## 尚未验证

H仅plan，NOT_RUN；正式全视频1676条阶段记录NOT_RUN；V/N/U/O/Q及原32负例标签未确认；V召回/分段、N误检、Q误差、方向覆盖及baseline差异均无分母/无结果。无完整批准仿射范围，无性能/压力通过记录。六项统计获批不关闭这些缺口。
