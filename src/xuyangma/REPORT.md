# Campus Marker Recognition Report

## 1. Task Overview

The goal of this project is to detect the campus competition marker from continuous video frames using only traditional computer vision methods and OpenCV.

For each frame, the program determines whether the marker is present.

When the marker is detected, the program outputs:

- the marker outer region;
- four outer corner points;
- the corner order `LT`, `RT`, `RB`, `LB`;
- a visualization of the detection result.

When no reliable marker is found, the program outputs:

```text
NOT DETECTED
```

The program does not indefinitely reuse the previous frame result after the marker disappears.

In addition to the required detection task, camera calibration and marker pose estimation are also implemented.

---

## 2. Marker Visual Structure

The target is a planar marker with an approximately square dark outer panel and a fixed bright internal pattern.

The marker drawing shows an outer size of approximately:

```text
80 x 80
```

The most useful feature is not only the square outer boundary, but the asymmetric internal bright structure.

Three locations contain similar L-shaped bright patterns:

- upper-left;
- lower-left;
- lower-right.

The upper-right region contains a different multi-part bright pattern.

Therefore, the marker can be identified using both:

1. local bright-component shape;
2. global geometric layout.

This asymmetric structure also provides orientation information and helps distinguish the marker from ordinary bright rectangular objects in the scene.

---

## 3. Main Detection Strategy

The final implementation uses a coarse-to-fine detection pipeline.

```text
Input frame
    |
    v
HSV bright-region segmentation
    |
    v
Bright contour extraction
    |
    v
L-shape candidate filtering
    |
    v
Three-L geometric-layout search
    |
    v
Upper-right special-pattern verification
    |
    v
Coarse marker geometry estimation
    |
    v
Perspective-normalized marker verification
    |
    v
Outer-edge refinement
    |
    v
Temporal consistency
    |
    v
DETECTED / NOT DETECTED
```

The main design idea is:

> First generate relatively permissive candidates, then use marker-specific geometry to verify them.

Marker identity and marker boundary localization are treated as two different problems.

The internal bright pattern is mainly used to determine whether an object is the required marker.

The outer dark boundary is then refined separately to obtain more accurate corner positions.

---

## 4. Bright Region Segmentation

The original image is provided in BGR format.

It is first converted to HSV.

The marker light regions generally have:

- high brightness;
- relatively low saturation.

The detector therefore uses both the Saturation channel and the Value channel.

The Value channel is thresholded using Otsu thresholding together with upper and lower limits.

This avoids depending entirely on one fixed brightness threshold.

The Saturation channel is used to suppress strongly colored regions.

The two binary masks are combined to obtain a bright-region mask.

A small `3 x 3` morphological kernel is then used for:

- morphological opening;
- morphological closing.

Opening helps remove isolated small noise.

Closing helps reconnect small gaps inside valid bright components.

---

## 5. Bright Component Extraction

Contours are extracted from the bright binary mask using OpenCV `findContours`.

The detector rejects components that are clearly unreasonable according to:

- area relative to total image area;
- minimum width and height;
- extreme aspect ratio.

This stage is intentionally permissive.

Its purpose is candidate generation rather than final marker recognition.

A component that passes this stage is only considered a possible marker feature.

---

## 6. L-Shape Model

The marker contains three similar L-shaped bright components.

The marker drawing provides useful geometric dimensions, including approximately:

```text
30
22
8
```

These dimensions define the basic L structure.

A normalized L-shape polygon is constructed using:

```text
(0,0)
(30,0)
(30,8)
(8,8)
(8,30)
(0,30)
```

Each candidate contour is compared with this L model using OpenCV:

```text
matchShapes
```

The detector also checks the foreground fill ratio inside the component bounding box.

The expected fill ratio is derived from the approximate geometry of the two L arms.

Combining contour-shape similarity and fill ratio helps reject:

- ordinary bright squares;
- long bright bars;
- unrelated robot structures;
- small reflections.

---

## 7. Three-L Geometric Layout

A valid marker should contain three L-shaped components in approximately the following marker positions:

```text
upper-left        upper-right special pattern


lower-left        lower-right
```

The three L-shaped components correspond to:

```text
upper-left
lower-left
lower-right
```

The detector searches combinations of L candidates and checks their geometric consistency.

The following properties are considered:

- distance between components;
- horizontal and vertical scale;
- angle between the two main directions;
- area similarity between the three L components.

The three components should form a structure similar to two approximately perpendicular marker axes.

Candidates with extreme scale difference or strongly inconsistent geometry are rejected.

---

## 8. Coarse Marker Coordinate Model

After the three L-shaped components are identified, their centers are used to estimate a coarse marker coordinate model.

The implementation assigns approximate normalized marker coordinates:

```text
upper-left L center   = (20,20)
lower-left L center   = (20,60)
lower-right L center  = (60,60)
```

These three correspondences are used to estimate an affine transformation from marker coordinates to image coordinates.

The marker outer square is modeled using:

```text
LT = (0,0)
RT = (80,0)
RB = (80,80)
LB = (0,80)
```

The affine transformation projects these four model corners into the image and produces a coarse marker quadrilateral.

This coarse result is not automatically accepted as the final detection.

It is verified using the marker's upper-right pattern and the complete normalized marker structure.

---

## 9. Upper-Right Special-Pattern Verification

The upper-right marker region is intentionally different from the other three corners.

Instead of one L-shaped component, it contains several separated bright components.

This region is useful because it reduces orientation ambiguity.

After the three L components define the coarse marker geometry, the expected upper-right area is projected into the image.

The detector checks:

- total bright-pixel ratio;
- number of significant connected components;
- approximate distribution of bright regions.

A candidate is rejected if the predicted upper-right area does not contain a reasonable special-pattern structure.

This step is important for rejecting objects that accidentally contain three L-like bright regions.

---

## 10. Perspective-Normalized Verification

The coarse marker quadrilateral is transformed to a fixed square image using a perspective transformation.

OpenCV `getPerspectiveTransform` and `warpPerspective` are used.

The normalized marker image has a fixed size:

```text
320 x 320
```

After normalization, the detector checks the complete marker structure again.

Expected regions include:

```text
upper-left  -> L-shaped pattern
upper-right -> special multi-part pattern
lower-right -> L-shaped pattern
lower-left  -> L-shaped pattern
```

The normalized view makes verification less sensitive to:

- image position;
- apparent scale;
- moderate perspective distortion.

This second verification stage prevents acceptance based only on several unrelated bright objects appearing close to each other.

---

## 11. Outer Boundary Refinement

The internal bright components are suitable for recognizing the marker, but the required output corners should correspond to the actual outer marker boundary.

Therefore, the coarse quadrilateral is refined using local image gradients.

The image is converted to grayscale and smoothed using Gaussian blur.

Sobel operators are then used to calculate:

```text
horizontal image gradient
vertical image gradient
```

For each predicted marker edge:

```text
top
right
bottom
left
```

multiple sample locations are selected along the predicted edge.

For each sample, the detector searches a narrow region around the predicted position.

The strongest gradient that is consistent with the expected edge normal direction is selected.

The selected points are then fitted with OpenCV `fitLine`.

This produces four refined outer lines.

Their intersections give the final:

```text
LT
RT
RB
LB
```

corner coordinates.

If the refined result is geometrically unreasonable or moves too far away from the coarse prediction, the detector rejects the refinement and falls back to the coarse marker quadrilateral.

This prevents unstable edge measurements from destroying an otherwise valid detection.

---

## 12. Corner Ordering

The final marker corners are maintained in the following order:

```text
LT
RT
RB
LB
```

where:

```text
LT = left-top
RT = right-top
RB = right-bottom
LB = left-bottom
```

This ordering is important for both visualization and pose estimation.

The same corner order is used consistently by the PnP marker model.

---

## 13. Temporal Consistency

The task processes continuous video frames rather than isolated images.

A real marker normally moves continuously between adjacent frames.

The detector therefore checks temporal consistency between the current valid detection and the previous valid detection.

Two main measurements are used.

### 13.1 Area change

The current marker area is compared with the previous marker area.

A very large instantaneous increase or decrease is considered suspicious.

### 13.2 Center displacement

The center displacement between two frames is calculated.

Instead of using only a fixed pixel threshold, the displacement is normalized using the previous marker scale.

This makes the temporal rule more appropriate for markers appearing at different distances from the camera.

If the current detection is valid and temporally consistent, corner smoothing is applied.

The smoothing uses:

```text
60% previous position
40% current position
```

This reduces visible frame-to-frame jitter.

However, the previous result is not used indefinitely.

If the current frame has no reliable marker detection, the program outputs:

```text
NOT DETECTED
```

and resets the previous-frame state.

---

## 14. Detection Development Process

Several approaches were tested during development.

### 14.1 First approach: generic contour and rectangle detection

The first approach was approximately:

```text
grayscale
-> Gaussian blur
-> Canny
-> morphology
-> contour extraction
-> polygon approximation / minAreaRect
```

This method could detect many geometric objects.

However, it was too general.

Objects such as wheels and unrelated background structures could also generate strong contours.

Using `minAreaRect` could force a rectangular box around objects that were not actually the marker.

Therefore:

> rectangular geometry alone was not sufficiently marker-specific.

---

### 14.2 Second approach: grouping bright regions

A later version used the marker's bright internal regions.

Nearby bright components were grouped and an outer quadrilateral was estimated from their combined geometry.

This improved marker recognition.

However, nearby robot support structures could also appear bright.

When these unrelated regions were included in the same group, the estimated outer marker corners moved away from the real marker boundary.

Therefore:

> all nearby bright pixels should not directly define the outer marker boundary.

---

### 14.3 Final approach

The final method separates:

```text
marker recognition
```

from:

```text
outer boundary localization
```

The structured internal marker pattern is used for identity verification.

The actual outer edge is then refined locally using image gradients.

This design provides much better rejection of unrelated objects and more accurate outer corners.

---

## 15. Parameter Design

The implementation attempts to avoid relying only on arbitrary fixed pixel values.

### 15.1 Brightness threshold

The brightness threshold uses Otsu thresholding together with reasonable upper and lower bounds.

This gives some adaptation to changing illumination.

### 15.2 Candidate component area

Component areas are compared with the total image area.

Therefore, the filtering is less dependent on one specific resolution.

### 15.3 L-shape geometry

The L model is based on the relative dimensions shown in the marker drawing instead of one exact pixel template.

### 15.4 Marker geometry

The internal feature locations and outer boundary are represented in a normalized `80 x 80` marker coordinate system.

### 15.5 Outer-edge search region

The outer-edge refinement search range is related to the predicted marker edge length.

This allows larger targets to use a slightly larger search region.

### 15.6 Temporal displacement

Frame-to-frame center displacement is normalized by marker size.

This avoids applying the same absolute pixel-motion limit to both near and distant markers.

---

## 16. Known Limitations

The detector still has several practical limitations.

### Small target size

When the marker occupies very few pixels, the L-shaped components lose their geometric detail.

This can reduce shape-matching accuracy.

### Motion blur

Strong motion blur may merge or break bright marker components.

This can cause contour geometry to become unreliable.

### Strong overexposure

Severe overexposure can enlarge the white regions and change their apparent shape.

### Severe occlusion

If multiple internal marker regions are blocked, the three-L layout or upper-right verification may fail.

### Very weak outer-edge contrast

The black marker boundary may have low contrast against some dark backgrounds.

In this situation, edge refinement may fail.

The program then keeps the coarse geometrically estimated quadrilateral instead of using an unreliable refined result.

---

# Camera Calibration

## 17. Calibration Target

Camera calibration is performed using the provided calibration video:

```text
data/raw/calibration_video.avi
```

The target is a:

```text
7 x 7 symmetric circles grid
```

The center-to-center physical spacing between adjacent circle centers is:

```text
0.03 m
```

The calibration image resolution is:

```text
1440 x 1080
```

OpenCV `findCirclesGrid` is used with the symmetric-grid model.

The object points are defined on the planar calibration board as:

```text
(x * 0.03, y * 0.03, 0)
```

where `x` and `y` are the grid indices.

---

## 18. Calibration Frame Selection

The implementation samples one video frame every:

```text
30 frames
```

A maximum of 30 successful calibration frames is used.

The final calibration used 30 frames:

```text
0
30
60
90
120
150
180
210
240
270
300
330
360
390
420
450
480
510
540
570
600
630
660
690
720
750
780
810
840
870
```

---

## 19. Camera Intrinsic Matrix

The calibrated camera intrinsic matrix is:

```text
[ 2410.2783150179343,    0.0,                 705.9996456514790
     0.0,              2410.5995316416447,    560.6478366522402
     0.0,                 0.0,                   1.0               ]
```

The corresponding intrinsic parameters are approximately:

```text
fx = 2410.2783150179343
fy = 2410.5995316416447
cx = 705.9996456514790
cy = 560.6478366522402
```

where:

- `fx` is the focal length in the image x direction;
- `fy` is the focal length in the image y direction;
- `cx` and `cy` are the principal-point coordinates.

---

## 20. Distortion Coefficients

The calibrated distortion coefficients are:

```text
[-0.06434498006241739,
  0.19342453003720356,
 -0.0005435050609637051,
 -0.0011031832077103248,
  0.8409139831464698]
```

Using OpenCV's standard five-coefficient model:

```text
k1 = -0.06434498006241739
k2 =  0.19342453003720356
p1 = -0.0005435050609637051
p2 = -0.0011031832077103248
k3 =  0.8409139831464698
```

where:

- `k1`, `k2`, `k3` are radial distortion coefficients;
- `p1`, `p2` are tangential distortion coefficients.

The calibration parameters are saved to:

```text
src/xuyangma/calibration.yaml
```

---

## 21. Calibration Accuracy

The final calibration RMS error is:

```text
0.06222580695747921
```

The mean reprojection error is:

```text
0.06089088451195195 px
```

The reprojection error measures the difference between:

```text
detected calibration image points
```

and:

```text
3D calibration points projected back into the image using the estimated camera model
```

The low reprojection error shows that the selected calibration observations are geometrically consistent with the estimated camera parameters.

---

# Marker Pose Estimation

## 22. Marker Physical Model

The marker drawing gives an outer dimension of:

```text
80
```

For the current pose experiment, this dimension is treated as:

```text
80 mm = 0.08 m
```

Therefore:

```text
marker size = 0.08 m
half size   = 0.04 m
```

The marker coordinate-system origin is placed at the center of the marker.

The four 3D marker corners are defined as:

```text
LT = (-0.04, +0.04, 0)
RT = (+0.04, +0.04, 0)
RB = (+0.04, -0.04, 0)
LB = (-0.04, -0.04, 0)
```

All distances are represented in meters.

---

## 23. Coordinate Systems

### Marker coordinate system

The marker coordinate system is defined as:

```text
origin = center of marker
+X = marker right
+Y = marker top
+Z = normal to the marker plane
```

The axes form a right-handed coordinate system.

For a front-facing marker, the positive marker Z direction points approximately outward from the marker surface toward the camera.

### OpenCV camera coordinate system

The OpenCV camera coordinate system is:

```text
+X = camera/image right
+Y = camera/image down
+Z = forward from the camera
```

The translation vector returned by PnP therefore describes the marker origin in camera coordinates.

Because the marker size and calibration spacing are both expressed in meters, the translation vector is also expressed in meters.

---

## 24. PnP Pose Estimation

Pose estimation uses the four detected image corners:

```text
LT
RT
RB
LB
```

and the four corresponding 3D marker-model points.

The implementation first uses:

```text
SOLVEPNP_IPPE_SQUARE
```

This method is designed specifically for planar square targets.

If that method fails, the program falls back to:

```text
SOLVEPNP_ITERATIVE
```

The pose consists of:

```text
rvec
tvec
```

where:

- `rvec` describes marker rotation relative to the camera;
- `tvec` describes marker translation relative to the camera.

---

## 25. Pose Visualization

The estimated marker coordinate axes are projected back into the image using OpenCV `projectPoints`.

The program displays:

```text
X axis
Y axis
Z axis
```

together with:

```text
X translation
Y translation
Z translation
camera-to-marker distance
reprojection error
```

The distance is calculated using the Euclidean norm:

```text
distance = sqrt(X^2 + Y^2 + Z^2)
```

---

## 26. Example Pose Result

One successfully detected frame produced approximately:

```text
X = 0.076 m
Y = -0.047 m
Z = 0.807 m
Distance = 0.811 m
Reprojection error = 0.71 px
```

The values are physically reasonable for a marker located mainly in front of the camera with a small horizontal and vertical offset.

This is an example result from one frame and is not intended to represent a constant error value for the complete video.

An example visualization is provided in:

```text
pose_example.png
```

---

## 27. Result Material

The submission includes a short continuous detection result:

```text
result_demo.avi
```

This video demonstrates marker detection over continuous frames.

The calibration result is stored in:

```text
calibration.yaml
```

A pose-estimation visualization is stored in:

```text
pose_example.png
```

---

## 28. Final Summary

The final system uses only traditional computer vision and OpenCV.

The main recognition strategy is based on:

```text
bright-region extraction
+
marker-specific L-shape geometry
+
three-L spatial layout
+
special upper-right pattern
+
perspective-normalized verification
+
outer-edge refinement
+
temporal consistency
```

Compared with generic contour detection or simple bright-region grouping, the final method makes stronger use of the known marker structure.

The completed system provides:

- continuous marker detection;
- correct `DETECTED` / `NOT DETECTED` state;
- marker outer-region localization;
- ordered `LT`, `RT`, `RB`, `LB` corners;
- temporal stabilization;
- camera calibration;
- optional square-marker PnP pose estimation.