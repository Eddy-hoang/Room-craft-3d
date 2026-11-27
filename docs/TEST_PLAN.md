# Kế hoạch kiểm thử — RoomCraft 3D (Test Plan)

## 1. Chiến lược kiểm thử

Dự án áp dụng mô hình kiểm thử 3 tầng:
1. **Unit Testing thuần CPU**:
   - `RoomCraft3D_UnitTests` (`M1_UnitTests`): `Transform`, `Camera`, `MeshValidation`.
   - `RoomCraft3D_GeometryUnitTests` (`M2_GeometryUnitTests`): Primitives, Bézier Curve/Surface, Revolution Surface, BoundingBox, 6 Procedural Furniture models.
   - `RoomCraft3D_AssetUnitTests` (`M3_AssetUnitTests`): `ModelLoader`, `AssetNormalizer`, `FurnitureCatalog`, 8 Blender assets & fixtures.
   - `RoomCraft3D_SceneUnitTests` (`M4_SceneUnitTests`): `Room`, `RoomSettings`, `Scene`, `SceneObject`, `ObjectId`, `SelectionState`, `PlacementSystem`, `ScenePresets`.
2. **Integration, Runtime Smoke & Benchmark Testing**:
   - `--smoke-test`, `--m1-smoke-test`, `--m2-smoke-test`, `--m3-smoke-test`, `--m4-smoke-test`, `--m4-benchmark`.
3. **Manual GUI Testing**: Người dùng chạy trực tiếp `RoomCraft3D.exe` và xác nhận checklist trực quan.

## 2. Danh mục Test Case — Regression M0, M1, M2, M3

- **M0 (`TC-M0-01`..`TC-M0-07`)**, **M1 (`TC-M1-01`..`TC-M1-16`)**, **M2 (`TC-M2-01`..`TC-M2-23`)**, **M3 (`TC-M3-01`..`TC-M3-27`)**: Toàn bộ 7 bộ CTest trước phải tiếp tục đạt `PASS`.

## 3. Danh mục Test Case — Milestone M4 (Scene Management & Room Editor)

| Test ID | Hạng mục | Kịch bản kiểm thử | Kết quả mong đợi |
|---|---|---|---|
| **TC-M4-01** | Regression M0–M3 | Chạy lại 7 bộ CTest từ M0 đến M3 | Toàn bộ 7/7 bộ test cũ đạt `PASS` |
| **TC-M4-02** | Default Room Dimensions | Khởi tạo `Room` mặc định | `5.0 × 6.0 × 3.0 m`, tường `0.15 m`, sàn `0.10 m`, grid `0.50 m`, diện tích `30 m²`, thể tích `90 m³` |
| **TC-M4-03** | Room Interior Bounds | Gọi `Room::getInteriorBounds()` | `X ∈ [-2.5, 2.5]`, `Y ∈ [0, 3.0]`, `Z ∈ [-3.0, 3.0]` |
| **TC-M4-04** | Floor Geometry & `y = 0` | Kiểm tra BBox & `validate()` của `FloorMesh` | Mặt trên tại `y = 0`, mặt dưới tại `y = -0.10`, kích thước ngoài `5.30 × 6.30 m` |
| **TC-M4-05** | Wall Geometry & Dimensions | Kiểm tra BBox & `validate()` của 4 bức tường | Dựng ngoài biên sử dụng, cao `3.0 m`, tường Bắc/Nam phủ góc `5.30 m`, Đông/Tây dài `6.0 m`, không khe hở |
| **TC-M4-06** | Room Resize | Đổi màu/visibility rồi đổi kích thước sang `7×8×3.5 m` | Đổi màu không tăng `geometryRevision`; đổi kích thước tăng `geometryRevision` và cập nhật interior bounds |
| **TC-M4-07** | Invalid Room Parameters | Áp dụng width âm, height `0`, length `NaN` | Từ chối sạch, giữ nguyên kích thước & lưới hợp lệ trước đó |
| **TC-M4-08** | Wall Visibility | Kiểm tra `CutawayView` và `Manual` toggles | Ẩn/hiện đúng tường & trần theo chế độ, KHÔNG thay đổi interior bounds |
| **TC-M4-09** | Add Procedural Object | Thêm `procedural-desk` vào `Scene` | Trả về `ObjectId` hợp lệ, `objectCount` tăng 1 |
| **TC-M4-10** | Add Imported Object | Thêm `blender-gaming-chair` vào `Scene` | Trả về `ObjectId` hợp lệ, tham chiếu đúng `assetId` |
| **TC-M4-11** | Unique Object ID | Thêm & nhân bản nhiều vật thể, xóa rồi thêm tiếp | Mọi ID duy nhất, tăng đơn điệu, không tái sử dụng ID đã xóa |
| **TC-M4-12** | Remove Object | Xóa ID tồn tại và thử xóa lại / xóa ID không tồn tại | Xóa hợp lệ trả `true`, thao tác ID không tồn tại trả `false` không crash |
| **TC-M4-13** | Duplicate Object | Nhân bản đối tượng với offset `(0.5, 0, 0.5)` | Tạo ID mới, giữ `assetId`, vị trí dịch đúng offset |
| **TC-M4-14** | Rename Object | Đổi tên có khoảng trắng thừa và thử tên rỗng | Cắt khoảng trắng sạch, từ chối tên rỗng/ID không tồn tại |
| **TC-M4-15** | Object Visibility | Tắt `visible` của 1 đối tượng | `getVisibleObjectCount()` giảm, `getObjectCount()` giữ nguyên, giảm draw calls |
| **TC-M4-16** | Transform Independence | Xoay/scale ghế #1 trong 3 ghế cùng `assetId` | Ghế #2 và #3 giữ nguyên transform ban đầu |
| **TC-M4-17** | Invalid Transform Rejection | Đặt scale `0`, scale âm, position `NaN` | Từ chối sạch, không làm hỏng đối tượng |
| **TC-M4-18** | Invalid Asset ID Rejection | Thêm đối tượng với `assetId` không có trong catalog | Trả về `kInvalidObjectId`, không thêm rác vào `Scene` |
| **TC-M4-19** | SelectionState | Chọn, kiểm tra `isSelected`, xóa chọn | Hoạt động chính xác theo `ObjectId` |
| **TC-M4-20** | Delete Selected Safely | Xóa đối tượng đang chọn + gọi `reconcileWithScene()` | Selection tự xóa an toàn, không truy cập dữ liệu đã hủy |
| **TC-M4-21** | Shared GPU Asset Reuse | Render 3 `blender-gaming-chair` + desk + computer | `AssetManager` trả cùng `GpuAsset*`, `uniqueAssetsUsed == 3` cho 5 objects |
| **TC-M4-22** | Multi-Object Rendering | Render đồng thời phòng + 5 vật thể vào FBO | Đúng `roomDrawCalls`, `objectDrawCalls`, diện tích phủ pixel thực tế `> 5000 px` |
| **TC-M4-23** | Parent & Submesh Transforms | Đặt `Computer Setup` ở `y = 0.75` trên bàn và render | Kết hợp `M_object * M_asset_part` chính xác, an toàn sau `reloadAsset()` |
| **TC-M4-24** | Scene Preset Validation | Nạp & render cả 3 preset (`Bedroom`, `Gaming`, `Living`) | 3/3 preset hợp lệ (7 objects/preset, đặt vật đúng độ cao bàn, thảm lệch `3mm`), FBO phủ `> 5000 px` |
| **TC-M4-25** | Room Camera Fit | Gọi `Camera::fitBoundingBox()` cho `Room::getExteriorBounds()` | Hoạt động đúng ở cả Perspective và Top Orthographic |
| **TC-M4-26** | Clear Scene & Lifecycle | Gọi `clearObjects()` và kiểm tra render + cleanup | `objectDrawCalls == 0`, giải phóng GPU sạch không double-delete |
| **TC-M4-27** | Framebuffer Resize | Resize FBO sang `960×540` và vẽ lại scene | FBO complete, kích thước đúng, pixel coverage `> 5000 px` |
| **TC-M4-28** | 30-Object Benchmark Execution | Chạy benchmark 30 vật thể tại `1280×720` | Thực thi hoàn chỉnh, ghi nhận thời gian khung hình và FPS thực tế |
| **TC-M4-29** | OpenGL Errors & Safe Shutdown | Quét `glGetError()` và đóng cửa sổ | `OpenGL errors: NONE`, thoát mã `0` |
| **TC-M4-MANUAL** | Room Editor Manual GUI | Người dùng chạy `RoomCraft3D.exe` kiểm tra 12 kịch bản nghiệm thu M4 | Xác nhận trực quan bởi người dùng |
