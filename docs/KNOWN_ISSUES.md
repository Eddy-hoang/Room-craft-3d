# Danh sách lỗi và giới hạn đã biết — RoomCraft 3D (Known Issues)

Cập nhật lần cuối: **2026-11-27**

## 1. Giới hạn về môi trường phát triển tại Milestone M0

| ID | Mức độ | Mô tả | Trạng thái / Hướng xử lý |
|---|---|---|---|
| **KI-M0-01** | Trung bình | Máy phát triển hiện chưa cài **Visual Studio 2022 / MSVC Build Tools** và **vcpkg** (và phiên làm việc không có quyền Administrator để cài MSVC tự động). Do đó preset `msvc-x64-vcpkg-debug` đã được cấu hình sẵn nhưng chưa kiểm chứng build thực tế (`NOT RUN`). | Đã cài **WinLibs MinGW-w64 x64 UCRT (`GCC 16.2.0`, `CMake 4.4.4`, `Ninja 1.13.2`)** ở phạm vi user-space và kiểm chứng đầy đủ build/runtime Windows x64. Khi cài xong MSVC + vcpkg có thể chuyển sang preset MSVC ngay. |
| **KI-M0-02** | Thấp | Trong System PATH của máy vẫn tồn tại `C:\MinGW\bin` (MinGW.org 32-bit GCC 6.3.0 cũ từ năm 2016). | Đã khóa compiler trong `CMakePresets.json` (`x86_64-w64-mingw32-gcc`/`g++`) để tránh CMake nhận nhầm GCC 32-bit cũ. Người dùng vẫn cần đảm bảo `mingw64\bin` của WinLibs có trong PATH khi chạy `cmake`/`ninja` (xem `docs/BUILD_GUIDE.md`). |
| **KI-M0-03** | Thấp | Ở chế độ standalone (`ROOMCRAFT_USE_VCPKG=OFF`), lần `cmake --preset` đầu tiên cần kết nối Internet và Python 3 để tải FreeGLUT, GLAD, Dear ImGui và đặc tả `gl.xml` cho GLAD. | Sau lần configure đầu tiên, mã nguồn đã được cache cục bộ trong `build/<preset>/_deps` và `generated/glad`. |

## 2. Giới hạn theo phạm vi Milestone M0

- **KI-M0-04 (Thông tin)**: Ứng dụng mới dừng ở nền tảng M0 (vẽ tam giác kiểm chứng + cửa sổ chẩn đoán Dear ImGui). Chưa triển khai Camera 3D, ma trận MVP, Mesh/Shader abstraction, Procedural Modeling, Assimp, Lighting, Collision hay Save/Load (đúng phạm vi yêu cầu M0).
- **KI-M0-05 (Thông tin)**: Kiểm thử hiện tại xác nhận tự động qua `ctest` / `--smoke-test` (gồm cả `glReadPixels` xác minh pixel tam giác và kiểm thử resize `960x540`), nhưng tương tác kéo chuột trực quan của người dùng trên cửa sổ ImGui vẫn nên được xác nhận thủ công bằng cách chạy `.\build\mingw-x64-debug\RoomCraft3D.exe`.
