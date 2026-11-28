# Bàn giao Phiên làm việc — RoomCraft 3D (Milestone M8 · `v0.8.0`)

Cập nhật: **2026-11-28**

## 1. Trạng thái Hiện hành
- **M8 (`v0.8.0`)**: **`IMPLEMENTED`** — toàn bộ kiểm thử tự động đạt **`17/17` CTest PASS** trong `6.65 s` (`0` build errors, `0` project warnings).
- **Executable chuẩn**: `build/mingw-x64-debug/RoomCraft3D.exe` (chạy độc lập ngoài PATH WinLibs, chỉ link 8 DLL hệ thống Windows).
- **Trạng thái các milestone trước (giữ nguyên trung thực)**:
  - **M6**: `CONDITIONAL PASS` (chờ nghiệm thu thủ công/thị giác).
  - **M7**: `IMPLEMENTED` (Final QA `CONDITIONAL PASS`, Shadow Mapping `DEFERRED`, Performance `CONDITIONAL PASS`, Manual/Visual `NOT VERIFIED`).
- **Git**: Chưa tự động `git commit` hay `git tag`; working tree bao gồm thay đổi M7 + M8 chờ người dùng kiểm tra và commit theo ý muốn.

## 2. Việc cần Người dùng Nghiệm thu trước khi Mở cổng M9
1. Chạy `.\build\mingw-x64-debug\RoomCraft3D.exe` và xác nhận checklist thủ công trong [`docs/M8_DELIVERY.md`](M8_DELIVERY.md) (`Warning Only`, `Prevent Invalid`, Room Resize, `.roomcraft` Save/Save As/Open/Recent, `Save / Discard / Cancel` khi bấm `X`, `Ctrl+Z`/`Ctrl+Y`).
2. Xác nhận các mục manual/visual tồn đọng của M6–M7 trong [`docs/M7_DELIVERY.md`](M7_DELIVERY.md).

## 3. Hạng mục Chuyển tiếp sang Milestone M9
- Benchmark cô lập bản **`Release`** (`mingw-x64-release`), đo p95/p99 ổn định khi tắt cạnh tranh cửa sổ nền.
- Xem xét triển khai các hạng mục tùy chọn còn mở: `UI Layout Persistence` (`M8-F18`), `Recent Projects` lưu giữa các phiên chạy, `Shadow Mapping` (M7 P2) nếu nằm trong ngân sách khung hình, tổng nghiệm thu và đóng gói phát hành **`v1.0.0`**.
