# Báo cáo Bàn giao RoomCraft 3D — Milestone M8 · `v0.8.0`

**Trạng thái Milestone M8**: **`IMPLEMENTED`** *(Kiểm thử tự động CPU + GPU đạt **`17/17` CTest PASS** trong `6.65 s`; chờ người dùng thực hiện nghiệm thu thủ công & trực quan `M8-T32` và các mục tồn đọng M6–M7).*

> **Ghi chú trung thực về trạng thái các milestone trước**:
> - **M6** giữ **`CONDITIONAL PASS`** (chờ nghiệm thu thủ công/thị giác).
> - **M7** giữ **`IMPLEMENTED`** (Final QA `CONDITIONAL PASS`, Shadow Mapping giữ `DEFERRED`, Performance giữ `CONDITIONAL PASS`, Manual/Visual giữ `NOT VERIFIED`).
> - Không đánh dấu M6, M7 hay M8 là `DONE` khi chưa có nghiệm thu thủ công & trực quan thực tế từ người dùng.

---

## 1. Baseline Audit (M8.0)

- **Baseline trước khi bắt đầu M8**: `v0.7.0`, working tree chứa thay đổi M7 chưa commit (được bảo toàn 100%, không xóa/ghi đè).
- **CTest baseline**: `14/14 PASS` trong `2.97 s` trên `build/mingw-x64-debug/RoomCraft3D.exe`.
- **Toolchain & GPU kiểm chứng**: Windows 11 x64 (`10.0.26200`), `Intel(R) UHD Graphics` (OpenGL `3.3.0 Core Profile`, Driver `32.0.101.7088`), WinLibs MinGW-w64 x64 UCRT (`g++ 16.2.0`), CMake `4.4.4`, Ninja `1.13.2`.

---

## 2. Tóm tắt Kiến trúc & Các Thành phần Đã Triển khai

| Thành phần | Đường dẫn chính | Trách nhiệm |
|---|---|---|
| **World AABB & OBB** | [`OBB.hpp`](../include/roomcraft/math/OBB.hpp), [`OBB.cpp`](../src/math/OBB.cpp) | Dựng World OBB chính xác từ `BoundingBox` + `Transform` TRS (hỗ trợ xoay 3D & non-uniform scale, từ chối scale `<= 0`/NaN), chiếu `toWorldAABB()`, phân loại `Separated / Touching / Penetrating` |
| **3D OBB SAT (15 trục)** | [`SATIntersection.hpp`](../include/roomcraft/math/SATIntersection.hpp), [`SATIntersection.cpp`](../src/math/SATIntersection.cpp) | Kiểm tra 3 trục A + 3 trục B + 9 tích có hướng $A_i \times B_j$, xử lý trục gần song song ($\varepsilon = 10^{-5}$), đối xứng `testOBB(A,B) == testOBB(B,A)` |
| **Collision & Room Boundary** | [`CollisionSystem.hpp`](../include/roomcraft/systems/CollisionSystem.hpp), [`CollisionSystem.cpp`](../src/systems/CollisionSystem.cpp) | Pipeline 2 giai đoạn (AABB Broad-Phase → OBB SAT Narrow-Phase), kiểm tra 6 mặt biên phòng từ World AABB xoay, kiểm tra tăng dần $O(N)$, chính sách vật ẩn & thảm trải sàn |
| **JSON Engine (RFC 8259)** | [`Json.hpp`](../include/roomcraft/persistence/Json.hpp), [`Json.cpp`](../src/persistence/Json.cpp) | Bộ phân tích/xuất JSON nội bộ thuần C++17 (không thêm phụ thuộc ngoài), giới hạn 16 MB / độ sâu 64, phân biệt `int64`/`uint64`, từ chối `NaN`/ký tự thừa |
| **Project Data & Schema v1** | [`ProjectData.hpp`](../include/roomcraft/persistence/ProjectData.hpp), [`ProjectSerializer.hpp/cpp`](../src/persistence/ProjectSerializer.cpp), [`ProjectValidator.hpp/cpp`](../src/persistence/ProjectValidator.cpp) | Định dạng `.roomcraft` schema v1, `objectId` & `nextObjectId` dạng chuỗi thập phân lossless `uint64_t`, lưu đầy đủ Room, 14 Assets, Transforms, `shininessScale`, M7 Lighting, MSAA, Camera, Grid/Snap/Collision |
| **Safe Save & Transactional Load** | [`ProjectManager.hpp/cpp`](../src/persistence/ProjectManager.cpp), [`Scene.cpp`](../src/scene/Scene.cpp) | Ghi nguyên tử qua tệp tạm cùng thư mục + `MoveFileExW`; nạp giao dịch dựng `Scene` & `Camera` ứng viên trước khi commit; Dirty-State & Recent Projects (P1) |
| **Undo / Redo (P2)** | [`CommandHistory.hpp`](../include/roomcraft/systems/CommandHistory.hpp) | Lịch sử giới hạn 64 bước (1 lần kéo = 1 mục, xóa Redo khi sửa mới, dọn sạch khi đổi dự án) |
| **Win32 File Dialog & `WM_CLOSE`** | [`FileDialog.hpp/cpp`](../src/core/FileDialog.cpp), [`Window.hpp/cpp`](../src/core/Window.cpp) | Hộp thoại `GetOpenFileNameW`/`GetSaveFileNameW` hỗ trợ Unicode + khoảng trắng + `OFN_NOCHANGEDIR`; chặn nút `X` hệ điều hành để mở hộp thoại `Save / Discard / Cancel` khi dirty |
| **Editor & GPU Visuals** | [`RoomEditor.hpp/cpp`](../src/editor/RoomEditor.cpp), [`SceneRenderer.hpp/cpp`](../src/graphics/SceneRenderer.cpp), [`ViewportInteraction.hpp/cpp`](../src/editor/ViewportInteraction.cpp), [`Application.cpp`](../src/core/Application.cpp) | Menu `File` (`New/Open/Save/Save As/Recent`), `Edit` (`Undo/Redo`), `Collision & Placement` (`Off / Warning Only / Prevent Invalid`), khung cảnh báo hổ phách/đỏ san hô (giữ nguyên viền cyan chọn), cảnh báo chi tiết trong Inspector & Status Bar |

- **Hạng mục P2 hoãn sang M9**: `M8-F18` (*UI Layout Persistence*) được hoãn có chủ đích sang M9 để ưu tiên độ tin cậy của Collision và Project Persistence.

---

## 3. Kết quả Build, CTest & Ma trận Truy vết Yêu cầu (`M8-T01`..`M8-T32`)

- **Build**: `cmake --preset mingw-x64-debug -DFETCHCONTENT_UPDATES_DISCONNECTED=ON` + `cmake --build --preset mingw-x64-debug` → **Exit `0`, `0` errors, `0` project warnings** dưới `-Wall -Wextra -Wpedantic`.
- **CTest**: `ctest --test-dir d:\CG\build\mingw-x64-debug --output-on-failure` → **`17/17 PASS (100%, 6.65 s)`**.
- **Độc lập ngoài PATH WinLibs**: `RoomCraft3D.exe --help` (`v0.8.0`) và `RoomCraft3D.exe --m8-smoke-test` thoát mã `0`, `OpenGL errors: NONE`; `ntldd` chỉ link **8 DLL hệ thống Windows** (`ADVAPI32`, `COMDLG32`, `GDI32`, `KERNEL32`, `OPENGL32`, `SHELL32`, `USER32`, `WINMM`).

| Test ID | Yêu cầu | Bộ kiểm thử & Bằng chứng | Trạng thái |
|---|---|---|---|
| **M8-T01** | Baseline Regression | `14/14` CTest M0–M7 (`M0_SmokeTest`..`M7_LightingGpu`) đạt `PASS` trong đợt chạy `17/17` | **PASS** |
| **M8-T02** | AABB overlap & separation | `M8_CollisionUnitTests`: `M8-T02` (`depth=0.5 m`, separation & invalid rejection) | **PASS** |
| **M8-T03** | AABB touching behavior | `M8_CollisionUnitTests`: `M8-T03` (lamp on desk `y=0.75 m` & flush side contact -> `Touching`) | **PASS** |
| **M8-T04** | OBB SAT intersection | `M8_CollisionUnitTests`: `M8-T04` (3D rotated OBB penetration & exact symmetry `A,B == B,A`) | **PASS** |
| **M8-T05** | Rotated OBB false-positive rejection | `M8_CollisionUnitTests`: `M8-T05` (two 45° rotated planks: AABB overlaps, SAT separates) | **PASS** |
| **M8-T06** | Near-parallel SAT axes | `M8_CollisionUnitTests`: `M8-T06` (`0.0005° / 90° / 180°` finite MTV & stable classification) | **PASS** |
| **M8-T07** | Non-uniform scale & world bounds | `M8_CollisionUnitTests`: `M8-T07` (OBB matches `BoundingBox::transformed(M)`, rejects `0/neg/NaN`) | **PASS** |
| **M8-T08** | Room boundary validation | `M8_CollisionUnitTests`: `M8-T08` (rotated corner past East wall, elevated valid, floor/ceiling) | **PASS** |
| **M8-T09** | Room resize validation & policies | `M8_CollisionUnitTests`: `M8-T09` (shrink preserves all objects, reports out-of-bounds, hidden/rug rules) | **PASS** |
| **M8-T10** | Object ID & allocator restoration | `M8_PersistenceUnitTests`: `M8-T10` (lossless `9007199254740993`, new add gets `nextObjectId`) | **PASS** |
| **M8-T11..12** | JSON serialization / deserialization | `M8_PersistenceUnitTests`: `M8-T11/12` (valid schema v1, lossless string IDs, zero GPU handles) | **PASS** |
| **M8-T13..18** | 14-asset save/load round-trip (transforms, overrides, lighting, room, camera) | `M8_PersistenceUnitTests` (Unicode + spaces path) + `M8_IntegrationSmokeTest` (`M8-T14..18/30`) | **PASS** |
| **M8-T19** | Invalid JSON rejection | `M8_PersistenceUnitTests`: `M8-T19` (syntax error, trailing tokens, empty, `NaN`) | **PASS** |
| **M8-T20** | Unsupported schema rejection | `M8_PersistenceUnitTests`: `M8-T20` (`schemaVersion=99` & foreign format rejected) | **PASS** |
| **M8-T21** | Duplicate / invalid ID rejection | `M8_PersistenceUnitTests`: `M8-T21` (duplicate ID, `0` ID, `nextObjectId <= maxId` rejected) | **PASS** |
| **M8-T22** | Invalid numeric & transform rejection | `M8_PersistenceUnitTests`: `M8-T22` (negative scale, `NaN`, invalid room width, zero constant attenuation) | **PASS** |
| **M8-T23** | Missing asset handling | `M8_PersistenceUnitTests`: `M8-T23` (rejects `nonexistent-vintage-piano` with diagnostic naming asset) | **PASS** |
| **M8-T24** | Transactional load failure preserves active project | `M8_PersistenceUnitTests`: `M8-T24` (corrupt file leaves active scene, camera, dirty & name intact) | **PASS** |
| **M8-T25** | Save failure preserves previous valid project | `M8_PersistenceUnitTests`: `M8-T25` (read-only target & missing dir keep prior bytes byte-for-byte) | **PASS** |
| **M8-T26..27** | Dirty-state tracking & Save/Discard/Cancel semantics | `M8_PersistenceUnitTests`: `M8-T26/27` (failed save retains dirty; success clears dirty & records Recent) | **PASS** |
| **M8-T28** | Collision warning integration | `M8_IntegrationSmokeTest`: `M8-T28` (overlap + boundary warning rendered cleanly on FBO) | **PASS** |
| **M8-T29** | Picking & gizmo regression after load | `M8_IntegrationSmokeTest`: `M8-T29` (picking + Move Gizmo X hit immediately after `.roomcraft` load) | **PASS** |
| **M8-T30** | Repeated save/load & GPU resource lifetime | `M8_IntegrationSmokeTest`: `M8-T14..18/30` + `M8-T30-GL` (5x reload, GPU cache reused, `0` GL errors) | **PASS** |
| **M8-T31** | Optional Undo/Redo (P2) | `M8_PersistenceUnitTests`: `M8-T31` (Add/Move/Rotate/Scale/Delete, redo clear, cap=3 tested) | **PASS** |
| **M8-T32** | UI Integration & Manual Acceptance | Chưa được người dùng thao tác kiểm chứng trực tiếp trên cửa sổ tương tác | **NOT VERIFIED** |

---

## 4. Benchmark Hiệu năng Collision Detection (CPU, Build Debug)

Đo thực tế bằng `RoomCraft3D_CollisionUnitTests.exe` (2 vòng warm-up + 20 vòng đo full scene + 100 vòng đo kiểm tra tăng dần 1 vật thể `validateCandidateObject`):

| Số đối tượng (`N`) | Broad-Phase Pairs Tested | Narrow-Phase SAT Tests | Collisions Detected | Full Scene Avg Time | Incremental 1-Object Check ($O(N)$) |
|---:|---:|---:|---:|---:|---:|
| **30** | `435` | `10` | `6` | **`0.1191 ms`** | **`0.0451 ms`** |
| **100** | `4,950` | `6` | `6` | **`0.5097 ms`** | **`0.1702 ms`** |
| **250** *(stress)* | `31,125` | `303` | `99` | **`1.8744 ms`** | **`0.3709 ms`** |

- Toàn bộ phép kiểm tra va chạm chỉ chạy khi dữ liệu cảnh thay đổi (`m_collisionDirty`), nên khi cảnh đứng yên chi phí CPU mỗi khung hình là **`0 ms`**. Với 100 vật thể, ngay cả full scene validation chỉ mất `~0.51 ms` (< 3% ngân sách 16.67 ms của 60 Hz) và không cần cấu trúc spatial partitioning phức tạp.

---

## 5. Checklist Nghiệm thu Thủ công & Trực quan dành cho Người dùng (`M8-T32`)

Chạy trực tiếp `.\build\mingw-x64-debug\RoomCraft3D.exe`:

- [ ] **Collision Warning Only (mặc định)**: Kéo hai món đồ (ví dụ Sofa và Bed) đè lên nhau → xác nhận khung cảnh báo màu hổ phách xuất hiện, `Properties -> Dimensions & Placement` hiển thị đúng tên vật xung đột, Status Bar hiện số cảnh báo; đặt đồ lên thảm (`blender-carpet`) hoặc đèn lên mặt bàn (`y = 0.75 m`) **không** báo lỗi giả.
- [ ] **Room Boundary & Resize**: Kéo một món đồ thò ra khỏi tường → khung đỏ san hô + thông báo đúng phía tường (`East (+X)`...); thu nhỏ phòng trong `Room Settings` → xác nhận không tự xóa đồ và cảnh báo vượt biên cập nhật ngay.
- [ ] **Prevent Invalid Mode**: Chuyển `Validation Mode` sang `Prevent Invalid` → kéo đồ chồng lên đồ khác hoặc ra ngoài tường rồi thả chuột → xác nhận vật tự trả về vị trí hợp lệ trước đó kèm Toast cảnh báo (không mở popup chặn chuột).
- [ ] **Save / Save As / Open (`.roomcraft`)**: Chỉnh phòng, ánh sáng M7, `shininessScale`, đặt thêm đồ, lưu thành tệp có tên tiếng Việt & khoảng trắng → bấm `New Project` → mở lại tệp `.roomcraft` (hoặc qua `File -> Recent Projects`) → xác nhận mọi thông số (phòng, đồ, ánh sáng, camera, MSAA) khôi phục y hệt.
- [ ] **Dirty-State & Nút `X` cửa sổ**: Chỉnh sửa cảnh (dấu `*` xuất hiện trên thanh tiêu đề & góc phải menu) → bấm nút `X` cửa sổ Windows hoặc `File -> New Project` → kiểm tra đủ 3 nhánh **`Cancel`** (giữ nguyên), **`Save`** (lưu rồi mới thực hiện), **`Discard`** (bỏ thay đổi).
- [ ] **Undo / Redo (`Ctrl+Z` / `Ctrl+Y`)**: Thêm, di chuyển (1 lần kéo = 1 bước Undo), xoay, xóa đồ rồi bấm `Ctrl+Z` / `Ctrl+Y` xác nhận khôi phục đúng.
- [ ] *(Tồn đọng M6–M7)*: Hoàn tất các mục nghiệm thu thủ công/thị giác còn mở trong [`docs/M7_DELIVERY.md`](M7_DELIVERY.md).
