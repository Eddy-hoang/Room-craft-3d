# RoomCraft 3D

**RoomCraft 3D** là ứng dụng desktop thiết kế và bố trí nội thất 3D viết bằng **C++17**, **Modern OpenGL 3.3 Core Profile**, **FreeGLUT**, **GLAD**, **GLM**, **GLSL** và **Dear ImGui**, hướng tới phương án **Hybrid Modeling** (kết hợp Procedural Modeling bằng C++ và Asset-based Modeling từ Blender qua Assimp).

> **Trạng thái hiện tại**: Đã hoàn thành triển khai & kiểm thử tự động **Milestone M2 — Procedural Geometry & Modeling (`v0.2.0`)** (`5/5` CTest suites `PASS`; kiểm thử tương tác GUI thủ công chờ người dùng xác nhận).

## Tính năng hiện có (M0 + M1 + M2)

- **Nền tảng OpenGL 3.3 Core + Core Graphics Engine (M0–M1)**: `Window`, `Shader`, `Vertex`, `Mesh` (VAO/VBO/EBO), `Transform`, `Camera` (Perspective/Orthographic), `Framebuffer` (off-screen `RGBA8 + DEPTH24_STENCIL8`), `Renderer` (Solid/Wireframe + `RenderStats` thực tế).
- **Procedural Geometry Engine thuần CPU (`roomcraft_geometry` — M2)**:
  - Hoàn toàn độc lập với OpenGL/FreeGLUT/ImGui, kiểm thử được độc lập không cần GPU context.
  - **Primitives**: `Plane`, `Cube`, `Cuboid`, `Cylinder`, `Sphere`, `Cone`, `Frustum` (tự tính tọa độ, pháp tuyến chuẩn hóa, UV seam, CCW winding, không dùng `glutSolid*`).
  - **Parametric Geometry**: `BezierCurve` (Bernstein + de Casteljau + đạo hàm), `BezierSurface` (lưới 4×4, đạo hàm riêng, pháp tuyến, control net), `RevolutionSurface` (mặt tròn xoay quanh trục Y từ profile hoặc Bézier Curve).
  - **6 món nội thất phân cấp (`FurnitureGenerator`)**:
    1. `Procedural Desk` (Cuboid + Hierarchical Modeling)
    2. `Procedural Chair` (Cuboid + Cylinder)
    3. `Procedural Bookshelf` (Loop-based Cuboid panels)
    4. `Procedural Desk Lamp` (Cylinder + Sphere + Frustum)
    5. `Procedural Ceramic Vase` (Bézier Curve + Surface of Revolution)
    6. `Procedural Curved Chair` (Cylinder + Cuboid + Bicubic Bézier Surface backrest)
- **Geometry Playground UI (Dear ImGui)**: Chọn danh mục/loại hình học, chỉnh tham số trực tiếp, `Regenerate Mesh`, chuyển Solid/Wireframe, bật/tắt Depth Test, Grid, XYZ Axes, BoundingBox, Control Net/Points, Vertex Normals, chuyển góc nhìn Camera và xem thống kê thời gian sinh/đỉnh/tam giác/draw calls thực tế.

## Build và chạy nhanh trên Windows x64

Xem hướng dẫn chi tiết tại [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md).

```powershell
# Đưa WinLibs MinGW-w64 x64 (UCRT) lên đầu PATH của phiên PowerShell hiện tại
$winlibsBin = (Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*" -Directory | Select-Object -First 1).FullName + "\mingw64\bin"
$env:PATH = "$winlibsBin;$env:PATH"

# Configure & Build Debug
cmake --preset mingw-x64-debug
cmake --build --preset mingw-x64-debug

# Chạy toàn bộ 5 bộ kiểm thử tự động (M0, M1, M2)
ctest --test-dir build\mingw-x64-debug --output-on-failure -V

# Chạy ứng dụng tương tác M2 Geometry Playground (chạy độc lập, không cần PATH WinLibs)
.\build\mingw-x64-debug\RoomCraft3D.exe
```

## Tài liệu dự án

- [`AGENTS.md`](AGENTS.md) — Quy tắc kỹ thuật và chuẩn phát triển dự án
- [`docs/PROJECT_SCOPE.md`](docs/PROJECT_SCOPE.md) — Phạm vi và phân loại ưu tiên P0/P1/P2
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — Kiến trúc phần mềm & sơ đồ lớp/pipeline
- [`docs/GEOMETRY_ALGORITHMS.md`](docs/GEOMETRY_ALGORITHMS.md) — Giải thích thuật toán Procedural & Parametric Geometry (M2)
- [`docs/GRAPHICS_THEORY.md`](docs/GRAPHICS_THEORY.md) — Kiến thức Computer Graphics áp dụng
- [`docs/ROADMAP.md`](docs/ROADMAP.md) — Lộ trình 10 milestone
- [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md) — Hướng dẫn cài môi trường và biên dịch trên Windows
- [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md) & [`docs/TEST_RESULTS.md`](docs/TEST_RESULTS.md) — Kế hoạch và kết quả kiểm thử thực tế
- [`docs/PROGRESS.md`](docs/PROGRESS.md) — Theo dõi tiến độ từng task
- [`docs/KNOWN_ISSUES.md`](docs/KNOWN_ISSUES.md) — Danh sách lỗi và giới hạn hiện tại
- [`docs/ASSET_LICENSES.md`](docs/ASSET_LICENSES.md) — Nguồn gốc và giấy phép thư viện/tài nguyên
