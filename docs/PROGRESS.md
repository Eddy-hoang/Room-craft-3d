# Theo dõi tiến độ dự án — RoomCraft 3D (Progress)

Cập nhật lần cuối: **2026-11-28**

## Tổng quan các Milestone

| Milestone | Mô tả | Trạng thái | Ghi chú |
|---|---|---|---|
| **M0** | Project Foundation | **VERIFIED** | 7/7 tự động PASS; `TC-M0-MANUAL` giữ `NOT VERIFIED` |
| **M1** | Core Graphics Engine | **VERIFIED** | 16/16 tự động PASS; `TC-M1-MANUAL` giữ `NOT VERIFIED` |
| **M2** | Procedural Geometry & Modeling | **VERIFIED** | 23/23 tự động PASS; `TC-M2-MANUAL` giữ `NOT VERIFIED` |
| **M3** | Blender Asset Pipeline | **VERIFIED** | 27/27 tự động PASS; đã đối chiếu 11 PNG & SHA256; `TC-M3-MANUAL` giữ `NOT VERIFIED` |
| **M4** | Scene Management & Room Editor | **VERIFIED** | 29/29 tự động PASS (`9/9 CTest`, `2.76s`); benchmark 10/30/100 vật thể hoàn tất; `TC-M4-MANUAL` giữ `NOT VERIFIED` |
| **M5** | Camera, Transformation & Object Picking | TODO | Chờ xác nhận cổng M4 |
| **M6** | Professional Editor UI/UX | TODO | Chưa bắt đầu |
| **M7** | Lighting, Materials & Visual Quality | TODO | Chưa bắt đầu |
| **M8** | Collision Detection & Project Persistence | TODO | Chưa bắt đầu |
| **M9** | Testing, Optimization & Release | TODO | Chưa bắt đầu |

---

## Chi tiết công việc Milestone M4 — Scene Management & Room Editor

| Task ID | Công việc | Ưu tiên | Trạng thái | Bằng chứng / Ghi chú |
|---|---|---|---|---|
| **M4-T01** | Audit M3 & regression tests | P0 | DONE | Xác minh 11 PNG, sửa Assimp `v6.0.6` commit SHA, khóa `URL_HASH SHA256` trong `CMakeLists.txt`, chạy lại `7/7` CTest PASS |
| **M4-T02** | Thiết kế `SceneObject` | P0 | DONE | `SceneObject.hpp`: `id`, `name`, `assetId`, `transform`, `visible`, `locked`, `getWorldBounds()` — tách khỏi GPU handle |
| **M4-T03** | Stable Object ID | P0 | DONE | `ObjectId.hpp`: `uint64_t` tăng đơn điệu, không dùng vector index, không tái sử dụng ID đã xóa |
| **M4-T04** | Xây dựng `Scene` class | P0 | DONE | `Scene.hpp/cpp`: `addObject`, `removeObject`, `duplicateObject`, `findObject`, `renameObject`, `setObjectVisibility`, `setObjectTransform`, `clearObjects` |
| **M4-T05** | Xây dựng `RoomSettings` | P0 | DONE | `RoomSettings.hpp`: 5×6×3m mặc định, giới hạn 2–20m / 2–6m / 0.05–0.50m, validation đầy đủ, `geometryEquals()` |
| **M4-T06** | Sinh sàn và tường theo tham số | P0 | DONE | `Room.hpp/cpp`: tái sử dụng `PrimitiveGenerator::createCuboid`, sàn `y = 0`, 4 tường ngoài biên không hở góc, trần & lưới sàn |
| **M4-T07** | Room resize | P0 | DONE | `Room::applySettings()` transactional rebuild; đổi màu/visibility không rebuild mesh; giữ nguyên transform nội thất |
| **M4-T08** | Wall Visibility System | P0 | DONE | 4 chế độ (`AllWalls`, `HideCeiling`, `CutawayView`, `Manual`) bật/tắt từng tường & trần, không đổi interior bounds |
| **M4-T09** | Furniture Placement | P0 | DONE | `PlacementSystem.hpp/cpp`: đặt 14 asset vào phòng, offset lệch sơ bộ, hỗ trợ chỉnh `Position Y` lên mặt bàn |
| **M4-T10** | Multi-object rendering | P0 | DONE | `SceneRenderer.hpp/cpp`: render phòng + nhiều `SceneObject` + Grid/Axes + Selected BBox vào FBO |
| **M4-T11** | Shared GPU asset resources | P0 | DONE | Dùng chung `std::shared_ptr<GpuAsset>` từ `AssetManager`; an toàn khi xóa, nhân bản, clear scene & reload asset |
| **M4-T12** | Scene Hierarchy | P0 | DONE | Hiển thị Room Structure & Furniture Objects với Stable `ObjectId` làm ImGui ID, Select/Visibility/Duplicate/Delete/Search |
| **M4-T13** | `SelectionState` | P0 | DONE | `SelectionState.hpp`: chọn theo `ObjectId`, tự xóa chọn an toàn qua `reconcileWithScene()` khi xóa object |
| **M4-T14** | Properties Inspector | P0 | DONE | Hiển thị & sửa Name, Visible, Position/Rotation/Scale (Uniform Scale), Reset Transform, Asset/World BBox, cảnh báo vượt biên |
| **M4-T15** | Add/Delete/Duplicate/Rename | P0 | DONE | Liên kết trực tiếp giữa Library, Hierarchy, Toolbar và Inspector tới `Scene` thật |
| **M4-T16** | Room Settings UI | P0 | DONE | Sửa kích thước, màu sắc, Wall Mode, tường riêng lẻ, Apply/Reset, hiển thị Area/Volume/Object Count/Interior Bounds |
| **M4-T17** | Room Camera Fit | P0 | DONE | `Fit Room`, `Fit Selected`, `3D`, `Iso`, `Top 2D`, `Front` dùng `Camera::fitBoundingBox()` |
| **M4-T18** | Three Scene Presets | P0 | DONE | `Modern Bedroom (5×6×3m)`, `Gaming Workspace (4×5×3m)`, `Minimalist Living Room (6×7×3m)` kèm hộp thoại xác nhận khi có chỉnh sửa |
| **M4-T19** | Grid Snapping cơ bản | P1 | DONE | `PlacementSystem::snapToGrid()` (snap X/Z, giữ nguyên Y) + checkbox `Snap` trên Toolbar |
| **M4-T20** | Performance Benchmark | P0 | DONE | `--m4-benchmark` đo thực tế 10 / 30 / 100 vật thể tại `1280×720` trên Intel UHD Graphics (`667.8 FPS` ở 30 vật thể Debug) |
| **M4-T21** | Unit & Integration Tests | P0 | DONE | `M4_SceneUnitTests` (23/23 checks PASS) + `M4_IntegrationSmokeTest` (`TC-M4-21..29` PASS) → `9/9 CTest` PASS |
| **M4-T22** | Documentation | P0 | DONE | Tạo `docs/SCENE_MANAGEMENT.md`, `docs/ROOM_EDITOR_GUIDE.md` và cập nhật toàn bộ tài liệu dự án |
