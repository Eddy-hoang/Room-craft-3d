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
| **M5** | Camera, Transformation & Object Picking | **VERIFIED** | 39/39 tự động PASS (`11/11 CTest`, `4.17s`); bao gồm Ray–Triangle & Wall Occlusion; `TC-M5-MANUAL` giữ `NOT VERIFIED` |
| **M6** | Professional Editor UI/UX | TODO | Chờ xác nhận cổng M5 |
| **M7** | Lighting, Materials & Visual Quality | TODO | Chưa bắt đầu |
| **M8** | Collision Detection & Project Persistence | TODO | Chưa bắt đầu |
| **M9** | Testing, Optimization & Release | TODO | Chưa bắt đầu |

---

## Chi tiết công việc Milestone M5 — Camera, Object Picking & Interactive Transformation

| Task ID | Công việc | Ưu tiên | Trạng thái | Bằng chứng / Ghi chú |
|---|---|---|---|---|
| **M5-T01** | Repository Audit & Regression M0–M4 | P0 | DONE | `9/9` CTest PASS trước M5; đối chiếu `TC-M3-02` bằng kết quả OBJ import thực từ `ModelLoaderTests.cpp` |
| **M5-T02** | `InputManager` | P0 | DONE | `InputManager.hpp/cpp`: phân biệt `Pressed`/`Released` 1-frame với `Held`, chống auto-repeat `Delete`/`Ctrl+D`, chuyển tiếp đầy đủ tới ImGui |
| **M5-T03** | Viewport Input Routing | P0 | DONE | `ViewportInteraction.hpp/cpp` + `ViewportRect` đo từ `ImGui::GetItemRectMin/Max`, `IsItemHovered`, `WantTextInput`, Popup |
| **M5-T04** | Orbit Camera | P0 | DONE | `CameraController::orbit()` tọa độ cầu quanh target, kẹp pitch `[-85°, +85°]` |
| **M5-T05** | Pan Camera | P0 | DONE | `CameraController::pan()` dịch chuyển `position` và `target` đồng bộ theo `right`/`trueUp` |
| **M5-T06** | Camera Zoom | P0 | DONE | `CameraController::zoom()` đổi `distance` (Perspective) / `orthoHalfHeight` (Orthographic), không scale cảnh |
| **M5-T07** | Camera Focus & Presets Sync | P0 | DONE | `syncFromCamera()` sau mọi Preset (`1/2/3/Front`) và `Fit Room` / `Fit Selected` (`Home`/`F`) |
| **M5-T08** | Ray Data Structure | P0 | DONE | `Ray.hpp`: `Ray`, `ViewportRect`, kiểm tra finite & chuẩn hóa hướng |
| **M5-T09** | Screen-to-World Ray | P0 | DONE | `Ray::screenToWorldRay()` giải chiếu Near/Far bằng `(P*V)^-1`, hỗ trợ cả Perspective, Orthographic & DPI scaling |
| **M5-T10** | Ray–AABB Intersection | P0 | DONE | `Intersection.cpp`: Slab Method xử lý tia song song, hướng âm, gốc trong box, box sau tia |
| **M5-T11** | Object Picking System | P0 | DONE | `PickingSystem.hpp/cpp`: chọn gần nhất, loại bỏ hidden, chính sách locked, xóa chọn khi click trống, Wall Occlusion |
| **M5-T12** | Selection Highlight | P0 | DONE | Khung bao cyan độc lập theo instance (không sửa material dùng chung), kiểm chứng pixel trong `--m5-smoke-test` |
| **M5-T13** | Ray–Plane Intersection | P0 | DONE | `Plane.hpp` + `intersectRayPlane()` kiểm tra song song, gốc trên mặt phẳng, phía sau tia |
| **M5-T14** | Mouse-based Furniture Dragging | P0 | DONE | `TransformController::beginMove/updateMove`: giữ nguyên `grabOffset` và cao độ `Y` trên mặt phẳng kéo |
| **M5-T15** | Grid Snapping | P0 | DONE | Tính trực tiếp từ vị trí target chưa snap (không tích lũy sai số), hỗ trợ tọa độ âm & giữ nguyên `Y` |
| **M5-T16** | Interactive Rotation | P0 | DONE | `TransformController::beginRotate/updateRotate`: xoay quanh `Y` (hoặc `X`/`Z`), angle snap `15°` |
| **M5-T17** | Interactive Scaling | P0 | DONE | `TransformController::beginScale/updateScale`: hàm mũ luôn dương trong `[0.1, 10.0]`, hỗ trợ uniform / theo trục |
| **M5-T18** | Cancel Transformation | P0 | DONE | `Esc` khôi phục nguyên trạng `initialTransform` và không đóng app khi đang chỉnh sửa |
| **M5-T19** | Keyboard Shortcuts | P0 | DONE | `Q/G/R/S`, `X/Y/Z`, `F`, `Home`, `Delete`, `Ctrl+D`, `1/2/3`, `Esc` (tắt khi đang gõ text/popup) |
| **M5-T20** | Selection / Inspector Synchronization | P0 | DONE | Viewport Picking, Scene Hierarchy và Properties Inspector dùng chung `SelectionState` và cập nhật trong cùng frame |
| **M5-T21** | Ray–Triangle Refinement (Optional P1) | P1 | DONE | Thuật toán Möller–Trumbore trong `PickingSystem`; đã kiểm thử xuyên qua gầm bàn & trên GPU smoke test |
| **M5-T22** | Basic 3-Axis Transform Gizmo (Optional P1) | P1 | TODO | Để mở rộng ở giai đoạn hoàn thiện (chưa triển khai handle 3 trục; không tạo mũi tên trang trí giả) |
| **M5-T23** | Unit & Integration Testing | P0 | DONE | `M5_InteractionUnitTests` (30/30 CPU checks PASS) + `M5_IntegrationSmokeTest` PASS → `11/11 CTest` PASS |
| **M5-T24** | Documentation | P0 | DONE | Tạo `CAMERA_SYSTEM.md`, `OBJECT_PICKING.md`, `INTERACTION_GUIDE.md` và cập nhật toàn bộ tài liệu dự án |
