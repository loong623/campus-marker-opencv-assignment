//配置模块的（测 config.cpp） 坏配置能被拒（5 种错误都抛 ConfigError）
// 配置错误测试：验证"坏配置"能被正确拒绝：5 种错误情况都抛 ConfigError
//测试部分依然黑盒（等需要改测试时再展开）
/*三步看懂：
1.先看 main()：5 个 if，每个调一个 test 函数，全过才 return 0，挂一个就 return 1
2.再看任意一个 test 函数（5 个长得一样）：造一个"坏"的 YAML → 调 loadConfig → 期望抛 ConfigError → 打印错误 → 删临时文件
3.看中文注释：每个函数头一句话告诉你"这个 case 测什么坏情况"

5 个函数是同一模板，只是"坏"的地方不同（mode 类型 / mode 值 / threshold / 开关 / 文件缺失）。看懂一个，其他四个扫一眼就行。
*/
#include "config/config.hpp"
#include "core/config_error.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace
{

    void writeTestConfig(
        const std::filesystem::path &path,
        const std::string &content)
    {
        std::ofstream file(path);

        if (!file)
        {
            throw std::runtime_error(
                "cannot create test config file");
        }

        file << "%YAML:1.0\n"; // OpenCV FileStorage 需要这个头 定位问题修改1 YAML 头问题修好
        /*工程实践问题记录：定位问题修改1
1. 现象
ctest 显示 50%，第一个 case 有 [expected] 输出，第二个 case 静默失败——连错误都没打印。

2. 定位"哪挂了"
在 main() 每个 if 前加打印 run testXXX。输出停在 run testModeTypeError——第二个 case 挂的。

3. 定位"怎么挂的"
第二个 case 的 catch (const mark::ConfigError&) 没触发。三种可能：

抛了别的异常，被 main 的 catch (...) 吃掉 → 无声 return 1
根本没抛异常，测试返回 false → 无声 return 1
崩溃——但 ctest 会报，不像

把 catch 临时放宽成 catch (const std::exception& e) 并打印，一测便知是哪种。
note:    catch (const mark::ConfigError& e)  // 只抓我们自己的
         catch (const std::exception& e)     // 抓所有标准异常，谁抛的都能看见
         catch (...)                         // 抓一切，但连错误信息都看不到


4. 真相
打印出 Input file is invalid in function 'open'——OpenCV 的 FileStorage 在打开文件时就抛了自己的异常，压根没进到我们的 ConfigError 逻辑。

5. 根因
OpenCV 的 FileStorage 不是通用 YAML 解析器，它要求文件以 %YAML:1.0 开头。测试手写的 YAML 没这个头，OpenCV 拒读。

6. 为啥这样改

改 writeTestConfig 自动加头：一处改，4 个 case 全受益
不动 config.cpp：生产代码没问题，是测试工具造的文件不合格
把诊断用的 std::exception 改回去：最终要验证的就是 ConfigError，诊断工具用完就拆

记住这个套路：定位"哪" → 定位"怎么" → 定位"为啥"，每次只改一处，看数据不猜。*/
        file << content;
    }

    // 测试配置文件不存在时是否抛出 ConfigError。
    bool testMissingFile()
    {
        try
        {
            mark::loadConfig(
                "not_exist_config.yaml");
        }
        catch (const mark::ConfigError &e)
        {
            std::cout << "[expected] " << e.what() << std::endl;

            // 期望：文件不存在时统一抛出 ConfigError。
            return true;
        }

        return false;
    }

    // 测试 detector.mode 类型错误时是否抛出 ConfigError。
    bool testModeTypeError()
    {
        const auto path =
            std::filesystem::temp_directory_path() / "mode_type_error.yaml";

        writeTestConfig(
            path,
            R"(
schema_version: 1
input:
  pixel_format: BGR8
  timestamp_unit: us
preprocess:
  work_width: 640
  work_height: 480
  threshold: 128
detector:
  mode: 123
temporal:
  stabilization_enabled: 0
  display_hold_enabled: 0
  max_hold_frames: 5
output:
  show_window: 0
  show_held_state: 0
debug:
  timing_enabled: 0
  draw_candidates: 0
)");

        try
        {
            mark::loadConfig(path);
        }

        catch (const mark::ConfigError &e)
        {
            std::cout << "[expected] " << e.what() << std::endl;

            // 期望：detector.mode 类型不是字符串时抛出 ConfigError。
            std::filesystem::remove(path);
            return true;
        }

        /*
        catch (const std::exception &e)
        {
          std::cout << "[caught std::exception] " << e.what() << std::endl;

          std::filesystem::remove(path);
          return true;
        }          // 定位问题修改1
        */

        std::filesystem::remove(path);
        return false;
    }

    // 测试 detector.mode 非法值时是否抛出 ConfigError。
    bool testModeValueError()
    {
        const auto path =
            std::filesystem::temp_directory_path() / "mode_value_error.yaml";

        writeTestConfig(
            path,
            R"(
schema_version: 1
input:
  pixel_format: BGR8
  timestamp_unit: us
preprocess:
  work_width: 640
  work_height: 480
  threshold: 128
detector:
  mode: invalid_mode
temporal:
  stabilization_enabled: 0
  display_hold_enabled: 0
  max_hold_frames: 5
output:
  show_window: 0
  show_held_state: 0
debug:
  timing_enabled: 0
  draw_candidates: 0
)");

        try
        {
            mark::loadConfig(path);
        }
        catch (const mark::ConfigError &e)
        {
            std::cout << "[expected] " << e.what() << std::endl;

            // 期望：detector.mode 不支持未知模式时抛出 ConfigError。
            std::filesystem::remove(path);
            return true;
        }

        std::filesystem::remove(path);
        return false;
    }

    // 测试 preprocess.threshold 超出范围时是否抛出 ConfigError。
    bool testThresholdRangeError()
    {
        const auto path =
            std::filesystem::temp_directory_path() / "threshold_error.yaml";

        writeTestConfig(
            path,
            R"(
schema_version: 1
input:
  pixel_format: BGR8
  timestamp_unit: us
preprocess:
  work_width: 640
  work_height: 480
  threshold: 300
detector:
  mode: skeleton
temporal:
  stabilization_enabled: 0
  display_hold_enabled: 0
  max_hold_frames: 5
output:
  show_window: 0
  show_held_state: 0
debug:
  timing_enabled: 0
  draw_candidates: 0
)");

        try
        {
            mark::loadConfig(path);
        }
        catch (const mark::ConfigError &e)
        {
            std::cout << "[expected] " << e.what() << std::endl;

            // 期望：threshold 不在 0~255 范围时抛出 ConfigError。
            std::filesystem::remove(path);
            return true;
        }

        std::filesystem::remove(path);
        return false;
    }

    // 测试开启未实现功能开关时是否抛出 ConfigError。
    bool testUnsupportedFeatureError()
    {
        const auto path =
            std::filesystem::temp_directory_path() / "unsupported_feature.yaml";

        writeTestConfig(
            path,
            R"(
schema_version: 1
input:
  pixel_format: BGR8
  timestamp_unit: us
preprocess:
  work_width: 640
  work_height: 480
  threshold: 128
detector:
  mode: skeleton
temporal:
  stabilization_enabled: 1
  display_hold_enabled: 0
  max_hold_frames: 5
output:
  show_window: 0
  show_held_state: 0
debug:
  timing_enabled: 0
  draw_candidates: 0
)");

        try
        {
            mark::loadConfig(path);
        }
        catch (const mark::ConfigError &e)
        {
            std::cout << "[expected] " << e.what() << std::endl;

            // 期望：未实现功能开启时抛出 ConfigError。
            std::filesystem::remove(path);
            return true;
        }

        std::filesystem::remove(path);
        return false;
    }

} // namespace

// 运行所有测试（主函数）
// ctest --test-dir build/tushenghao --output-on-failure    :  --output-on-failure：测试挂了才显示它的输出。
// 平时 ctest 只告诉你 Passed/Failed，不给细节。现在测的是错误情况——万一挂了，必须看到底哪个 case 挂的、打印了啥，不然没法修。所以加上。
int main()
{
    try
    {
        std::cout << "run testMissingFile" << std::endl;         // 排查哪个 case 失败(工程经验) 定位问题修改1
        if (!testMissingFile())
        {
            return 1;
        }
        std::cout << "run testModeTypeError" << std::endl;
        if (!testModeTypeError())
        {
            return 1;
        }
        std::cout << "run testModeValueError" << std::endl;
        if (!testModeValueError())
        {
            return 1;
        }
        std::cout << "run testThresholdRangeError" << std::endl;
        if (!testThresholdRangeError())
        {
            return 1;
        }
        std::cout << "run testUnsupportedFeatureError" << std::endl;
        if (!testUnsupportedFeatureError())
        {
            return 1;
        }
    }
    catch (...)
    {
        return 1;
    }

    return 0;
}

/*  第一次原版：文件缺失（配置错误）
// 配置错误测试(验证 loadConfig() 遇到错误配置时，会不会正确抛出 ConfigError)
#include "config.hpp"
#include "config_error.hpp"

using namespace mark;

int main()
{
    try      // C++ 异常处理机制（try 包住“可能出错的代码”，如果里面发生异常，就跳到对应的 catch 处理）
    {
        loadConfig("invalid_config.yaml");
    }
    catch (const ConfigError &)    // 我只关心它发生了，不需要读取错误信息（预期结果）只判断类型
    {
        return 0;
    }

    // 如果没有抛出 ConfigError，说明测试失败
    return 1;
}
*/