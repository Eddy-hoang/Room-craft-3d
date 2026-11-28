# Danh sách Kiểm tra Phát hành (Release Checklist — `v1.0.0-rc2` → `v1.0.0`)

Trạng thái hiện tại: **`RELEASE CANDIDATE 2 (v1.0.0-rc2)`** — hoàn tất giai đoạn tinh chỉnh chất lượng thị giác (P0 + P1), chưa gắn thẻ `v1.0.0` chính thức cho tới khi hoàn tất **Independent QA lần 2** và **Product Owner Manual & Visual Acceptance**.

## A. Các Cổng Tự động, Thị giác & Đóng gói RC2 (ĐÃ ĐẠT)

- [x] **Debug & Release Builds (`mingw-x64-debug`, `mingw-x64-release`)**: Thành công, `0` lỗi, `0` cảnh báo dự án.
- [x] **Full Automated Regression**: **`19/19` CTest PASS** trên cả **Release (`5.95 s`)** và **Debug (`6.62 s`)**, kèm đầy đủ kiểm thử hồi quy RC2 (`RC2-P0-PRESET`, `RC2-P0-CAM`, `RC2-P0-DIM`, `RC2-P1-MAT`, `RC2-P1-ARCH`, `RC2-GPU-01..03`).
- [x] **Quality Gate A (P0 Visual & Preset Fixes)**:
  - Wall Shadow Acne đã triệt tiêu (`UpperWestWall meanDiff = 0.938 < 1.5`, ảnh diff 4× sạch).
  - Cháy sáng `Warm Interior` & `Gaming Workspace` giảm từ `8.47% / 5.98%` xuống **`0.00%`**.
  - `3/3` phòng mẫu mặc định đạt **`0 overlaps, 0 bounds`**; đồng bộ kích thước `Indoor Plant` & 8 Blender assets.
  - Camera Framing tăng độ phủ silhouette lên **`45.8%–55.4%`** (BBox `81.7%–89.2%`), 0 cắt phòng; nền Studio Gradient; tắt mặc định Grid & Axes.
- [x] **Quality Gate B (P1 Material, Architecture, Modeling & Editor Polish)**:
  - Hiệu chỉnh vật liệu 8 file `.mtl` + vật liệu Procedural + Energy-Aware Fresnel + Anisotropic Filtering + Soft Floor Contact Grounding.
  - Bổ sung Baseboards 4 tường, Cửa sổ trang trí tường Bắc (`showWindow`, tương thích ngược M8/RC1), sàn gỗ chống moiré `fwidth()`, nâng cấp `Procedural Desk` & `Procedural Bookshelf`.
  - Tên thẻ C++ hết cắt chữ (giữ nguyên 14 `assetId`), thumbnail framing `1.16f`, Hierarchy `0.43`, sửa `Validation Mode`, thêm Passive 3D Orientation Indicator.
- [x] **Intel UHD Performance Benchmark (300-frame Release)**: `30 obj + Shadows 1024`: `126.6 FPS` (`7.90 ms`); `MSAA 4× + Shadows 1024`: `99.5 FPS` (`10.05 ms`); **Complete Editor Frame**: **`112.0 FPS` (`8.93 ms` avg / `11.78 ms` p95)**.
- [x] **Visual Evidence (`docs/evidence/rc2/`)**: Đủ `18/18` ảnh GPU thật `1280×720` (gồm `512/1024/2048`, `Iso`, `Room Settings`, `Close-up`) + `4` bảng so sánh RC1 vs RC2 + bảo lưu toàn bộ baseline RC1.
- [x] **Portable Packaging (`v1.0.0-rc2`)**: `dist/RoomCraft3D-v1.0.0-rc2-win64.zip` kèm `SHA256SUMS.txt` (giữ nguyên gói `v1.0.0-rc1` để đối chiếu), đã kiểm chứng thực thi từ thư mục ngoài repo có dấu tiếng Việt (`PORTABLE_CLEAN_EXIT=0`, `OpenGL errors: NONE`).

## B. Các Cổng Nghiệm thu Độc lập & Thủ công (CHỜ XÁC NHẬN — `NOT VERIFIED`)

- [ ] **Independent QA lần 2**: Tái kiểm tra độc lập `BUG-01`..`BUG-08` và chấm điểm thẩm mỹ RC2 (mục tiêu `≥ 8.0/10`).
- [ ] **Manual Desktop Interaction Acceptance**: Chạy trực tiếp `dist\RoomCraft3D-v1.0.0-rc2-win64\RoomCraft3D.exe` và kiểm chứng Orbit/Pan/Zoom, Picking, Move Gizmo, `R/S/Esc`, Win32 native Open/Save dialog, `WM_CLOSE` modal, Preferences persistence và UI Scale `100%/125%/150%`.
- [ ] **Product Owner Final Approval**: Phê duyệt thẩm mỹ thực tế để gắn thẻ `v1.0.0` chính thức.
