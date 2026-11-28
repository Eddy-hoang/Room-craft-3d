# Báo cáo Bàn giao M9 — Release Candidate 2 (`v1.0.0-rc2` · Visual Quality Refinement)

- **Phiên bản**: `v1.0.0-rc2` (Giai đoạn tinh chỉnh chất lượng thị giác thuộc Milestone M9 — không tạo M10)
- **Môi trường kiểm chứng**: Windows 11 x64 (`10.0.26200`), `Intel(R) UHD Graphics` (Driver `32.0.101.7088`, OpenGL 3.3 Core Profile), WinLibs MinGW-w64 UCRT `g++ 16.2.0`, CMake 4.4.4, Ninja 1.13.2
- **Trạng thái**: **IMPLEMENTED & AUTO-VERIFIED (`v1.0.0-rc2`)** — Chờ **Independent QA lần 2** và **Product Owner Visual & Manual Acceptance** trước khi phát hành `v1.0.0`.

---

## 1. Tóm tắt Kết quả Chính

1. **Khắc phục 8/8 lỗi từ Independent QA RC1 (`BUG-01` → `BUG-08`)**:
   - **Shadow Mapping (`RC2.1`)**: Chuyển `ShadowMap` sang hardware `sampler2DShadow` (`GL_COMPARE_REF_TO_TEXTURE` + `GL_LINEAR`), frustum vuông có texel-snapping, `GL_FRONT` culling riêng cho tường đặc + `glPolygonOffset(1.4, 1.8)`, 5-tap bilinear PCF + normal-offset bias → triệt tiêu hoàn toàn wall shadow acne (`meanDiff = 0.938 < 1.5`), hết răng cưa & peter-panning.
   - **Lighting & ACES Tone Mapping (`RC2.2`)**: Kiểm toán tuyến tính → ACES Filmic Tone Mapping trước `displayColor()` (chỉ 1 lần chuyển sRGB, không tác động UI/overlays), cân chỉnh 4 presets, 2-color hemisphere ambient, sửa `computeRoomCornerAO` theo `uVisibleWalls` & `uIsRoomSurface` → giảm pixel cháy sáng từ `8.47%` (`Warm Interior`) và `5.98%` (`Gaming Workspace`) xuống **`0.00%`** trên toàn bộ 4 presets.
   - **Camera Framing, Studio View & Presets (`RC2.3`)**: `CameraController::fitRoomView` căn tâm NDC 3 vòng trên các đỉnh phòng thực tế đang hiển thị → nâng độ phủ silhouette từ `22.5%–26.5%` lên **`45.8%–55.4%`** (projected bbox `81.7%–89.2%`, 0 cắt phòng); nền Studio radial gradient; ẩn mặc định Grid & Axes (giữ đủ nút bật lại); đồng bộ kích thước 8 Blender assets trong `FurnitureCatalog` (`Indoor Plant`: `0.89 × 1.08 × 0.87 m`); sửa `Gaming Workspace` đạt **`0 overlaps, 0 bounds`** trên cả 3 preset mặc định.
   - **Materials & Contact Grounding (`RC2.4`)**: Hiệu chỉnh toàn bộ 8 file `.mtl` (loại bỏ `Ks = 0.50` đồng loạt, giữ nguyên `ComputerAluminum`), Energy-aware Blinn-Phong + Schlick Fresnel, Anisotropic Filtering an toàn theo Core Profile (`glGetStringi`), Soft Contact Grounding Shadows trên sàn.
   - **Architecture, Procedural Modeling & Composition (`RC2.5`)**: Thêm Baseboards 4 tường, Cửa sổ kiến trúc trang trí tường Bắc (`showWindow`, tương thích ngược `.roomcraft`), sàn gỗ chống moiré `fwidth()`, nâng cấp `Procedural Desk` (cạnh vát + ngăn kéo + tay nắm đồng) & `Procedural Bookshelf` (plinth + sách & hộp procedural), cải thiện bố cục 3 phòng mẫu.
   - **Editor Presentation (`RC2.6`)**: Bỏ tiền tố `"Procedural "` cắt chữ trên 6 thẻ C++ (giữ nguyên `assetId` 100%), nâng cấp thumbnail framing (`1.16f`), mở rộng `Scene Hierarchy` (`0.43`), sửa nhãn `Validation Mode`, thêm Passive 3D Orientation Indicator.

2. **Kiểm thử Tự động & Tương thích Ngược**:
   - **Release**: **`19/19 CTest PASS`** (`6.86 s`, 0 build errors, 0 project warnings).
   - **Debug**: **`19/19 CTest PASS`** (`6.10 s`, 0 build errors, 0 project warnings).
   - Bổ sung đầy đủ kiểm thử hồi quy mới mà **không làm yếu bất kỳ test cũ nào**: `RC2-P0-PRESET`, `RC2-P0-CAM`, `RC2-P1-MAT`, `RC2-P1-ARCH`, `RC2-GPU-01`, `RC2-GPU-02`, `RC2-GPU-03`, cùng kiểm tra tương thích ngược schema v1 M8/RC1 thiếu trường `showWindow` & shadow fields.

3. **Hiệu năng Thực tế trên Intel UHD Graphics (`1280 × 720`, Release `-O3`)**:
   - **30 obj · MSAA Off · Shadows 1024 (300 frames)**: `7.90 ms` avg / `10.36 ms` p95 (**`126.6 FPS`**).
   - **30 obj · MSAA 4× · Shadows 1024 (300 frames)**: `10.05 ms` avg / `13.11 ms` p95 (**`99.5 FPS`**).
   - **Complete Editor Frame (ImGui + 30 obj + MSAA 4× + Shadows 1024)**: `8.93 ms` avg / `11.78 ms` p95 (**`112.0 FPS`** — vượt xa mục tiêu 60 Hz / `16.67 ms`).
   - **100 obj · Shadows 1024**: `14.39 ms` avg (**`69.5 FPS`**).
   - **250 obj · Stress · Shadows 1024**: `25.95 ms` avg (**`38.5 FPS`** — tăng ~14 ms so với RC1 do độ phủ màn hình tăng >2× và chi tiết sách/baseboards tăng draw call từ `2,896 → 3,329`; có tùy chọn `Shadows 512 / Off` để fallback nếu người dùng dựng cảnh >150 vật thể).

4. **Đóng gói Portable `v1.0.0-rc2`**:
   - Tệp phát hành: `dist/RoomCraft3D-v1.0.0-rc2-win64.zip` (đồng thời giữ nguyên bản `v1.0.0-rc1` để đối chiếu).
   - Đã kiểm chứng thực thi từ thư mục ngoài repo chứa dấu tiếng Việt (`%TEMP%\RoomCraft3D_RC2_Phòng 3D\RoomCraft3D.exe --m9-smoke-test` → `PASS`, exit `0`, `OpenGL errors: NONE`).

5. **Trạng thái Nghiệm thu còn chờ**:
   - **Independent QA lần 2**: `PENDING`
   - **Manual Desktop Interaction Acceptance** (chuột Orbit/Pan/Zoom, kéo Gizmo, hộp thoại Win32 Save/Open, `WM_CLOSE`, UI Scale 125–150%): `NOT VERIFIED`
   - **Product Owner Final Visual Acceptance & Phát hành `v1.0.0`**: `PENDING`
