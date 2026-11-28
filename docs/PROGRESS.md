# Theo dõi tiến độ dự án — RoomCraft 3D (Progress)

Cập nhật lần cuối: **2026-11-28**

## Tổng quan các Milestone

| Milestone | Mô tả | Trạng thái | Ghi chú |
|---|---|---|---|
| **M0** | Project Foundation | **VERIFIED** | 7/7 tự động PASS; `TC-M0-MANUAL` giữ `NOT VERIFIED` |
| **M1** | Core Graphics Engine | **VERIFIED** | 16/16 tự động PASS; `TC-M1-MANUAL` giữ `NOT VERIFIED` |
| **M2** | Procedural Geometry & Modeling | **VERIFIED** | 23/23 tự động PASS; `TC-M2-MANUAL` giữ `NOT VERIFIED` |
| **M3** | Blender Asset Pipeline | **VERIFIED** | 27/27 tự động PASS; đã đối chiếu `TC-M3-02` OBJ import; `TC-M3-MANUAL` giữ `NOT VERIFIED` |
| **M4** | Scene Management & Room Editor | **VERIFIED** | 29/29 tự động PASS; benchmark 10/30/100 hoàn tất; `TC-M4-MANUAL` giữ `NOT VERIFIED` |
| **M5** | Camera, Transformation & Object Picking | **VERIFIED** | 39/39 tự động PASS (đã ghi rõ ánh xạ trong `TEST_RESULTS.md`); `TC-M5-MANUAL` giữ `NOT VERIFIED` |
| **M6** | Professional Editor UI/UX | **VERIFIED** | **`13/13` CTest PASS (`2.90s`)**; gồm Design System, `Roboto-Medium.ttf`, Vector Icons, 14/14 GPU FBO Thumbnails, 3-Axis Move Gizmo (P1); `TC-M6-MANUAL` & kiểm tra thị giác/screenshot giữ `NOT VERIFIED` |
| **M7** | Lighting, Materials & Visual Quality | TODO | Chưa bắt đầu (dừng sau M6 chờ xác nhận cổng) |
| **M8** | Collision Detection & Project Persistence | TODO | Chưa bắt đầu |
| **M9** | Testing, Optimization & Release | TODO | Chưa bắt đầu |

---

## Chi tiết công việc Milestone M6 — Professional Editor UI/UX

| Task ID | Công việc | Ưu tiên | Trạng thái | Bằng chứng / Ghi chú |
|---|---|---|---|---|
| **M6-T01** | Repository Audit & Regression M0–M5 | P0 | DONE | `11/11` CTest PASS trước M6; `Dear ImGui v1.93.0-docking`; kiểm tra trạng thái process trước build |
| **M6-T02** | Design System & Theme Tokens (`EditorTheme`) | P0 | DONE | `EditorTheme.hpp/cpp`: bảng màu Modern Dark CAD (`#171A20`, `#20242B`, `#292E38`, `#799CE3`...), trục X/Y/Z, spacing 4–24px, `UI Scale 100/125/150%` |
| **M6-T03** | Typography & Safe Font Loading | P0 | DONE | `assets/fonts/Roboto-Medium.ttf` (Apache 2.0, 15px + Vietnamese glyphs) + fallback an toàn về ImGui default |
| **M6-T04** | Vector Icon System (`EditorIcons`) | P0 | DONE | `EditorIcons.hpp/cpp`: 21+ biểu tượng vector thuần `ImDrawList` (MIT), không phụ thuộc emoji Unicode, sắc nét ở mọi DPI |
| **M6-T05** | Product Main Menu Bar & Loại bỏ Tab Milestone | P0 | DONE | `Application::renderProductMainMenuBar()`: `RoomCraft | File | Edit | View | Room | Tools | Help`; loại bỏ hoàn toàn nhãn `M1`/`M2`/`M3`/`M4`/`M5` trên giao diện chính |
| **M6-T06** | Relocate Tools (`Geometry Lab`, `Asset Inspector`, `Developer Tools`) | P0 | DONE | Đưa vào menu `Tools`, `switchWorkspace()` lưu/khôi phục `Scene`, `Camera`, `SelectionState` của Room Editor |
| **M6-T07** | Responsive Workspace Layout & Splitters (`EditorLayout`) | P0 | DONE | `EditorLayout.hpp/cpp` (thuần CPU trong `roomcraft_scene`): thanh kéo dọc/ngang, kẹp min/max không chồng lấn ở `1280×720..1920×1080` & `100..150%` scale, `Reset Workspace Layout` |
| **M6-T08** | Main Toolbar Chuyên nghiệp | P0 | DONE | Biểu tượng vector + tooltip phím tắt (`New`, `Select/Move/Rotate/Scale`, `Snap`, `Gizmo`, `Grid`, `3D/Top/Iso/Fit`, `Room Presets` dropdown) |
| **M6-T09** | GPU Asset Thumbnail Renderer (`ThumbnailRenderer`) | P0 | DONE | `ThumbnailRenderer.hpp/cpp`: render FBO `256×256` góc nhìn studio cho 14/14 mẫu (`3.70 ms` tổng), cache theo ID + asset token, invalidate khi `Reload Asset`, bảo toàn OpenGL state |
| **M6-T10** | Redesigned Furniture Library Panel | P0 | DONE | Chế độ Grid 2 cột ảnh thu nhỏ 256×256 + List View, tìm kiếm, lọc nguồn (`All/C++/OBJ`) & danh mục, double-click hoặc `+ Add Furniture` |
| **M6-T11** | Clean 3D Viewport & Overlay Controls | P0 | DONE | Khung trung tâm lớn nhất, pill góc trên-trái (`Perspective / Top 2D`), chuyển nhanh `Solid / Wire` góc trên-phải, badge trạng thái tương tác góc dưới-trái |
| **M6-T12** | Scene Hierarchy Panel + Icon Controls & Context Menu | P0 | DONE | Gốc tên phòng + `Floor & Walls`, danh sách đối tượng ổn định theo `ObjectId`, nút icon `Eye` / `Lock`, chuột phải Context Menu (`Focus`, `Duplicate`, `Hide/Show`, `Lock/Unlock`, `Reset Transform`, `Delete`) |
| **M6-T13** | Redesigned Properties Inspector | P0 | DONE | Nhóm thu gọn `Object`, `Transform` (ô X/Y/Z mã màu Đỏ/Xanh lá/Xanh dương, `Snap` khi kết thúc edit), `Dimensions`, `Material`, `Geometry`, `Actions` + tab `Room Settings` |
| **M6-T14** | Clean Status Bar | P0 | DONE | Hiển thị kích thước phòng, số vật thể, lưới, chế độ tương tác, tọa độ thế giới con trỏ và trạng thái `Scene ready` (FPS chuyển vào `Performance Monitor` / tùy chọn bật) |
| **M6-T15** | Non-blocking Notification System (`NotificationSystem`) | P0 | DONE | `NotificationSystem.hpp` (thuần CPU): hàng đợi Toast tối đa 4 mục góc dưới-phải Viewport, tự ẩn sau `3.0 s`, phân loại `Info / Success / Warning` |
| **M6-T16** | Confirmation Modals | P0 | DONE | Hộp thoại `Confirm Replace Scene` (khi `New Room` / đổi Preset có thay đổi) và `Confirm Delete Object` (khi xóa từ nút/menu) |
| **M6-T17** | Product Dialogs (`Performance`, `OpenGL`, `Shortcuts`, `Guide`, `System`, `About`) | P0 | DONE | `Application::renderProductDialogs()` đầy đủ, không tạo nút giả cho tính năng M7/M8 |
| **M6-T18** | Input Capture & Interaction Integrity | P0 | DONE | Giữ nguyên toàn bộ Orbit/Pan/Zoom, Ray–AABB + Ray–Triangle + Wall Occlusion, Drag/Rotate/Scale/Cancel/Shortcuts; không xung đột với modal/text input |
| **M6-T19** | Asset & Font Path Resolution Across Working Directories | P0 | DONE | Xác minh chạy `RoomCraft3D.exe --m6-smoke-test` từ `C:\` ngoài PATH WinLibs nạp đúng `assets/fonts/Roboto-Medium.ttf`, shaders và 14 mẫu |
| **M6-T20** | Performance Check (M6 vs M4) | P0 | DONE | 30 vật thể off-screen `1280×720`: `1.68 ms (595.3 FPS)` vs M4 `1.50 ms (667.8 FPS)`; 14 thumbnails cache 1 lần `3.70 ms` (`0.265 ms/thumb`) |
| **M6-T21** | Basic 3-Axis Move Gizmo (P1) | P1 | DONE | `TransformGizmo.hpp/cpp` + `SceneRenderer::renderMoveGizmo()`: hit-testing 3 trục X/Y/Z ưu tiên trước picking cảnh, kích thước màn hình ổn định, hover/active highlight, chặn khi `Locked`, `Esc` khôi phục |
| **M6-T22** | Rotate / Scale Visual Gizmos (Optional P2) | P2 | TODO | Không bắt buộc trong M6; giữ `TODO` để mở rộng ở giai đoạn hoàn thiện mà không tạo hình vẽ giả |
| **M6-T23** | UI Layout Persistence Across Sessions (Optional P1) | P1 | TODO | Chưa lưu/tải tỷ lệ splitter ra file cấu hình giữa các lần mở app (`Reset Workspace Layout` hoạt động trong phiên); dự kiến gom cùng Persistence M8/M9 |
| **M6-T24** | Automated Testing & Documentation | P0 | DONE | `M6_EditorUiUnitTests` + `M6_IntegrationSmokeTest` → `13/13 CTest PASS (2.90s)`; tạo `DESIGN_SYSTEM.md`, `UI_ARCHITECTURE.md`, `EDITOR_USER_GUIDE.md` và cập nhật toàn bộ tài liệu |
