# Danh sách Kiểm tra Phát hành (Release Checklist — `v1.0.0-rc1` → `v1.0.0`)

Trạng thái hiện tại: **`RELEASE CANDIDATE (v1.0.0-rc1)`** — chưa gắn thẻ `v1.0.0` chính thức cho tới khi người dùng xác nhận các mục nghiệm thu thủ công bên dưới.

## A. Các Cổng Tự động & Đóng gói (ĐÃ ĐẠT)

- [x] **Debug Build (`mingw-x64-debug`)**: Thành công, `0` lỗi, `0` cảnh báo dự án (`-Wall -Wextra -Wpedantic`).
- [x] **Release Build (`mingw-x64-release`)**: Thành công, `0` lỗi, `0` cảnh báo dự án, `7.59 MB`.
- [x] **Full Automated Regression**: **`19/19` CTest PASS** trên cả Debug (`4.82 s`) và Release (`3.56 s`).
- [x] **Reliability & Safety Audit (M9.1)**: `M9_ReliabilityUnitTests` (JSON UTF-8/duplicate-key/1500-case fuzzing, locked-file failure injection, exact Undo/Redo dirty-state, non-trapping Prevent Invalid, PreferencesManager) — **PASS**.
- [x] **Performance Benchmark (M9.2)**: Đạt mục tiêu >60 FPS (`366.5 FPS` tiêu chuẩn 30 obj + Shadows 1024; `292.2 FPS` với MSAA 4×; `110.6 FPS` ở stress 250 obj trên Intel UHD Release).
- [x] **Optional Graphics (M9.4)**: Directional Shadow Mapping (512/1024/2048 + 3×3 PCF + toggle) đã triển khai & kiểm chứng; Rotate/Scale Gizmo được ghi nhận `DEFERRED`.
- [x] **Portable Packaging (M9.6)**: `dist/RoomCraft3D-v1.0.0-rc1-win64.zip` kèm `SHA256SUMS.txt`, chỉ link 8 DLL Windows, đã test giải nén tại đường dẫn Unicode + khoảng trắng với `PATH` sạch.
- [x] **Visual Evidence (GPU thật)**: `12/12` ảnh PNG `1280×720` tại `docs/evidence/m9/` + 3 dự án mẫu `examples/*.roomcraft`.
- [x] **Licenses & Academic Docs**: `licenses/THIRD_PARTY_NOTICES.txt`, `docs/ACADEMIC_REPORT_GUIDE.md`, `docs/EDITOR_USER_GUIDE.md`, `docs/FINAL_QA_REPORT.md`, `docs/PERFORMANCE_REPORT.md`, `docs/PACKAGING_GUIDE.md`.

## B. Các Cổng Nghiệm thu Thủ công & Phê duyệt Người dùng (CHỜ XÁC NHẬN — `NOT VERIFIED`)

Chạy trực tiếp từ gói portable (`dist\RoomCraft3D-v1.0.0-rc1-win64\RoomCraft3D.exe`) hoặc `build\mingw-x64-release\RoomCraft3D.exe`:

- [ ] **Giao diện & Tiêu đề**: Kiểm tra thanh tiêu đề Windows hiển thị đúng `RoomCraft 3D — Modern Bedroom` (không vỡ `â€”`), có biểu tượng 3D trên cửa sổ/taskbar.
- [ ] **UI Layout Persistence**: Kéo đổi độ rộng panel trái/phải, đổi sang List View hoặc UI Scale 125% → đóng ứng dụng → mở lại xác nhận bố cục được giữ nguyên; thử `View -> Reset Workspace Layout`.
- [ ] **Đồ họa & Ánh sáng**: Thử bật/tắt `Directional Shadows`, đổi `512/1024/2048`, thử cả 4 preset ánh sáng (`Daylight`, `Warm Interior`, `Neutral Studio`, `Evening`), bật/tắt `Grid` và `MSAA 4x`.
- [ ] **Tương tác M5–M6**: Orbit/Pan/Zoom, chọn bằng chuột (picking), kéo Move Gizmo 3 trục, xoay (`R`), phóng to/thu nhỏ (`S`), `Esc` hủy, mở `Tools -> Geometry Lab` & `Asset Inspector` rồi quay lại giữ nguyên cảnh.
- [ ] **Va chạm M8**: Kiểm tra `Warning Only` (chồng đồ hiện hổ phách, vượt tường hiện đỏ san hô, viền cyan vẫn rõ, đồ trên thảm/đèn trên bàn không báo sai) và `Prevent Invalid` (chặn đặt mới lỗi, cho phép kéo vật lỗi sẵn về vùng hợp lệ).
- [ ] **Lưu/Mở & Undo/Redo M8–M9**: Mở `examples\ModernBedroom.roomcraft`, sửa phòng/đồ → thấy dấu `*`, bấm `Ctrl+Z` về đúng bản lưu thấy mất dấu `*`; `Save As` tên tiếng Việt có khoảng trắng; thử mở file hỏng không mất cảnh; bấm nút `X` cửa sổ kiểm tra `Save / Discard / Cancel`.
- [ ] **Phê duyệt phát hành `v1.0.0`**: Người dùng xác nhận đạt yêu cầu để gắn thẻ `git tag -a v1.0.0` và phát hành bản cuối.
