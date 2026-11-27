# Kết quả kiểm thử thực tế — RoomCraft 3D (Test Results)

Cập nhật lần cuối: **2026-11-27**

## 1. Cấu hình máy kiểm thử

- **OS**: Microsoft Windows 11 Home Single Language 64-bit (`10.0.26200`)
- **GPU**: `Intel(R) UHD Graphics` (Vendor: `Intel`, Driver: `32.0.101.7088`)
- **OpenGL Runtime**: `3.3.0 - Build 32.0.101.7088` (`Core Profile`), GLSL `3.30 - Build 32.0.101.7088`
- **Toolchain**: `x86_64-w64-mingw32-g++ 16.2.0` (WinLibs UCRT), `CMake 4.4.4`, `Ninja 1.13.2`

## 2. Kết quả kiểm thử Milestone M0

| Test ID | Module | Kịch bản | Kết quả mong đợi | Kết quả thực tế | Trạng thái |
|---|---|---|---|---|---|
| **TC-M0-01** | Foundation | `cmake --preset mingw-x64-debug` | Configure không lỗi | `-- Configuring done (7.8s)`, `-- Generating done (0.2s)`, exit code `0` | **PASS** |
| **TC-M0-02** | Foundation | `cmake --build --preset mingw-x64-debug` | Biên dịch thành công `RoomCraft3D.exe` | Build thành công 0 lỗi, 0 cảnh báo trên code project; `ntldd` xác nhận chỉ phụ thuộc 7 DLL chuẩn của Windows System32 | **PASS** |
| **TC-M0-03** | Foundation | Khởi động `RoomCraft3D.exe --smoke-test` (cả qua CTest và chạy độc lập ngoài PATH WinLibs) | Cửa sổ xuất hiện, OpenGL 3.3 Core + thông tin GPU | `FreeGLUT window created (1280x720)`, `OpenGL 3.3.0 (Core Profile)`, `GLSL 3.30`, `Intel(R) UHD Graphics` | **PASS** |
| **TC-M0-04** | Renderer (M0) | Render tam giác bằng VAO, VBO, GLSL `#version 330 core` | Tam giác vẽ đúng, không lỗi GL | Shader compile & link thành công; render 10 khung hình; `glReadPixels` tại tâm viewport trả về `RGB=(140,147,160)` (khớp màu nội suy tam giác, khác màu nền `#181A20`); `OpenGL errors: NONE` | **PASS** |
| **TC-M0-05** | Window | Thay đổi kích thước cửa sổ (`glutReshapeWindow(960, 540)`) | Viewport cập nhật đúng | `[SMOKE] Resize verified: YES (960x540)` | **PASS** |
| **TC-M0-06** | GUI (ImGui) | Tích hợp `imgui_impl_glut` + `imgui_impl_opengl3` (`#version 330 core`) | Khởi tạo và vẽ cửa sổ chẩn đoán ImGui thành công | `Dear ImGui initialized (imgui_impl_glut + imgui_impl_opengl3 #version 330 core)`, render 10 khung hình cùng tam giác không phát sinh lỗi OpenGL | **PASS** |
| **TC-M0-07** | Lifecycle | Đóng chương trình (`glutLeaveMainLoop()` + `glutCloseFunc`) | Giải phóng tài nguyên GPU/ImGui trước khi hủy context, thoát mã `0` | `GPU & Dear ImGui resources released cleanly`, `Exited FreeGLUT main loop cleanly`, `STANDALONE_EXIT=0`, `ctest` báo `1/1 Passed (1.41 sec)` | **PASS** |

## 3. Các bài kiểm thử khác

| Test ID | Module | Kịch bản | Trạng thái | Ghi chú |
|---|---|---|---|---|
| **TC-M0-MSVC** | Build (MSVC + vcpkg) | `cmake --preset msvc-x64-vcpkg-debug` | **NOT RUN** | Máy chưa cài Visual Studio / MSVC Build Tools và `vcpkg` |
| **TC-M0-MANUAL** | Manual Interactive GUI | Người dùng tự mở `RoomCraft3D.exe`, kéo thả cửa sổ ImGui, thử checkbox/color picker và đóng bằng nút X / phím ESC | **NOT VERIFIED** | Đã kiểm chứng tự động qua `--smoke-test`; khuyến nghị người dùng tự mở kiểm tra trực quan |
| **TC-M1..M8** | Các milestone kế tiếp | Xem [`docs/TEST_PLAN.md`](TEST_PLAN.md) | **NOT RUN** | Chưa đến phạm vi triển khai |
