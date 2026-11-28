# Bàn giao Dự án — RoomCraft 3D (Milestone M9 · `v1.0.0-rc1` Release Candidate)

Cập nhật: **2026-11-28**

## 1. Trạng thái Hiện hành
- **Phiên bản ứng viên phát hành**: **`v1.0.0-rc1` (`RELEASE CANDIDATE`)** — phân biệt rõ với bản phát hành chính thức `v1.0.0` cho tới khi người dùng phê duyệt cổng nghiệm thu cuối.
- **Build & CTest**:
  - **Debug (`build/mingw-x64-debug/RoomCraft3D.exe`, `47.99 MB`)**: `0` lỗi, `0` cảnh báo dự án, **`19/19` CTest PASS (`4.82 s`)**.
  - **Release (`build/mingw-x64-release/RoomCraft3D.exe`, `7.59 MB`)**: `0` lỗi, `0` cảnh báo dự án, **`19/19` CTest PASS (`3.56 s`)**, chỉ link `8` DLL hệ thống Windows.
- **Gói Portable**: `dist/RoomCraft3D-v1.0.0-rc1-win64.zip` (`2,831,544` bytes, SHA-256 `23ef9d9596994e4fc0293ecf22c229a8c83c0a74e90b8bd041e18b680d496e79`), đã kiểm chứng chạy trong thư mục cô lập chứa dấu tiếng Việt & khoảng trắng ngoài PATH WinLibs.
- **Bằng chứng & Tài liệu**:
  - `12/12` ảnh GPU thật `1280×720` tại `docs/evidence/m9/`.
  - `3` dự án mẫu tại `examples/` (`ModernBedroom.roomcraft`, `GamingWorkspace.roomcraft`, `MinimalistLivingRoom.roomcraft`).
  - Báo cáo đầy đủ: `docs/M9_DELIVERY.md`, `docs/FINAL_QA_REPORT.md`, `docs/PERFORMANCE_REPORT.md`, `docs/PACKAGING_GUIDE.md`, `docs/RELEASE_CHECKLIST.md`, `docs/ACADEMIC_REPORT_GUIDE.md`, `licenses/THIRD_PARTY_NOTICES.txt`.
- **Git**: Chưa tự động `git commit`, `git tag`, hay `git push`; toàn bộ thay đổi M7 + M8 + M9 được bảo toàn nguyên vẹn trong working tree chờ người dùng rà soát và phê duyệt.

## 2. Hành động Cuối cùng cần Người dùng Thực hiện để Chốt `v1.0.0`
1. Chạy thử bản portable `dist\RoomCraft3D-v1.0.0-rc1-win64\RoomCraft3D.exe` và đối chiếu checklist trong [`docs/RELEASE_CHECKLIST.md`](RELEASE_CHECKLIST.md).
2. Khi đã hài lòng, thực hiện `git commit` và gắn thẻ `git tag -a v1.0.0` theo lệnh gợi ý trong báo cáo bàn giao M9.
3. Sử dụng [`docs/ACADEMIC_REPORT_GUIDE.md`](ACADEMIC_REPORT_GUIDE.md), ảnh `docs/evidence/m9/` và số liệu `docs/PERFORMANCE_REPORT.md` để hoàn thiện báo cáo & slide bảo vệ đồ án Đồ họa Máy tính.
