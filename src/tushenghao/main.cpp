// main.cpp 加 --check-config 全链路测试 配置检查模式:程序能正确理解配置文件
// 读 detector.yaml → loadConfig → 校验（validateConfig()） → new Detector（Detector(config.detector_config)） → 打印"配置 OK"。证明配置和 Detector 真能连起来干活。
#include "config.hpp"
#include "detector.hpp"
#include "config_error.hpp"

#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    if (argc == 2 && std::string(argv[1]) == "--check-config")
    {
        try
        {
            // 这里只验证配置加载到 Detector 构造的完整链路，不进入实际检测流程。
            mark::AppConfig config =
                mark::loadConfig(
                    "src/tushenghao/config/detector.yaml");

            // TODO(作业后、赛前修): marker_geometry_path_ 相对路径依赖 CWD。
            // 现状: 必须 cd 到 src/tushenghao/ 再跑 app，否则找不到 config/marker_geometry.yaml 会崩。
            // 修法: 读完 detector.yaml 后，若 marker_geometry_path_ 是相对路径，转成绝对路径。
            //       可参考 tests/detector_contract_test.cpp 里用 __FILE__ 推导的写法。
            //       另需检查 detector.yaml 本身是否也有同类相对路径加载问题。

            // 构造时再次校验 DetectorConfig，确保直接传入配置也满足要求。
            mark::Detector detector(config.detector_config);

            (void)detector;

            std::cout
                << "Config check passed"
                << std::endl;

            return 0;
        }
        catch (const mark::ConfigError &e)
        {
            std::cerr
                << "Config check failed: "
                << e.what()
                << std::endl;

            return 1;
        }
    }

    return 0;
}
