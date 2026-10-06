
#pragma once
#include <nlohmann/json.hpp>
#include <opencv2/core.hpp>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
namespace sg  {
  using Json=nlohmann::json;
  using Point=cv::Point2d;
  using Polygon=std::vector<Point>;
  namespace fs=std::filesystem;
  // 错误携带 CLI 退出码，区分输入、批准和产物失败。
  struct Error:std::runtime_error  {
    int code;
    // 构造携带具体退出码的异常，错误正文保留文件或 case_id。
    Error(int c,const std::string& s):std::runtime_error(s),code(c)  {
    }
  };
  struct Context  {
    Json model,grid,cases,sources;
    std::string config_hash;
    Point center;
  };
  struct Piece  {
    std::string id,type;
    int instance=0;
    Polygon source,ideal,perturbed;
    Json edges,anchor,outer;
    bool deleted=false,invalid=false;
    cv::Matx33d transform=cv::Matx33d::eye();
  };
  struct Scene  {
    Json descriptor,fixture=Json::object();
    std::vector<Piece> pieces;
    bool image=true;
    std::string status="GENERATED",evidence="CONTINUOUS_AND_RASTER_INPUT";
  };
  struct Raster  {
    cv::Mat image,mask,instances;
  };
  // 读取/写入 UTF-8 文本与 JSON，失败给出文件路径。
  std::string read_text(const fs::path& p);
  Json read_json(const fs::path& p);
  void write_text(const fs::path& p,const std::string& s);
  void write_json(const fs::path& p,const Json& j);
  // 使用 OpenSSL EVP 对字节或文件计算 SHA-256。
  std::string hash_text(const std::string& s);
  std::string hash_file(const fs::path& p);
  // JSON 的键排序来自 map；非有限值在序列化前拒绝。
  std::string canonical(const Json& j);
  Json points_json(const Polygon& p);
  Polygon polygon(const Json& j);
  Json matrix_json(const cv::Matx33d& m);
  // 面积保留符号供绕向判断；合法性拒绝自交、重复点和零长边。
  double signed_area(const Polygon& p);
  bool valid_polygon(const Polygon& p);
  bool inside(Point p,const Polygon& poly);
  Context load_context();
  void validate_model(const Json& model,const std::string& markdown);
  // 批准记录只读取，不能由生成器自签；验证模型与有效实验配置摘要。
  Json approval(const Context& c);
  Json environment();
  // 应用已知矩阵而非估计矩阵；透视分母为零时报错。
  Point transform_point(const cv::Matx33d& m,Point p);
  Polygon transform_polygon(const cv::Matx33d& m,const Polygon& p);
  cv::Matx33d affine(const Context& c,const Json& d);
  std::array<double,2> stroke_widths(const cv::Matx33d& m,double w);
  double singular_ratio(const cv::Matx33d& m);
  // 沿外法向移动支撑线，相邻线求交；非法偏移返回 false 并保留样例。
  bool offset_polygon(const Polygon& p,const std::vector<double>& offsets,Polygon& result);
  // 固定 8×8 子点采样，多片并集；实例掩膜独立计算过半覆盖。
  Raster rasterize(const Scene& s,int width=960,int height=720);
  // 回调式枚举不把完整网格加载到内存；case_id 绑定完整配置。
  using Visitor=std::function<void(const Json&)>;
  void enumerate(const Context& c,const std::string& mode,const Visitor& visit);
  void enumerate_special(const Context& c,const Visitor& visit);
  std::string case_id(const Context& c,const Json& descriptor);
  Scene make_scene(const Context& c,const Json& descriptor);
  void apply_special(const Context& c,Scene& s);
  Json truth_record(const Context& c,const Scene& s,const Raster* r);
  // 只检查输入构造：连通域是白片压力与桥接的证据，不是识别算法。
  void check_scene(const Scene& s,const Raster& r);
  fs::path plan(const Context& c,const std::string& run_id);
  fs::path generate(const Context& c,const std::string& mode,const std::string& run_id,const std::string& only="");
  // 核对文件与独立重建的语义真值/PNG；报告明确算法验收尚未运行。
  Json verify(const Context& c,const std::string& run_id,bool save=true,bool pending_generation=false);
  // 运行目录禁止覆盖，run_id 只接受安全的单级目录名。
  fs::path run_path(const std::string& name);
}
