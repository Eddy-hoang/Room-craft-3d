# Theo dõi tiến độ dự án — RoomCraft 3D (Progress)

Cập nhật lần cuối: **2026-11-27**

## Tổng quan các Milestone

| Milestone | Mô tả | Trạng thái | Ghi chú |
|---|---|---|---|
| **M0** | Project Foundation | **VERIFIED** | 7/7 test tự động PASS (regression vẫn PASS trong M1); `TC-M0-MANUAL` giữ `NOT VERIFIED` |
| **M1** | Core Graphics Engine | **VERIFIED** | 16/16 test tự động PASS (`3/3 CTest`); `TC-M1-MANUAL` giữ `NOT VERIFIED` chờ người dùng xác nhận |
| **M2** | Procedural Geometry & Modeling | TODO | Chờ xác nhận cổng M1 |
| **M3** | Blender Asset Pipeline | TODO | Chưa bắt đầu |
| **M4** | Scene Management & Room Editor | TODO | Chưa bắt đầu |
| **M5** | Camera, Transformation & Object Picking | TODO | Chưa bắt đầu |
| **M6** | Professional Editor UI/UX | TODO | Chưa bắt đầu |
| **M7** | Lighting, Materials & Visual Quality | TODO | Chưa bắt đầu |
| **M8** | Collision Detection & Project Persistence | TODO | Chưa bắt đầu |
| **M9** | Testing, Optimization & Release | TODO | Chưa bắt đầu |

## Chi tiết công việc Milestone M1 — Core Graphics Engine

| Task ID | Nội dung | Trạng thái | Bằng chứng / Ghi chú |
|---|---|---|---|
| **M1-T01** | Review mã nguồn M0 & chạy regression | DONE | `git status` sạch; `M0_SmokeTest` PASS trước và sau M1 |
| **M1-T02** | Tích hợp GLM | DONE | Khóa `GLM 1.0.1` (`0af55ccecd98d4e5a8d1fad7de25ba429d60e863`) trong `CMakeLists.txt` và bổ sung `glm` vào `vcpkg.json` |
| **M1-T03** | Tách `Window` khỏi `Application` | DONE | `Window.hpp/cpp` quản lý FreeGLUT, OpenGL 3.3 Core Context, GLAD và callback sự kiện |
| **M1-T04** | Xây dựng `Shader` class | DONE | `Shader.hpp/cpp` (RAII, move-only, load file/source, log lỗi, cache uniform) |
| **M1-T05** | Xây dựng `Vertex` struct | DONE | `Vertex.hpp` (`position`, `normal`, `texCoord`, kiểm tra `standard_layout`) |
| **M1-T06** | Xây dựng `Mesh` class | DONE | `Mesh.hpp/cpp` + `MeshValidation.cpp` (VAO/VBO/EBO, `offsetof`/`sizeof`, kiểm tra biên chỉ mục) |
| **M1-T07** | Xây dựng `Transform` | DONE | `Transform.hpp/cpp` (`M = T * Ry * Rx * Rz * S`, Y-up, mét, 10 unit test PASS) |
| **M1-T08** | Xây dựng `Camera` | DONE | `Camera.hpp/cpp` (View/Perspective/Orthographic, validation input lỗi, 7 unit test PASS) |
| **M1-T09** | Xây dựng `Renderer` | DONE | `Renderer.hpp/cpp` (`beginFrame`/`drawMesh`/`drawLines`/`endFrame` + `RenderStats` thực tế) |
| **M1-T10** | Render cube thử nghiệm | DONE | 3 cube khác vị trí/xoay/scale/màu dùng chung 1 Indexed Cube Mesh, đã xác minh pixel 3 màu trong FBO |
| **M1-T11** | Depth Testing | DONE | Chứng minh tự động trên cả 2 thứ tự draw (Xa→Gần & Gần→Xa) và đối chứng khi tắt Depth Test (`TC-M1-11`) |
| **M1-T12** | Framebuffer | DONE | `Framebuffer.hpp/cpp` (`RGBA8` + `DEPTH24_STENCIL8`), completeness + resize + `0x0` safety + ImGui UV flip |
| **M1-T13** | Grid và XYZ Axis | DONE | `grid.vert/frag` + VAO/VBO/EBO `GL_LINES` (không dùng immediate mode) |
| **M1-T14** | Diagnostics UI | DONE | Mở rộng bảng Diagnostics với FPS, frame time, draw calls, vertex/index count, Depth/Grid toggles, Camera & Transform controls |
| **M1-T15** | Unit Test & Smoke Test | DONE | `3/3 CTest` PASS (`M0_SmokeTest`, `M1_UnitTests`, `M1_IntegrationSmokeTest`) |
| **M1-T16** | Documentation | DONE | Cập nhật toàn bộ tài liệu `README.md`, `docs/*`, `CHANGELOG.md` kèm sơ đồ Mermaid |
