// 接口契约:给四个物理角发"身份证"
#pragma once

#include <array>
#include <string>

namespace mark
{

    /**
     * @brief 目标物理角编号
     *
     * P0~P3 是图纸中规定的固定角身份。
     * 它们表示“目标自身哪个角”，不表示屏幕中的左上、右上等位置。
     *
     * 即使目标在图像中旋转：
     * - P0 仍然是模型左上完整 L 的外侧角；
     * - P1 仍然是模型右上 M 分段角；
     * - P2 仍然是模型右下完整 L 的外侧角；
     * - P3 仍然是模型左下完整 L 的外侧角。
     *
     * 后续屏幕排序必须先知道物理身份，再建立屏幕位置关系。
     */
    enum class PhysicalCorner
    {
        P0, // 模型左上完整 L 的外侧上边与外侧左边交汇凸角
        P1, // 模型右上分段 M 的外侧上边与外侧右边交汇凸角
        P2, // 模型右下完整 L 的外侧右边与外侧下边交汇凸角
        P3  // 模型左下完整 L 的外侧下边与外侧左边交汇凸角
    };

    /**
     * @brief 物理角到图纸模型的绑定关系:记录"这个物理角去模型哪里找"，不存坐标(坐标在 YAML 里，运行时去 MarkerGeometry 查)
     *
     * 这里只记录：
     * “这个物理角应该去模型哪里找”。
     *
     * 不保存实际坐标：
     * - 坐标属于 MarkerGeometry；
     * - YAML 模型修改后，这里不用同步修改；
     * - 运行时通过 model_piece_id_ 和顶点编号查询模型数据。
     *
     * 不负责：
     * - 图像检测；
     * - 当前角点计算；
     * - 屏幕排序；
     * - 方向判断。
     */
    struct CornerBinding
    {
        /**
         * @brief 物理角编号
         *
         * 表示这个绑定对应 P0~P3 中哪一个固定角。
         *
         * 它保存的是目标语义身份，不是屏幕位置。
         */
        PhysicalCorner physical_corner_;

        /**
         * @brief 对应的模型片段编号
         *
         * 用来找到 MarkerGeometry 中具体哪个几何片段。
         *
         * 例如：
         * - L0
         * - L2
         * - L3
         * - M1
         *
         * 不直接存坐标，避免模型数据出现两份来源。
         */
        std::string model_piece_id_;

        /**
         * @brief 该物理角对应模型顶点编号
         *
         * 编号来自 marker_geometry.yaml 中 vertices 数组下标。
         *
         * 例如：
         * vertices[0] 对应 v0。
         *
         * 这里保存编号，不保存 Point 坐标。
         */
        int vertex_index_;

        /**
         * @brief 构成该凸角的两条模型邻边编号
         *
         * 边编号来自模型顶点顺序：
         * e0 = v0 -> v1，
         * e1 = v1 -> v2，
         * 依次类推，
         * 最后一条边返回 v0。
         *
         * 后续求角时，需要知道是哪两条边形成这个物理角。
         */
        std::array<int, 2> adjacent_edge_indices_;
    };

} // namespace mark