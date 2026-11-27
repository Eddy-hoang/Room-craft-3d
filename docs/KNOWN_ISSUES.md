# Danh sách lỗi và giới hạn đã biết — RoomCraft 3D (Known Issues)

Cập nhật lần cuối: **2026-11-27**

## 1. Giới hạn về môi trường & build

| ID | Mức độ | Mô tả | Trạng thái / Hướng xử lý |
|---|---|---|---|
| **KI-M0-01** | Trung bình | Máy phát triển chưa cài Visual Studio 2022 / MSVC Build Tools và `vcpkg`; preset `msvc-x64-vcpkg-debug` chưa chạy kiểm chứng (`NOT RUN`). | Duy trì môi trường chính đã kiểm chứng: WinLibs MinGW-w64 x64 UCRT (`GCC 16.2.0`) + CMake FetchContent khóa phiên bản + Ninja. `vcpkg.json` đã cập nhật thêm `assimp` và `stb`. |
| **KI-M0-02** | Thấp | Trong System PATH vẫn có `C:\MinGW\bin` (32-bit GCC 6.3.0 cũ). | Preset khóa `x86_64-w64-mingw32-gcc/g++`; cần đưa WinLibs `mingw64\bin` lên đầu PATH khi chạy `cmake`/`ninja`. |
| **KI-M1-01** | Thấp | CMake `4.4.4` báo 2 cảnh báo deprecation từ file `CMakeLists.txt` nội bộ của `GLM 1.0.1` (`cmake_minimum_required < 3.10`). | Không ảnh hưởng build/runtime; mã nguồn dự án đạt `0` warnings dưới `-Wall -Wextra -Wpedantic`. |
| **KI-M3-01** | Thấp | WinLibs CMake `curl` trên Windows không có CA bundle mặc định khi tải archive HTTPS (Assimp/stb). | `CMakeLists.txt` tự động gán `CMAKE_TLS_CAINFO` tới `C:/Program Files/Git/mingw64/etc/ssl/certs/ca-bundle.crt` nếu tồn tại, bảo toàn xác thực chứng chỉ TLS. |
| **KI-M3-02** | Thấp | `Blender 5.2.3 LTS` được giải nén portable tại `D:\blender\Blender-5.2.3\Blender Foundation\Blender 5.2\blender.exe` (không nằm trong System PATH). | Khi cần sinh lại asset từ script `bpy`, gọi trực tiếp đường dẫn `blender.exe` (xem [`docs/BLENDER_GUIDE.md`](BLENDER_GUIDE.md)). Ứng dụng `RoomCraft3D.exe` không cần Blender khi chạy. |

## 2. Giới hạn về kiểm thử thủ công & phạm vi Milestone M2–M3

| ID | Mức độ | Mô tả | Hướng xử lý / Ghi chú |
|---|---|---|---|
| **KI-M2-01** | Trung bình | Các kiểm thử tương tác GUI thủ công `TC-M0-MANUAL`, `TC-M1-MANUAL`, `TC-M2-MANUAL` và `TC-M3-MANUAL` đang ở trạng thái `NOT VERIFIED` (chưa có xác nhận trực quan đầy đủ từ người dùng). | Người dùng chạy `.\build\mingw-x64-debug\RoomCraft3D.exe` và đối chiếu checklist trong báo cáo M3. |
| **KI-M2-02** | Thấp | Bình hoa (`Procedural Ceramic Vase`) và `RevolutionSurface` mô phỏng vỏ ngoài đơn có nắp đáy phẳng (`capBottom = true`), chưa có thành trong có độ dày thực tế. | Chấp nhận cho phạm vi M2; có thể mở rộng profile khép kín hai thành nếu cần. |
| **KI-M2-03** | Thấp | Mặt tựa lưng của `Curved Chair` và `BezierSurface` là bề mặt tham số 1 lớp (single-sided sheet). | Chấp nhận cho phạm vi M2; double-sided shading hoặc độ dày tấm có thể bổ sung ở M7. |
| **KI-M3-03** | Thấp | Shader preview hiện tại (`assets/shaders/mesh.frag`) sử dụng `baseColor` nhân với `uDiffuseMap` kèm chiếu sáng định hướng đơn giản; các thuộc tính vật liệu nâng cao trong `.mtl` (`Ks` specular, `Ns` shininess, `d` opacity) đã được đọc vào `MaterialData` nhưng chưa áp dụng đầy đủ mô hình Phong/Blinn-Phong. | Đúng phạm vi M3; hệ thống Lighting & Materials đầy đủ thuộc **Milestone M7**. |
| **KI-M3-04** | Thấp | Các hạng mục tùy chọn `M3-T18` (Thumbnail Preview PNG pre-rendered — P1) và `M3-T19` (glTF 2.0 / `.glb` importer — P2) chưa triển khai ở M3. | Asset Library hiện hiển thị thẻ/danh sách kèm nhãn nguồn (`Procedural` / `Blender`) và ô tìm kiếm; Assimp chỉ bật OBJ importer để rút ngắn thời gian build và giảm kích thước binary. |
| **KI-M3-05** | Thông tin | Chưa triển khai Scene & Room Editor (M4), Orbit/Pan/Zoom chuột & Object Picking (M5), Editor UI/UX docking hoàn chỉnh (M6), Phong Lighting & Shadow (M7), Collision & Project Persistence (M8). | Thuộc phạm vi các milestone kế tiếp theo đúng [`docs/ROADMAP.md`](ROADMAP.md). |
