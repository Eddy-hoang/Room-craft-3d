# RoomCraft 3D

**RoomCraft 3D** là ứng dụng desktop thiết kế và bố trí nội thất 3D viết bằng **C++17**, **Modern OpenGL 3.3 Core Profile**, **FreeGLUT**, **GLAD**, **GLM**, **GLSL** và **Dear ImGui**, hướng tới phương án **Hybrid Modeling** (kết hợp Procedural Modeling bằng C++ và Asset-based Modeling từ Blender qua Assimp).

> **Trạng thái hiện tại**: Đã hoàn thành triển khai & kiểm thử tự động **Milestone M1 — Core Graphics Engine (`v0.1.0`)** (kiểm thử tương tác GUI thủ công đang chờ người dùng xác nhận).

## Tính năng hiện có (Milestone M0 + M1)

- **Nền tảng OpenGL 3.3 Core + FreeGLUT + GLAD + Dear ImGui** (`imgui_impl_glut` + `imgui_impl_opengl3`).
- **Core Graphics Engine hướng đối tượng (RAII)**:
  - `Window`: bao gói FreeGLUT, khởi tạo OpenGL 3.3 Core Context, GLAD và truy vấn thông số GPU.
  - `Shader`: đọc/biên dịch/link GLSL `#version 330 core`, báo lỗi chi tiết, cache uniform (`int`, `float`, `vec3`, `vec4`, `mat4`).
  - `Vertex` & `Mesh`: VAO/VBO/EBO với Indexed Rendering (`glDrawElements`), bố cục `sizeof`/`offsetof` và kiểm tra tính hợp lệ dữ liệu hình học trước khi upload GPU.
  - `Transform`: hệ tọa độ tay phải, Y-up, đơn vị mét, ma trận `M = T * Ry * Rx * Rz * S`.
  - `Camera`: quản lý độc lập ma trận View (`glm::lookAt`) và Projection (`glm::perspective` / `glm::ortho`), kiểm tra dữ liệu đầu vào tránh ma trận suy biến.
  - `Framebuffer`: render off-screen vào texture màu `GL_RGBA8` + `GL_DEPTH24_STENCIL8`, tự kiểm tra completeness và xử lý resize/kích thước 0 an toàn.
  - `Renderer`: điều phối `beginFrame`, `drawMesh`, `drawLines`, `endFrame`, bật/tắt Depth Testing và thống kê draw call/vertex/index thực tế.
- **Demo Scene M1 & Bảng chẩn đoán**:
  - Render 3 khối lập phương 3D (có cặp khối chồng hình chiếu kiểm chứng Depth Testing), lưới căn chỉnh XZ và hệ trục tọa độ XYZ (`GL_LINES`) qua off-screen `Framebuffer` hiển thị trong cửa sổ ImGui Viewport.
  - Bảng `M1 Diagnostics`: thông số GPU/OpenGL/GLSL, FPS, Frame time, Draw calls, Vertex/Index count, chuyển đổi Perspective/Orthographic, Camera preset, bật/tắt Depth Test, Grid/Axis và chỉnh Transform trực tiếp cho từng cube.
- **Kiểm thử tự động (CTest)**: `M0_SmokeTest` (regression), `M1_UnitTests` (23 assertions thuần CPU), `M1_IntegrationSmokeTest` (GPU integration + chứng minh Depth Testing độc lập thứ tự draw).

## Build và chạy nhanh trên Windows x64

Xem hướng dẫn chi tiết tại [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md).

```powershell
# Đưa WinLibs MinGW-w64 x64 (UCRT) lên đầu PATH của phiên PowerShell hiện tại
$winlibsBin = (Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*" -Directory | Select-Object -First 1).FullName + "\mingw64\bin"
$env:PATH = "$winlibsBin;$env:PATH"

# Configure & Build Debug
cmake --preset mingw-x64-debug
cmake --build --preset mingw-x64-debug

# Chạy toàn bộ kiểm thử tự động (3/3 CTest suites)
ctest --test-dir build\mingw-x64-debug --output-on-failure -V

# Chạy ứng dụng tương tác M1 (chạy độc lập, không cần PATH WinLibs)
.\build\mingw-x64-debug\RoomCraft3D.exe
```

## Cấu trúc dự án hiện tại (Milestone M1)

```text
.
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
├── AGENTS.md
├── CHANGELOG.md
├── LICENSE
├── README.md
├── assets/shaders/
│   ├── basic.vert / basic.frag    # M0 regression shader
│   ├── mesh.vert  / mesh.frag     # M1 3D MVP shader
│   └── grid.vert  / grid.frag     # M1 Grid & XYZ axis shader
├── include/roomcraft/
│   ├── core/      (Application.hpp, Window.hpp)
│   ├── graphics/  (Camera.hpp, Framebuffer.hpp, Mesh.hpp, Renderer.hpp, Shader.hpp, Vertex.hpp)
│   └── scene/     (Transform.hpp)
├── src/
│   ├── main.cpp
│   ├── core/      (Application.cpp, Window.cpp)
│   ├── graphics/  (Camera.cpp, Framebuffer.cpp, Mesh.cpp, MeshValidation.cpp, Renderer.cpp, Shader.cpp)
│   └── scene/     (Transform.cpp)
├── tests/unit/
│   ├── TestMain.cpp
│   ├── TransformTests.cpp
│   ├── CameraTests.cpp
│   └── MeshValidationTests.cpp
└── docs/
```

## Tài liệu dự án

- [`AGENTS.md`](AGENTS.md) — Quy tắc kỹ thuật và chuẩn phát triển dự án
- [`docs/PROJECT_SCOPE.md`](docs/PROJECT_SCOPE.md) — Phạm vi và phân loại ưu tiên P0/P1/P2
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — Kiến trúc phần mềm & sơ đồ lớp/pipeline
- [`docs/ROADMAP.md`](docs/ROADMAP.md) — Lộ trình 10 milestone
- [`docs/GRAPHICS_THEORY.md`](docs/GRAPHICS_THEORY.md) — Kiến thức Computer Graphics áp dụng
- [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md) — Hướng dẫn cài môi trường và biên dịch trên Windows
- [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md) & [`docs/TEST_RESULTS.md`](docs/TEST_RESULTS.md) — Kế hoạch và kết quả kiểm thử thực tế
- [`docs/PROGRESS.md`](docs/PROGRESS.md) — Theo dõi tiến độ từng task
- [`docs/KNOWN_ISSUES.md`](docs/KNOWN_ISSUES.md) — Danh sách lỗi và giới hạn hiện tại
- [`docs/ASSET_LICENSES.md`](docs/ASSET_LICENSES.md) — Nguồn gốc và giấy phép thư viện/tài nguyên
