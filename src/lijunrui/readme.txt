白色发光装甲板四角点识别

使用了c++17和opencv，主要流程为滤波，二值化，形态学处理，轮廓提取，获取发光条的六角点并进行形态判断，最后进行灯条的匹配

1.构建与运行
    需要 CMake 3.16 及以上版本、C++17 编译器和支持 MP4 读写的 OpenCV 4 开发包。

    请将CMakeLists.txt置于项目文件夹下
    cmake -S . -B build
    cmake --build build -j
    ./build/detector_demo data/raw/marker_video.avi results/output.mp4

    每段视频会输出以下内容，全部与输入视频保持相同帧率，由 OpenCV 的 `VideoWriter` 直接写为 `mp4v` 编码的 MP4。程序不再进行外部转码；
    新生成的视频可能无法在 VS Code 内置播放器中直接打开，可使用支持 MPEG-4 Part 2 的播放器查看。

    读取的视频为 data/raw/marker_video.avi
    生成的视频保存在 results/optput.mp4

2.流程与参数
    主体思路：只关注四角上的 “L” 型灯条

    流程：
    1.滤波：采用 3*3 的中值滤波
    2.二值化：将bgr均大于min_color的设为 1
    3.形态学处理：先开运算后闭运算，均使用 3*3 kernel
    4.轮廓提取：使用findContours得到外轮廓上的若干关键点
    5.获取灯条角点：
        ①找外轮廓和凸包，在外轮廓上却不在凸包上的点就是内顶点
        ②根据下标得出外顶点
    6.灯条匹配：
        ①根据装甲板中心，内顶点，外顶点三点共线判断
        ②根据内（外）顶点构成的四边形对边平行判断

3.总体说明：
    对于正置的装甲板识别率很高
    可以实现对有旋转角度、有倾角的装甲板的识别
    因为判断四角分别是哪个暂时没用，故先不写（先判断灯条面积，再做极角排序（通过convexHull））