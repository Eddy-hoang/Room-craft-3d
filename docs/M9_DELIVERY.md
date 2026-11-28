# Báo cáo Bàn giao RoomCraft 3D — Milestone M9 (`v1.0.0-rc1` · Release Candidate)

- **Phiên bản ứng viên phát hành (Release Candidate)**: **`v1.0.0-rc1`** *(tách biệt với bản phát hành chính thức `v1.0.0` cho tới khi người dùng duyệt toàn bộ cổng nghiệm thu thủ công)*
- **Trạng thái Milestone M9**: **`RELEASE CANDIDATE`** *(Kiểm thử tự động CPU + GPU đạt **`19/19` CTest PASS** trên cả **Debug (`4.82 s`)** và **Release (`3.56 s`)**, bản đóng gói portable đã kiểm chứng ngoài PATH WinLibs và trên đường dẫn Unicode/khoảng trắng; chờ nghiệm thu tương tác cuối cùng & phê duyệt `v1.0.0` từ người dùng).*

---

## 1. Tóm tắt Kiểm toán & Sửa lỗi (M9.0 – M9.1)

Toàn bộ các rủi ro kỹ thuật được nêu trong Technical Review M8 đều đã được kiểm toán trực tiếp trên mã nguồn, vá triệt để và phủ bằng bộ kiểm thử tự động `M9_ReliabilityUnitTests` (16 bài kiểm thử, bao gồm **1.500 trường hợp fuzzing JSON**):

1. **JSON Parser nội bộ (`src/persistence/Json.cpp`)**:
   - Bổ sung kiểm tra nghiêm ngặt chuỗi byte **UTF-8** (từ chối lead/continuation sai, overlong encoding, raw UTF-8 surrogate `U+D800..U+DFFF`, codepoint `> U+10FFFF`).
   - Từ chối **lone low surrogate** (`\uDC00..\uDFFF`), **khóa trùng lặp** trong cùng JSON object (`{"a":1,"a":2}`), và giới hạn **tổng số node (`250,000`)** chống tấn công phình bộ nhớ.
   - Chuyển phân tích số thực sang `std::from_chars(..., double)` và `std::locale::classic()` khi xuất JSON để **100% độc lập locale**.
   - Sửa lỗi không gán `outError` khi tài liệu JSON rỗng/chỉ có khoảng trắng.
2. **Dirty-State & Undo/Redo (`ProjectManager.cpp`, `RoomEditor.cpp`)**:
   - Áp dụng chữ ký trạng thái chuẩn hóa (`captureSavedBaseline` / `syncDirtyWithState`) so khớp mọi trường dữ liệu được lưu (phòng, nội thất, transform, visibility/lock, `shininessScale`, ánh sáng, MSAA, render flags, snap, collision).
   - Undo về đúng bản đã lưu tự trả `dirty = false`; Redo vượt mốc lưu trả `dirty = true`; thay đổi ngoài `CommandHistory` (ví dụ ánh sáng) luôn giữ `dirty = true` chính xác.
3. **Collision System Audit (`CollisionSystem.cpp`, `RoomEditor.cpp`)**:
   - Thêm quy tắc **chống kẹt (non-trapping)** `CollisionSystem::isPreventInvalidTransitionAllowed()`: nếu vật thể đã vi phạm sẵn (sau khi thu nhỏ phòng hoặc đổi từ `Warning Only` sang `Prevent Invalid`), người dùng vẫn kéo được vật về hướng giảm vi phạm/hợp lệ thay vì bị khóa cứng.
   - Siết chặt `isFloorCovering()` chỉ miễn trừ asset thảm trải sàn (`blender-carpet`/rug), không miễn trừ nhầm nội thất thông thường bị thu nhỏ dẹt.
4. **Sửa lỗi thực tế từ ảnh người dùng & Đường dẫn Unicode Portable**:
   - Sửa vỡ chữ tiêu đề cửa sổ Windows **`RoomCraft 3D â€” Modern Bedroom`** → dùng Win32 `SetWindowTextW` hiển thị chuẩn dấu `—` và tên tệp tiếng Việt.
   - Sửa tìm thư mục thực thi (`GetModuleFileNameW`), nạp Font TTF (`AddFontFromMemoryTTF`), nạp Texture PNG (`stbi_load_from_memory`) và nạp Model Assimp (`u8string` / `u8path`) để bản portable chạy mượt trong đường dẫn **Unicode + khoảng trắng** ngoài `D:\CG`.
   - Tắt tự sinh `imgui.ini` (`io.IniFilename = nullptr`).

---

## 2. Đánh bóng Sản phẩm & Tính năng Đồ họa M9 (M9.3 – M9.4)

- **UI Layout Persistence (`M8-F18` / `M9-F14`)**: Triển khai [`PreferencesManager`](../include/roomcraft/persistence/PreferencesManager.hpp) lưu riêng tại `%APPDATA%\RoomCraft3D\preferences.json` (độ rộng panel trái/phải, tỷ lệ Hierarchy, UI Scale, chế độ Grid/List, tối đa 8 `Recent Projects`), kẹp giá trị hợp lệ và tự fallback an toàn khi tệp thiếu/hỏng.
- **Biểu tượng & Tài nguyên Windows (`M9-F16`)**: Nhúng biểu tượng đa độ phân giải `assets/icons/roomcraft.ico` (16/32/48/256 px) và `VS_VERSION_INFO` (`1.0.0-rc1`) vào `RoomCraft3D.exe` qua [`resources/roomcraft.rc`](../resources/roomcraft.rc).
- **Dự án mẫu (`M9-F15`)**: Xuất và xác thực 3 tệp mẫu thật tại `examples/`: `ModernBedroom.roomcraft`, `GamingWorkspace.roomcraft`, `MinimalistLivingRoom.roomcraft`.
- **Directional Shadow Mapping (`M9-F17`, P2 Đã đánh giá & Triển khai)**:
  - Depth-only FBO (`ShadowMap`, OpenGL 3.3 Core) độ phân giải chọn được `512 / 1024 / 2048`, chiếu trực giao bao khít phòng, **slope-scaled bias** + lọc mềm **3×3 PCF**, kèm vân ghép ván gỗ tinh tế và Corner AO nhẹ.
  - Chi phí đo thực tế trên **Intel(R) UHD Graphics** (`1280×720`, Release, 30 nội thất): chỉ **`+0.61 ms`** (`2.12 ms → 2.73 ms`, giữ **`366.5 FPS`**, tương thích 100% với **MSAA 4x** ở **`292.2 FPS`**) và không ảnh hưởng `M7_LightingGpu`.
  - **Rotate/Scale Gizmo (`M9-F18`)**: Giữ **`DEFERRED`** vì thao tác xoay/scale trực tiếp (`R`/`S` + trục `X/Y/Z` + Inspector) đã ổn định và không làm chậm tiến độ phát hành.

---

## 3. Tổng hợp Kết quả Build, Test, Benchmark & Đóng gói

| Hạng mục | Kết quả Thực tế | Tài liệu Chi tiết |
|---|---|---|
| **Debug Build (`mingw-x64-debug`)** | `0` lỗi, `0` cảnh báo dự án, `47,989,311` bytes, **`19/19` CTest PASS (`4.82 s`)** | [`FINAL_QA_REPORT.md`](FINAL_QA_REPORT.md) |
| **Release Build (`mingw-x64-release`)** | `0` lỗi, `0` cảnh báo dự án, `7,960,281` bytes (`7.59 MB`), **`19/19` CTest PASS (`3.56 s`)** | [`FINAL_QA_REPORT.md`](FINAL_QA_REPORT.md) |
| **Benchmark 30 obj (Release, 1280×720, Shadows 1024)** | `2.73 ms` avg / `3.56 ms` p95 (**`366.5 FPS`**); MSAA 4x: `3.42 ms` avg (**`292.2 FPS`**) | [`PERFORMANCE_REPORT.md`](PERFORMANCE_REPORT.md) |
| **Benchmark 250 obj Stress (Release, Shadows 1024)** | `9.04 ms` avg / `11.08 ms` p95 (**`110.6 FPS`**), tăng tốc **3.0×** so với Debug (`27.39 ms`) | [`PERFORMANCE_REPORT.md`](PERFORMANCE_REPORT.md) |
| **Portable Package (`dist/RoomCraft3D-v1.0.0-rc1-win64.zip`)** | `48` tệp, ZIP `2,831,544` bytes (`2.70 MB`), `8` DLL hệ thống Windows, kiểm chứng chạy trong thư mục Unicode + khoảng trắng ngoài PATH WinLibs | [`PACKAGING_GUIDE.md`](PACKAGING_GUIDE.md) |
| **Ảnh chụp GPU thật (`docs/evidence/m9/`)** | `12/12` ảnh PNG `1280×720` chụp trực tiếp từ FBO thật của ứng dụng | [`FINAL_QA_REPORT.md`](FINAL_QA_REPORT.md) |
| **Nghiệm thu thủ công tương tác (M0–M8/M9)** | **`NOT VERIFIED`** — chờ người dùng thao tác trực tiếp & phê duyệt phát hành `v1.0.0` | [`RELEASE_CHECKLIST.md`](RELEASE_CHECKLIST.md) |
