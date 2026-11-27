# Danh sách lỗi và giới hạn đã biết — RoomCraft 3D (Known Issues)

Cập nhật lần cuối: **2026-11-28**

## 1. Giới hạn về môi trường & build

| ID | Mức độ | Mô tả | Trạng thái / Hướng xử lý |
|---|---|---|---|
| **KI-M0-01** | Trung bình | Máy phát triển chưa cài Visual Studio 2022 / MSVC Build Tools và `vcpkg`; preset `msvc-x64-vcpkg-debug` chưa chạy kiểm chứng (`NOT RUN`). | Duy trì môi trường chính: WinLibs MinGW-w64 x64 UCRT (`GCC 16.2.0`) + CMake FetchContent khóa commit & `URL_HASH SHA256` + Ninja. |
| **KI-M0-02** | Thấp | Trong System PATH vẫn có `C:\MinGW\bin` (32-bit GCC 6.3.0 cũ). | Preset khóa `x86_64-w64-mingw32-gcc/g++`; cần đưa WinLibs `mingw64\bin` lên đầu PATH khi chạy `cmake`/`ninja`. |
| **KI-M1-01** | Thấp | CMake `4.4.4` báo 2 cảnh báo deprecation từ file `CMakeLists.txt` nội bộ của `GLM 1.0.1` (`cmake_minimum_required < 3.10`). | Không ảnh hưởng build/runtime; mã nguồn dự án đạt `0` warnings dưới `-Wall -Wextra -Wpedantic`. |
| **KI-M3-01** | Thấp | WinLibs CMake `curl` trên Windows không có CA bundle mặc định khi tải archive HTTPS. | `CMakeLists.txt` tự gán `CMAKE_TLS_CAINFO` tới CA bundle của Git for Windows và xác thực `URL_HASH SHA256`. |
| **KI-M3-02** | Thấp | `Blender 5.2.3 LTS` giải nén portable ngoài System PATH. | Chỉ cần khi sinh lại asset bằng `bpy`; `RoomCraft3D.exe` chạy độc lập không cần Blender. |

## 2. Giới hạn về kiểm thử thủ công & phạm vi Milestone M2–M5

| ID | Mức độ | Mô tả | Hướng xử lý / Ghi chú |
|---|---|---|---|
| **KI-M2-01** | Trung bình | Các kiểm thử tương tác GUI thủ công `TC-M0-MANUAL`..`TC-M5-MANUAL` đang ở trạng thái `NOT VERIFIED`. | Người dùng chạy `.\build\mingw-x64-debug\RoomCraft3D.exe` và xác nhận checklist trong báo cáo M5. |
| **KI-M3-03** | Thấp | Shader hiện tại mới dùng `baseColor` + `uDiffuseMap` với chiếu sáng định hướng đơn giản. | Hệ thống Blinn-Phong Lighting & Materials thuộc **Milestone M7**. |
| **KI-M4-01** | Thấp | Benchmark 10/30/100 vật thể đo trên bản `Debug` off-screen FBO + `glFinish()` (20–30 frame), chưa đo bản `Release` và thời gian dài end-to-end. | Sẽ mở rộng benchmark dài hơi hơn và đo Release ở **Milestone M9**. |
| **KI-M4-02** | Thấp | `Scene::isBoundsWithinRoom()` mới kiểm tra sơ bộ biên phòng; khi kéo đồ nội thất ở M5 chưa có hệ thống ngăn chồng lấn giữa các đồ nội thất. | Hệ thống Collision Detection hoàn chỉnh thuộc **Milestone M8**. |
| **KI-M5-01** | Thấp | Hạng mục tùy chọn `M5-T22` (**Basic 3-Axis Transform Gizmo** — P1) chưa triển khai trong M5 (`TODO`); hiện người dùng thao tác trực tiếp bằng `Move (G)`, `Rotate (R)`, `Scale (S)` kết hợp khóa trục `X / Y / Z`. | Có thể bổ sung 3-Axis Gizmo ở giai đoạn hoàn thiện (M6/M9) mà không cần thay đổi kiến trúc `TransformController`. |
| **KI-M5-02** | Thấp | `M5_IntegrationSmokeTest` kiểm thử thuật toán Ray Casting / Picking / Drag / Cancel trên ngữ cảnh OpenGL thật nhưng mô phỏng tọa độ chuột chiếu từ 3D thay vì gửi sự kiện chuột vật lý của hệ điều hành qua FreeGLUT/ImGui. | Ghi nhận trung thực; thao tác chuột/phím thực tế cần được người dùng xác nhận qua `TC-M5-MANUAL`. |
| **KI-M5-03** | Thông tin | Chưa triển khai Design System & Thumbnail UI (M6), Blinn-Phong Lighting & Shadows (M7), Collision Detection & JSON Save/Load (M8), Undo/Redo History đầy đủ. | Thuộc phạm vi các milestone kế tiếp theo [`docs/ROADMAP.md`](ROADMAP.md). |
