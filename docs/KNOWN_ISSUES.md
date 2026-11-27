# Danh sách lỗi và giới hạn đã biết — RoomCraft 3D (Known Issues)

Cập nhật lần cuối: **2026-11-28**

## 1. Giới hạn về môi trường & build

| ID | Mức độ | Mô tả | Trạng thái / Hướng xử lý |
|---|---|---|---|
| **KI-M0-01** | Trung bình | Máy phát triển chưa cài Visual Studio 2022 / MSVC Build Tools và `vcpkg`; preset `msvc-x64-vcpkg-debug` chưa chạy kiểm chứng (`NOT RUN`). | Duy trì môi trường chính đã kiểm chứng: WinLibs MinGW-w64 x64 UCRT (`GCC 16.2.0`) + CMake FetchContent khóa commit & `URL_HASH SHA256` + Ninja. |
| **KI-M0-02** | Thấp | Trong System PATH vẫn có `C:\MinGW\bin` (32-bit GCC 6.3.0 cũ). | Preset khóa `x86_64-w64-mingw32-gcc/g++`; cần đưa WinLibs `mingw64\bin` lên đầu PATH khi chạy `cmake`/`ninja`. |
| **KI-M1-01** | Thấp | CMake `4.4.4` báo 2 cảnh báo deprecation từ file `CMakeLists.txt` nội bộ của `GLM 1.0.1` (`cmake_minimum_required < 3.10`). | Không ảnh hưởng build/runtime; mã nguồn dự án đạt `0` warnings dưới `-Wall -Wextra -Wpedantic`. |
| **KI-M3-01** | Thấp | WinLibs CMake `curl` trên Windows không có CA bundle mặc định khi tải archive HTTPS (Assimp/stb). | `CMakeLists.txt` tự động gán `CMAKE_TLS_CAINFO` tới `C:/Program Files/Git/mingw64/etc/ssl/certs/ca-bundle.crt` nếu tồn tại và xác thực `URL_HASH SHA256`. |
| **KI-M3-02** | Thấp | `Blender 5.2.3 LTS` được giải nén portable tại `D:\blender\Blender-5.2.3\Blender Foundation\Blender 5.2\blender.exe` (không nằm trong System PATH). | Chỉ cần gọi trực tiếp khi tái sinh asset từ script `bpy`; `RoomCraft3D.exe` chạy độc lập không cần Blender. |

## 2. Giới hạn về kiểm thử thủ công & phạm vi Milestone M2–M4

| ID | Mức độ | Mô tả | Hướng xử lý / Ghi chú |
|---|---|---|---|
| **KI-M2-01** | Trung bình | Các kiểm thử tương tác GUI thủ công `TC-M0-MANUAL`, `TC-M1-MANUAL`, `TC-M2-MANUAL`, `TC-M3-MANUAL` và `TC-M4-MANUAL` đang ở trạng thái `NOT VERIFIED`. | Người dùng chạy `.\build\mingw-x64-debug\RoomCraft3D.exe` và xác nhận checklist trong báo cáo M4. |
| **KI-M3-03** | Thấp | Shader preview hiện tại (`mesh.frag`) mới dùng `baseColor` + `uDiffuseMap` với chiếu sáng định hướng đơn giản; `Ks`, `Ns`, `d` chưa áp dụng đầy đủ mô hình Phong/Blinn-Phong. | Đúng phạm vi; hệ thống Lighting & Materials đầy đủ thuộc **Milestone M7**. |
| **KI-M3-04** | Thấp | `M3-T18` (Thumbnail PNG pre-rendered — P1) và `M3-T19` (glTF 2.0 / `.glb` — P2) chưa triển khai. | Thumbnail sẽ bổ sung ở **Milestone M6**. |
| **KI-M4-01** | Thấp | Benchmark 10/30/100 vật thể được đo trên bản build **`Debug`** bằng off-screen FBO + `glFinish()` (chưa đo bản `Release` và chưa tính thời gian swap buffers có VSync của màn hình). | Đã ghi rõ `Config=Debug` và `VSync=N/A (Off-screen FBO + glFinish)` trong báo cáo và `TEST_RESULTS.md`. |
| **KI-M4-02** | Thấp | `Scene::isBoundsWithinRoom()` và `PlacementSystem` chỉ thực hiện kiểm tra biên phòng sơ bộ và lệch điểm đặt mặc định, **chưa** kiểm tra va chạm giữa các món nội thất. | Hệ thống Collision Detection hoàn chỉnh (AABB–AABB + Room Bounds) thuộc **Milestone M8**. |
| **KI-M4-03** | Thông tin | Chưa triển khai Camera Orbit/Pan/Zoom chuột, Ray Casting Object Picking & Transform Gizmo (M5), Docking Layout & Undo/Redo chuyên nghiệp (M6), Phong Lighting & Shadows (M7), Collision & Save/Load JSON (M8). | Thuộc phạm vi các milestone kế tiếp theo [`docs/ROADMAP.md`](ROADMAP.md). |
