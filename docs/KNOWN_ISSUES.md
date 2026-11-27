# Danh sách lỗi và giới hạn đã biết — RoomCraft 3D (Known Issues)

Cập nhật lần cuối: **2026-11-27**

## 1. Giới hạn về môi trường & build

| ID | Mức độ | Mô tả | Trạng thái / Hướng xử lý |
|---|---|---|---|
| **KI-M0-01** | Trung bình | Máy phát triển chưa cài Visual Studio 2022 / MSVC Build Tools và `vcpkg`; preset `msvc-x64-vcpkg-debug` chưa chạy kiểm chứng (`NOT RUN`). | Duy trì môi trường chính đã kiểm chứng: WinLibs MinGW-w64 x64 UCRT (`GCC 16.2.0`) + CMake FetchContent khóa commit + Ninja. |
| **KI-M0-02** | Thấp | Trong System PATH vẫn có `C:\MinGW\bin` (32-bit GCC 6.3.0 cũ). | Preset khóa `x86_64-w64-mingw32-gcc/g++`; cần đưa WinLibs `mingw64\bin` lên đầu PATH khi chạy `cmake`/`ninja`. |
| **KI-M1-01** | Thấp | CMake `4.4.4` báo 2 cảnh báo deprecation từ file `CMakeLists.txt` nội bộ của `GLM 1.0.1` (`cmake_minimum_required < 3.10`). | Không ảnh hưởng build/runtime; mã nguồn dự án đạt `0` warnings dưới `-Wall -Wextra -Wpedantic`. |

## 2. Giới hạn về kiểm thử thủ công & phạm vi Milestone M2

| ID | Mức độ | Mô tả | Hướng xử lý / Ghi chú |
|---|---|---|---|
| **KI-M2-01** | Trung bình | Kiểm thử tương tác GUI thủ công `TC-M0-MANUAL`, `TC-M1-MANUAL` và `TC-M2-MANUAL` đang ở trạng thái `NOT VERIFIED` (chưa nhận xác nhận trực quan từ người dùng). | Người dùng chạy `.\build\mingw-x64-debug\RoomCraft3D.exe` và đối chiếu checklist trong báo cáo M2. |
| **KI-M2-02** | Thấp | Bình hoa (`Procedural Ceramic Vase`) và `RevolutionSurface` hiện mô phỏng vỏ bề mặt ngoài đơn (exterior shell) có nắp đáy phẳng (`capBottom = true`), chưa có thành trong có độ dày thực tế (inner wall / solid shell thickness). | Ghi nhận rõ giới hạn hình học; có thể mở rộng profile khép kín hai thành nếu cần ở giai đoạn hoàn thiện. |
| **KI-M2-03** | Thấp | Mặt tựa lưng của `Curved Chair` và `BezierSurface` là bề mặt tham số mỏng 1 lớp (single-sided sheet); khi chưa bật backface culling hai mặt vẫn hiển thị được nhưng mặt sau có pháp tuyến ngược chiều nhìn. | Chấp nhận cho phạm vi học thuật M2; độ dày tấm tựa lưng hoặc double-sided shading có thể bổ sung ở M7. |
| **KI-M2-04** | Thông tin | Chưa triển khai Blender/Assimp import (M3), Room/Scene Management hoàn chỉnh (M4), Orbit/Pan/Zoom chuột & Object Picking (M5), Phong Lighting & Texture Mapping (M7), Collision & Save/Load (M8). | Thuộc phạm vi các milestone kế tiếp theo đúng Roadmap. |
