// Detector 模块的公共接口层组分（定义 外部怎么使用调用 Detector）
#pragma once                                                    

#include <memory>

#include "detector_config.hpp"
#include "detector_types.hpp"

/*
main.cpp                                                 
   |
   | include detector.hpp
   ↓
Detector 类
   |
   ↓
detector.cpp 实现

  -> detector.hpp:

        Detector
           |
           |
   ----------------
   |              |
构造配置       process()
               reset()
               config()


private:
   unique_ptr<Impl>
        |
        ↓
detector.cpp里的真实实现
*/

namespace mark
{
    // 定义检测器类
    class Detector
    {
    public:
        explicit Detector(DetectorConfig config);   // 构造函数，接收配置参数（防止隐式转换（单参数构造函数））

        ~Detector();       // 析构函数，释放资源（可能需要在 .cpp 里处理析构）

        // 禁止拷贝构造和赋值（复制创建和复制赋值）操作（Detector 不允许复制）
        Detector(const Detector &) = delete;

        Detector &operator=(const Detector &) = delete;

        // 处理输入帧，返回处理结果（核心接口）
        FrameResult process(const FrameInput &frame);

        // 重置Detector状态（根据不同原因进行重置）（清空防止污染）（noexcept表示不throw异常）
        void reset(ResetReason reason) noexcept;

        // 获取当前配置（只读访问,返回当前配置）
        const DetectorConfig &config() const noexcept;

    private:
        // PImpl 技巧(内部实现隐藏)：将实现细节隐藏在 Impl 类中，减少头文件依赖（内部实现，外部提供接口，避免重新编译）
        /*
        detector.hpp

        外部看到：
        Detector接口


        detector.cpp

        内部：
        算法细节
        变量
        状态
        */
        struct Impl;

        std::unique_ptr<Impl> impl_;    // 独占所有权的智能指针（不能复制，Detector内部有一个Impl对象由 Detector 独占管理）
    };

} // namespace mark