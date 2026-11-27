# Changelog — RoomCraft 3D

Tất cả các thay đổi đáng chú ý của dự án được ghi nhận trong file này theo chuẩn [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

## [0.2.0] - 2026-11-27 (Milestone M2: Procedural Geometry & Modeling)
### Added
- Tạo thư viện tĩnh thuần CPU `roomcraft_geometry` (không phụ thuộc OpenGL/FreeGLUT/ImGui):
  - `math::BoundingBox` (`BoundingBox.hpp/cpp`): tính AABB từ đỉnh, hợp nhất và biến đổi 8 góc bằng ma trận 4×4.
  - `geometry::MeshData` (`MeshData.hpp/cpp`) và `geometry::GeometryUtils` (`GeometryUtils.hpp/cpp`): dữ liệu mesh CPU kèm kiểm tra pháp tuyến đơn vị, UV hữu hạn, chỉ mục hợp lệ và chống tam giác suy biến; sinh overlay BoundingBox và Vertex Normals.
  - `geometry::PrimitiveGenerator` (`PrimitiveGenerator.hpp/cpp`): tự sinh thuật toán cho `Plane`, `Cube`, `Cuboid`, `Cylinder`, `Sphere`, `Cone`, `Frustum` với pivot tâm đáy (`y = 0`), winding CCW, pháp tuyến chuẩn hóa và UV seam.
  - `geometry::BezierCurve` (`BezierCurve.hpp/cpp`): Cubic Bézier (Bernstein + de Casteljau + đạo hàm bậc nhất + control polygon).
  - `geometry::BezierSurface` (`BezierSurface.hpp/cpp`): Bicubic Bézier Surface `4×4` (đạo hàm riêng `dS/du`, `dS/dv`, pháp tuyến bề mặt, control net).
  - `geometry::RevolutionSurface` (`RevolutionSurface.hpp/cpp`): mặt tròn xoay quanh trục `+Y` từ profile 2D hoặc Bézier Curve, xử lý cực `r = 0` và nắp phẳng.
  - `geometry::FurnitureGenerator` (`FurnitureGenerator.hpp/cpp`): mô hình hóa phân cấp (`M_world_part = M_parent * M_local_part`) cho 6 mẫu nội thất: `Desk`, `Chair`, `Bookshelf`, `Desk Lamp`, `Vase` (Revolution) và `Curved Chair` (Bicubic Bézier Surface).
- Nâng cấp `Renderer` hỗ trợ `drawMeshWithMatrix()` và `setWireframeMode()` tự khôi phục `GL_FILL` tại `endFrame()`.
- Nâng cấp `Application` thành **M2 Geometry Playground** (chọn danh mục/loại, chỉnh tham số, auto/manual regenerate, Solid/Wireframe, Grid/Axes/BoundingBox/Control Net/Vertex Normals overlay, thống kê thực tế) và bổ sung chế độ `--m2-smoke-test`.
- Bổ sung bộ Unit Test thuần CPU `RoomCraft3D_GeometryUnitTests` (`M2_GeometryUnitTests`) và `M2_IntegrationSmokeTest` trong CTest.
- Tạo tài liệu học thuật [`docs/GEOMETRY_ALGORITHMS.md`](docs/GEOMETRY_ALGORITHMS.md) giải thích thuật toán kèm sơ đồ Mermaid và mã giả.

### Verified
- Build Debug thành công (`24/24` targets, `0` errors, `0` project warnings).
- Toàn bộ `5/5` CTest suites đạt `PASS` trong `3.24s` (`M0_SmokeTest`, `M1_UnitTests`, `M1_IntegrationSmokeTest`, `M2_GeometryUnitTests`, `M2_IntegrationSmokeTest`).
- Kiểm thử tương tác GUI thủ công (`TC-M2-MANUAL`) giữ trạng thái `NOT VERIFIED` chờ người dùng xác nhận.

## [0.1.0] - 2026-11-27 (Milestone M1: Core Graphics Engine)
### Added
- Tích hợp `GLM 1.0.1`, tách `Window` khỏi `Application`, triển khai `Shader`, `Vertex`, `Mesh`, `Transform`, `Camera`, `Framebuffer`, `Renderer`, demo 3 cube + Grid/XYZ Axes, `M1 Diagnostics`, unit tests và GPU integration smoke test.

## [0.0.1] - 2026-11-27 (Milestone M0: Project Foundation)
### Added
- Khởi tạo cấu trúc dự án, CMake C++17, FreeGLUT `v3.8.0`, GLAD `v0.1.36`, Dear ImGui `v1.93.0-docking`, render tam giác OpenGL 3.3 Core và `M0_SmokeTest`.
