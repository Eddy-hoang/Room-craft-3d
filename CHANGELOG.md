# Changelog — RoomCraft 3D

Tất cả các thay đổi đáng chú ý của dự án được ghi nhận trong file này theo chuẩn [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

## [0.0.1] - 2026-11-27 (Milestone M0: Project Foundation)
### Added
- Khởi tạo Git repository, `.gitignore`, `LICENSE`, `AGENTS.md` và bộ tài liệu nền móng (`docs/`).
- Cấu hình build CMake (`CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json`) sử dụng C++17 và khóa phiên bản dependency (`FreeGLUT v3.8.0`, `GLAD v0.1.36`, `Dear ImGui v1.93.0-docking`).
- Triển khai lớp `roomcraft::core::Application` (`include/roomcraft/core/Application.hpp`, `src/core/Application.cpp`) và `src/main.cpp`:
  - Khởi tạo cửa sổ FreeGLUT với OpenGL 3.3 Core Profile.
  - Nạp OpenGL qua GLAD và in thông tin GPU, OpenGL Version, Profile và GLSL Version.
  - Render tam giác kiểm chứng bằng VAO, VBO và GLSL `#version 330 core` (`assets/shaders/basic.vert`, `assets/shaders/basic.frag`).
  - Tích hợp Dear ImGui sử dụng `imgui_impl_glut` kết hợp `imgui_impl_opengl3`.
  - Xử lý resize viewport, giải phóng tài nguyên GPU/ImGui trước khi hủy OpenGL context và tích hợp kiểm thử tự động `M0_SmokeTest` qua CTest.

### Verified
- Build Debug x64 thành công bằng WinLibs MinGW-w64 UCRT (`GCC 16.2.0`, `CMake 4.4.4`, `Ninja 1.13.2`) không cần DLL MinGW ngoài.
- Chạy thành công `M0_SmokeTest` qua CTest (`1/1 Passed`) và chạy độc lập trên `Intel(R) UHD Graphics` (`OpenGL 3.3.0 Core Profile`, `GLSL 3.30`).
