# Changelog — RoomCraft 3D

Tất cả các thay đổi đáng chú ý của dự án được ghi nhận trong file này theo chuẩn [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

## [0.1.0] - 2026-11-27 (Milestone M1: Core Graphics Engine)
### Added
- Tích hợp thư viện toán học **GLM 1.0.1** (khóa commit SHA qua CMake FetchContent và bổ sung vào `vcpkg.json`).
- Tách lớp `roomcraft::core::Window` (`Window.hpp/cpp`) quản lý FreeGLUT, OpenGL 3.3 Core Context, GLAD và tập callback sự kiện độc lập khỏi `Application`.
- Triển khai các thành phần Core Graphics Engine theo mô hình RAII (cấm copy, hỗ trợ move):
  - `Shader` (`Shader.hpp/cpp`): đọc/biên dịch/link GLSL `#version 330 core`, báo lỗi driver chi tiết, cache uniform (`int`, `float`, `vec3`, `vec4`, `mat4`).
  - `Vertex` (`Vertex.hpp`): cấu trúc đỉnh `position`, `normal`, `texCoord` chuẩn `standard_layout`.
  - `Mesh` (`Mesh.hpp/cpp`, `MeshValidation.cpp`): VAO/VBO/EBO với Indexed Rendering (`glDrawElements`), bố cục `sizeof`/`offsetof`, kiểm tra biên chỉ mục & dữ liệu NaN/Inf trước khi upload GPU, kèm fixture lập phương đơn vị, lưới XZ và trục XYZ.
  - `Transform` (`Transform.hpp/cpp`): hệ tọa độ tay phải Y-up, đơn vị mét, công thức `M = T * Ry * Rx * Rz * S` và kiểm tra scale hợp lệ.
  - `Camera` (`Camera.hpp/cpp`): View (`glm::lookAt`), Perspective (`glm::perspective`), Orthographic (`glm::ortho`), kiểm tra dữ liệu đầu vào tránh ma trận suy biến và các góc nhìn preset.
  - `Framebuffer` (`Framebuffer.hpp/cpp`): off-screen FBO (`GL_RGBA8` + `GL_DEPTH24_STENCIL8`), kiểm tra completeness, cache resize và xử lý an toàn kích thước `0x0`.
  - `Renderer` (`Renderer.hpp/cpp`): điều phối `beginFrame`, `drawMesh`, `drawLines`, `endFrame`, bật/tắt Depth Test và thống kê `RenderStats` thực tế.
- Bổ sung shader 3D `assets/shaders/mesh.vert`, `mesh.frag` và shader lưới/trục `assets/shaders/grid.vert`, `grid.frag`.
- Mở rộng giao diện chẩn đoán M1 (`M1 Diagnostics` + `3D Viewport (Off-screen Framebuffer)` với UV lật đúng chiều cho OpenGL texture).
- Bổ sung bộ Unit Test thuần CPU (`tests/unit/TransformTests.cpp`, `CameraTests.cpp`, `MeshValidationTests.cpp`) và GPU Integration Smoke Test (`--m1-smoke-test`) tích hợp với CTest.

### Verified
- CMake configure & Debug build thành công (`0` errors, `0` project warnings).
- Toàn bộ `3/3` bộ test CTest đạt `PASS` (`M0_SmokeTest`, `M1_UnitTests` với 23 assertions, `M1_IntegrationSmokeTest` gồm chứng minh Depth Testing độc lập thứ tự draw và xác minh pixel 3 cube trong FBO).
- Kiểm thử tương tác GUI thủ công (`TC-M1-MANUAL`) giữ trạng thái `NOT VERIFIED` chờ người dùng xác nhận.

## [0.0.1] - 2026-11-27 (Milestone M0: Project Foundation)
### Added
- Khởi tạo Git repository, `.gitignore`, `LICENSE`, `AGENTS.md` và bộ tài liệu nền móng (`docs/`).
- Cấu hình build CMake (`CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json`) sử dụng C++17 và khóa phiên bản dependency (`FreeGLUT v3.8.0`, `GLAD v0.1.36`, `Dear ImGui v1.93.0-docking`).
- Triển khai lớp `roomcraft::core::Application` (`Application.hpp/cpp`) và `src/main.cpp` với OpenGL 3.3 Core, GLAD, tam giác VAO/VBO/GLSL và tích hợp Dear ImGui (`imgui_impl_glut` + `imgui_impl_opengl3`).
