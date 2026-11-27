# Danh sách lỗi và giới hạn đã biết — RoomCraft 3D (Known Issues)

Cập nhật lần cuối: **2026-11-27**

## 1. Giới hạn về môi trường & build

| ID | Mức độ | Mô tả | Trạng thái / Hướng xử lý |
|---|---|---|---|
| **KI-M0-01** | Trung bình | Máy phát triển chưa cài **Visual Studio 2022 / MSVC Build Tools** và **vcpkg**; preset `msvc-x64-vcpkg-debug` chưa được kiểm chứng thực tế (`NOT RUN`). | Môi trường chính đã thống nhất sử dụng **WinLibs MinGW-w64 x64 UCRT (`GCC 16.2.0`) + CMake FetchContent khóa commit + Ninja**; giữ `vcpkg.json` (đã thêm `glm`) làm phương án thay thế. |
| **KI-M0-02** | Thấp | Trong System PATH của máy vẫn còn `C:\MinGW\bin` (32-bit GCC 6.3.0 cũ). | Preset `mingw-x64-debug` khóa `x86_64-w64-mingw32-gcc/g++`; cần đưa WinLibs `mingw64\bin` lên đầu PATH khi chạy `cmake`/`ninja`. |
| **KI-M1-01** | Thấp | Khi CMake `4.4.4` cấu hình `GLM 1.0.1` qua FetchContent, CMake phát sinh cảnh báo deprecation từ file `CMakeLists.txt` nội bộ của GLM (`cmake_minimum_required < 3.10`). | Cảnh báo đến từ mã nguồn upstream GLM `1.0.1`, không ảnh hưởng tới build hoặc runtime; mã nguồn project đạt `0` warnings dưới `-Wall -Wextra -Wpedantic`. |

## 2. Giới hạn theo phạm vi Milestone M1

| ID | Mức độ | Mô tả | Hướng xử lý dự kiến |
|---|---|---|---|
| **KI-M1-02** | Thông tin | `TC-M0-MANUAL` và `TC-M1-MANUAL` (kiểm thử tương tác GUI trực quan bằng chuột/bàn phím trên cửa sổ thật) đang ở trạng thái `NOT VERIFIED`. | Người dùng chạy `.\build\mingw-x64-debug\RoomCraft3D.exe` và xác nhận checklist kiểm thử thủ công trước khi chốt M1. |
| **KI-M1-03** | Thông tin | Camera trong M1 chỉ cung cấp các góc nhìn preset (`Default 3D`, `Top Ortho`, `Front Ortho`, `Reset`) và chỉnh `FOV`/`Ortho Size` trong Diagnostics; chưa có tương tác Orbit/Pan/Zoom bằng chuột hay Object Picking. | Thuộc phạm vi **Milestone M5**. |
| **KI-M1-04** | Thông tin | `mesh.frag` mới dùng màu cơ bản `uColor` kết hợp hệ số tương phản hướng mặt `uFaceContrast` để dễ nhìn hình khối 3D; chưa triển khai chiếu sáng Phong/Blinn-Phong hay Texture Mapping. | Thuộc phạm vi **Milestone M7**. |
| **KI-M1-05** | Thông tin | Hình lập phương và lưới trong `Mesh::build*Fixture` chỉ là dữ liệu kiểm thử tối thiểu cho M1, chưa phải hệ thống `PrimitiveGenerator` / Bézier / Surface of Revolution. | Thuộc phạm vi **Milestone M2**. |
