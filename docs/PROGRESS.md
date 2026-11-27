# Theo dõi tiến độ dự án — RoomCraft 3D (Progress)

Cập nhật lần cuối: **2026-11-27**

## Tổng quan các Milestone

| Milestone | Mô tả | Trạng thái | Ghi chú |
|---|---|---|---|
| **M0** | Project Foundation | **DONE** | Đã configure, build, chạy CTest & smoke test thực tế trên Intel UHD Graphics (OpenGL 3.3 Core) |
| **M1** | Core Graphics Engine | TODO | Chờ xác nhận cổng M0 |
| **M2** | Procedural Geometry & Modeling | TODO | Chưa bắt đầu |
| **M3** | Blender Asset Pipeline | TODO | Chưa bắt đầu |
| **M4** | Scene Management & Room Editor | TODO | Chưa bắt đầu |
| **M5** | Camera, Transformation & Object Picking | TODO | Chưa bắt đầu |
| **M6** | Professional Editor UI/UX | TODO | Chưa bắt đầu |
| **M7** | Lighting, Materials & Visual Quality | TODO | Chưa bắt đầu |
| **M8** | Collision Detection & Project Persistence | TODO | Chưa bắt đầu |
| **M9** | Testing, Optimization & Release | TODO | Chưa bắt đầu |

## Chi tiết công việc Milestone M0 — Project Foundation

| Task ID | Công việc | Trạng thái | Bằng chứng / Ghi chú |
|---|---|---|---|
| **M0-T01** | Kiểm tra compiler C++ | DONE | Phát hiện ban đầu chỉ có MinGW 32-bit GCC 6.3.0 (không đạt x64/C++17) và chưa có MSVC; đã cài WinLibs MinGW-w64 x64 UCRT (`GCC 16.2.0`) |
| **M0-T02** | Thiết lập CMake và quản lý dependency | DONE | Tạo `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json` khóa baseline/commit SHA; configure thành công bằng CMake `4.4.4` + Ninja `1.13.2` |
| **M0-T03** | Khởi tạo Git repository | DONE | Đã chạy `git init` tại `d:\CG`, tạo `.gitignore`, `LICENSE`, `AGENTS.md` |
| **M0-T04** | Tích hợp FreeGLUT | DONE | FreeGLUT `v3.8.0` biên dịch tĩnh thành công, mở cửa sổ `1280x720` |
| **M0-T05** | Khởi tạo OpenGL 3.3 Core Context | DONE | Xác nhận runtime `OpenGL 3.3.0` (`Core Profile`), `GLSL 3.30` trên `Intel(R) UHD Graphics` |
| **M0-T06** | Tích hợp GLAD | DONE | Sinh GLAD `v0.1.36` cho `gl=3.3` `core` và nạp thành công qua `gladLoadGL()` |
| **M0-T07** | Render tam giác đầu tiên | DONE | Shader `basic.vert`/`basic.frag` compile & link thành công; xác minh pixel tâm tam giác `RGB=(140,147,160)` bằng `glReadPixels`, không lỗi OpenGL |
| **M0-T08** | Kiểm tra resize và đóng cửa sổ | DONE | Xác nhận callback reshape về `960x540`, `glutCloseFunc` giải phóng VAO/VBO/Shader & ImGui trước khi thoát với exit code `0` |
| **M0-T09** | Kiểm tra tích hợp Dear ImGui | DONE | `imgui_impl_glut` + `imgui_impl_opengl3` (`#version 330 core`) khởi tạo và render khung hình không lỗi trên OpenGL 3.3 Core |
| **M0-T10** | Tạo tài liệu project và build guide | DONE | Hoàn thiện `README.md`, `AGENTS.md`, `CHANGELOG.md` và toàn bộ 10 tài liệu trong `docs/` |
