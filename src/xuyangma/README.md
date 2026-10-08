# Campus Marker Recognition

## Overview

This project implements campus competition marker recognition using traditional computer vision and OpenCV in C++.

The program supports:

- continuous video-frame marker detection;
- marker existence detection;
- marker outer-boundary localization;
- four ordered corner points: `LT`, `RT`, `RB`, `LB`;
- temporal consistency and corner smoothing;
- correct `NOT DETECTED` output when no reliable marker exists;
- camera calibration using a symmetric circle grid;
- optional marker pose estimation using PnP.

No deep learning model or online recognition service is used.

## Environment

The implementation was tested in the following Linux environment:

- OS: Ubuntu 24.04.4 LTS
- Compiler: GCC 13.3.0
- C++ standard: C++17
- CMake: 3.28.3
- OpenCV: 4.6.0

## Dependencies

Install the required packages on Ubuntu:

```bash
sudo apt update
sudo apt install build-essential cmake libopencv-dev
```

The installed OpenCV version can be checked with:

```bash
pkg-config --modversion opencv4
```

## Build

Run the following commands from the repository root:

```bash
cmake -S src/xuyangma -B build/xuyangma -DCMAKE_BUILD_TYPE=Release
cmake --build build/xuyangma --parallel
```

The executable will be generated at:

```text
build/xuyangma/marker_detector
```

## Marker Detection

Run marker detection on the provided video:

```bash
./build/xuyangma/marker_detector data/raw/marker_video.avi
```

To save the processed video:

```bash
./build/xuyangma/marker_detector data/raw/marker_video.avi output.avi
```

When the marker is detected, the program displays:

- the marker outer boundary;
- four corner points;
- the corner labels `LT`, `RT`, `RB`, `LB`;
- `DETECTED`.

When no reliable marker is found, the program displays:

```text
NOT DETECTED
```

The previous detection is not indefinitely reused after the marker disappears.

Press `q` or `Esc` to stop playback.

## Camera Input

The detector also supports camera input:

```bash
./build/xuyangma/marker_detector camera
```

or:

```bash
./build/xuyangma/marker_detector 0
```

## Camera Calibration

The provided calibration target is a `7 x 7` symmetric circles grid.

The center-to-center spacing between adjacent circles is:

```text
0.03 m
```

Run calibration with:

```bash
./build/xuyangma/marker_detector \
--calibrate \
data/raw/calibration_video.avi \
7 7 0.03 \
src/xuyangma/calibration.yaml
```

The calibration program outputs:

- camera intrinsic matrix;
- distortion coefficients;
- calibration RMS error;
- mean reprojection error;
- accepted calibration frame indices.

The current calibration result uses:

```text
Image resolution: 1440 x 1080
Accepted frames: 30
RMS error: 0.06222580695747921
Mean reprojection error: 0.06089088451195195 px
```

The calibration result is stored in:

```text
src/xuyangma/calibration.yaml
```

## Pose Estimation

Pose estimation uses the detected marker corners together with the calibrated camera parameters.

For the current experiment, the marker outer size is treated as:

```text
0.08 m
```

Run pose estimation with:

```bash
./build/xuyangma/marker_detector \
--pose \
data/raw/marker_video.avi \
src/xuyangma/calibration.yaml \
0.08
```

The pose mode displays:

- X translation;
- Y translation;
- Z translation;
- camera-to-marker distance;
- XYZ coordinate axes;
- reprojection error.

Translation and distance values are reported in meters.

## Project Files

The submission directory contains:

```text
src/xuyangma/
├── main.cpp
├── detector.cpp
├── detector.hpp
├── calibration.cpp
├── calibration.hpp
├── CMakeLists.txt
├── README.md
├── REPORT.md
├── calibration.yaml
├── result_demo.avi
└── pose_example.png
```

`result_demo.avi` contains a short continuous marker-detection example.

`pose_example.png` shows an example pose-estimation result.

`calibration.yaml` stores the camera calibration parameters.

More details about the algorithm, parameter choices, failure cases, calibration and pose estimation are provided in `REPORT.md`.