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
| **M5** | Camera, Transformation & Object Picking | **VERIFIED** | 39/39 tự động PASS; `TC-M5-MANUAL` giữ `NOT VERIFIED` |
| **M6** | Professional Editor UI/UX | **CONDITIONAL PASS** | 13/13 CTest PASS; `TC-M6-MANUAL` & kiểm tra thị giác giữ `NOT VERIFIED` |
| **M7** | Lighting, Materials & Visual Quality | **IMPLEMENTED** | Final QA 14/14 PASS (`CONDITIONAL PASS`), Shadow Mapping `DEFERRED`, Manual/Visual `NOT VERIFIED` — [`M7_DELIVERY.md`](M7_DELIVERY.md) |
| **M8** | Collision Detection & Project Persistence | **IMPLEMENTED** | **`17/17` CTest PASS (`6.65 s`)**; AABB/OBB SAT, Room Boundary, `.roomcraft` JSON v1, Safe Atomic Save, Transactional Load, Dirty-State, Recent Projects (P1), Undo/Redo (P2); `M8-T32` Manual/Visual giữ `NOT VERIFIED` — [`M8_DELIVERY.md`](M8_DELIVERY.md) |
| **M9** | Testing, Optimization & Release | TODO | Chưa bắt đầu (dừng đúng cổng M8) |

---

## Chi tiết công việc Milestone M8 — Collision Detection & Project Persistence (`v0.8.0`)

| ID | Chức năng | Ưu tiên | Trạng thái | Bằng chứng / Ghi chú |
|---|---|---|---|---|
| **M8-F01** | AABB Collision Detection | P0 | DONE | `OBB.hpp/cpp`: `intersectAABB()` phân biệt `Separated / Touching / Penetrating`; `M8-T02/03` PASS |
| **M8-F02** | OBB Collision Detection | P0 | DONE | `OBB::fromLocalBoundsAndTransform()` + `toWorldAABB()`, hỗ trợ xoay 3D & non-uniform scale; `M8-T04/05/07` PASS |
| **M8-F03** | Separating Axis Theorem (3D SAT) | P0 | DONE | `SATIntersection::testOBB()`: 15 trục (3+3+9), xử lý gần song song, đối xứng; `M8-T04..06` PASS |
| **M8-F04** | Room Boundary Validation | P0 | DONE | `RoomBoundaryValidator`: kiểm tra 6 mặt từ World AABB đã xoay, bảo toàn vật nâng hợp lệ, re-validate sau resize; `M8-T08/09` PASS |
| **M8-F05** | Collision Warnings | P0 | DONE | Khung cảnh báo hổ phách/đỏ san hô trong `SceneRenderer` (giữ viền cyan chọn), liệt kê tên vật xung đột trong Inspector & Status Bar; `M8-T28` PASS |
| **M8-F06** | Placement Validation (`Off / Warning / Prevent`) | P0 | DONE | Mặc định `Warning Only`, hỗ trợ `Prevent Invalid` khôi phục transform hợp lệ + Toast (không modal), cho phép đặt trên thảm |
| **M8-F07** | New Project | P0 | DONE | `File -> New Project (Ctrl+N)`, bảo vệ thay đổi chưa lưu qua `Save / Discard / Cancel` |
| **M8-F08** | Save Project | P0 | DONE | `File -> Save (Ctrl+S)`, ghi nguyên tử qua tệp tạm + `MoveFileExW`; `M8-T25` PASS |
| **M8-F09** | Save Project As | P0 | DONE | `File -> Save As... (Ctrl+Shift+S)`, hộp thoại Win32 `GetSaveFileNameW` hỗ trợ Unicode & `OFN_OVERWRITEPROMPT` |
| **M8-F10** | Open Project | P0 | DONE | `File -> Open Project... (Ctrl+O)`, hộp thoại Win32 `GetOpenFileNameW` (`OFN_NOCHANGEDIR`) |
| **M8-F11** | JSON Serialization (`.roomcraft`) | P0 | DONE | Bộ JSON RFC 8259 nội bộ (`Json.hpp/cpp`) + `ProjectSerializer` lưu toàn bộ cảnh, ánh sáng M7, material overrides, camera, MSAA; `M8-T11..18` PASS |
| **M8-F12** | Schema Versioning | P0 | DONE | `format = "RoomCraft3D"`, `schemaVersion = 1` độc lập app version, từ chối schema không hỗ trợ; `M8-T20` PASS |
| **M8-F13** | Transactional Project Loading | P0 | DONE | `ProjectManager::openTransactional()` + `Scene::restoreFromSnapshot()` dựng ứng viên rồi mới commit; `M8-T24` PASS |
| **M8-F14** | Unsaved Changes Tracking | P0 | DONE | Dấu `*` trên tiêu đề cửa sổ & menu bar, `Save / Discard / Cancel` kể cả khi bấm `X` (`WM_CLOSE` subclass); `M8-T26/27` PASS |
| **M8-F15** | Project Data Validation | P0 | DONE | `ProjectValidator`: kiểm tra ID trùng/0/overflow, thiếu asset, giá trị NaN/out-of-range, giới hạn 16 MB & 5000 vật; `M8-T19..23` PASS |
| **M8-F16** | Recent Projects | P1 | DONE | `File -> Recent Projects` (tối đa 8 tệp gần nhất); `M8-T26/27` PASS |
| **M8-F17** | Undo / Redo | P2 | DONE | `CommandHistory` giới hạn 64 bước (`Ctrl+Z` / `Ctrl+Y`, 1 lần kéo = 1 bước); `M8-T31` PASS |
| **M8-F18** | UI Layout Persistence | P2 | TODO | Hoãn sang M9 theo ưu tiên P2 để giữ tập trung vào độ ổn định Collision & Persistence |
