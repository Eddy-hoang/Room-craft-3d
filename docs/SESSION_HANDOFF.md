# Bàn giao Dự án — RoomCraft 3D (Milestone M9 · `v1.0.0-rc2` Visual Quality Refinement)

Cập nhật: **2026-11-28**

## 1. Trạng thái Hiện hành
- **Phiên bản ứng viên phát hành**: **`v1.0.0-rc2` (`RELEASE CANDIDATE 2`)** — đã hoàn tất toàn bộ giai đoạn **RC2.0 → RC2.7** (P0 + Quality Gate A + P1 + Quality Gate B + Packaging), chưa phát hành chính thức `v1.0.0`.
- **Build & CTest**:
  - **Release (`build/mingw-x64-release/RoomCraft3D.exe`)**: `0` lỗi, `0` cảnh báo dự án, **`19/19` CTest PASS (`5.95 s`)**.
  - **Debug (`build/mingw-x64-debug/RoomCraft3D.exe`)**: `0` lỗi, `0` cảnh báo dự án, **`19/19` CTest PASS (`6.62 s`)**.
- **Hiệu năng Intel UHD (`1280×720`, Release)**: `30 obj + Shadows 1024`: **`126.6 FPS`** (`7.90 ms`), `MSAA 4× + Shadows 1024`: **`99.5 FPS`** (`10.05 ms`), **Complete Editor Frame (ImGui + 4× + 1024)**: **`112.0 FPS`** (`8.93 ms` avg / `11.78 ms` p95).
- **Gói Portable**: `dist/RoomCraft3D-v1.0.0-rc2-win64.zip` (bảo lưu song song gói `v1.0.0-rc1` để đối chiếu), đã xác minh chạy sạch từ thư mục Unicode ngoài repo (`PORTABLE_CLEAN_EXIT=0`, `OpenGL errors: NONE`).
- **Bằng chứng & Tài liệu RC2**:
  - `docs/RC2_IMPLEMENTATION_PLAN.md`, `docs/RC2_VISUAL_QUALITY_REPORT.md`, `docs/RC2_DELIVERY.md`.
  - `docs/evidence/rc2/` (`18` ảnh GPU thật `1280×720` + `4` bảng so sánh RC1 vs RC2 + thư mục baseline `rc1_baseline/`).
  - `examples/ModernBedroom.roomcraft`, `GamingWorkspace.roomcraft`, `MinimalistLivingRoom.roomcraft` đã cập nhật sạch 0 cảnh báo.
- **Git**: Chưa tự động `git commit`, `git tag`, hay `git push`.

## 2. Bước Tiếp theo
1. Tiến hành **Independent QA lần 2** đối chiếu `docs/evidence/rc2/` và tái kiểm tra `BUG-01`..`BUG-08`.
2. Người dùng chạy trực tiếp `dist\RoomCraft3D-v1.0.0-rc2-win64\RoomCraft3D.exe` hoàn tất mục **Manual Desktop & Visual Acceptance** trong [`docs/RELEASE_CHECKLIST.md`](RELEASE_CHECKLIST.md) trước khi quyết định gắn thẻ `v1.0.0`.
