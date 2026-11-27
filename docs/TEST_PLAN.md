# Kế hoạch kiểm thử — RoomCraft 3D (Test Plan)

## 1. Chiến lược kiểm thử

Dự án áp dụng mô hình kiểm thử 3 tầng với **11 bộ kiểm thử CTest**:
1. **Unit Testing thuần CPU** (`M1_UnitTests`, `M2_GeometryUnitTests`, `M3_AssetUnitTests`, `M4_SceneUnitTests`, `M5_InteractionUnitTests`): Kiểm thử độc lập toán học, sinh hình học, Assimp import, quản lý cảnh, tọa độ cầu camera, Ray Casting, Slab Method, Möller–Trumbore, Wall Occlusion, InputManager và TransformController không cần OpenGL context.
2. **GPU Integration & Runtime Smoke Testing** (`M0_SmokeTest`..`M5_IntegrationSmokeTest` + `--m4-benchmark`): Chạy trên ngữ cảnh OpenGL 3.3 Core thật.
3. **Manual GUI Testing**: Người dùng chạy trực tiếp `RoomCraft3D.exe` và xác nhận checklist trực quan.

## 2. Regression M0–M4 (`9/9` CTest)

Toàn bộ 9 bộ kiểm thử từ M0 đến M4 (`TC-M0-01`..`TC-M4-29`) phải tiếp tục đạt `PASS`.

## 3. Danh mục Test Case — Milestone M5 (Camera, Object Picking & Interactive Transformation)

| Test ID | Hạng mục | Kịch bản kiểm thử | Kết quả mong đợi |
|---|---|---|---|
| **TC-M5-01** | Regression M0–M4 | Chạy lại 9 bộ CTest từ M0 đến M4 | 9/9 PASS |
| **TC-M5-02** | Mouse Coord Conversion | Chuyển `(screenX, screenY)` sang tọa độ cục bộ viewport | Chính xác theo `rectMin` |
| **TC-M5-03** | Viewport-local → NDC | Ánh xạ tâm, góc trên-trái, góc dưới-phải sang OpenGL NDC `[-1, 1]` | Lật trục Y đúng (`top=+1`, `bottom=-1`) |
| **TC-M5-04** | Framebuffer / DPI Scaling | Display `800×600`, FBO `1600×1200` (2× DPI) | Tâm quy đổi đúng `(800, 600)` |
| **TC-M5-05** | Perspective Ray Gen | Phát tia từ tâm & cạnh phải viewport ở Perspective | Tâm đi thẳng `-Z`, cạnh phải hướng `+X` |
| **TC-M5-06** | Orthographic Ray Gen | Phát 2 tia ở 2 vị trí chuột khác nhau trong Top Ortho | Hướng tia song song (`dot=1`), gốc tia khác nhau theo chuột |
| **TC-M5-07** | Invalid Viewport / Matrix | Viewport 0×0, chuột ngoài vùng, ma trận suy biến, vector 0/NaN | Từ chối an toàn, trả `false` |
| **TC-M5-08** | Ray Normalization | Kiểm tra độ dài vector hướng sau `Ray::create` & `screenToWorldRay` | `|length(D) - 1| < 1e-3` |
| **TC-M5-09** | Ray–AABB Hit | Tia từ `(0,1,5)` hướng `-Z` vào hộp `[-1,1]×[0,2]×[-1,1]` | Trúng chính xác tại `t = 4.0`, `z = 1.0` |
| **TC-M5-10** | Ray–AABB Miss / Behind | Tia lệch ra ngoài & tia hướng ngược ra xa box | Không giao (`false`) |
| **TC-M5-11** | Parallel Ray | Tia song song trục nằm trong slab & ngoài slab | Trong slab trúng `t = 4.0`; ngoài slab trượt không chia 0 |
| **TC-M5-12** | Ray Inside Box | Gốc tia nằm tại tâm hộp `(0,1,0)` | Trúng với `startedInside = true`, `tHit = 0` |
| **TC-M5-13** | Negative Direction & Invalid Box | Tia hướng `(-1, 0, -1)` & hộp bao chưa khởi tạo | Hướng âm trúng đúng; hộp lỗi bị từ chối |
| **TC-M5-14** | Nearest Object Selection | Hai kệ sách thẳng hàng trước/sau dọc trục `-Z` | Chọn kệ gần hơn |
| **TC-M5-15** | Hidden Object Exclusion | Ẩn kệ phía trước rồi bắn tia cùng hướng | Bỏ qua kệ ẩn, chọn trúng kệ phía sau |
| **TC-M5-16** | Locked Object Policy | Khóa đối tượng rồi thử bắt đầu Rotate/Move/Delete | Cho chọn xem thông tin, chặn chỉnh sửa kèm cảnh báo |
| **TC-M5-17** | Empty-space Clearing | Bắn tia vào vùng trống | Trả `hit = false`, `kInvalidObjectId` |
| **TC-M5-18** | Rotated/Scaled Bounds | Bàn xoay `90° Y`, scale X `1.5×`, bắn tia vào phần kéo dài | Trúng đúng `ObjectId` |
| **TC-M5-19** | Camera Orbit | Orbit `(60, 20)` quanh target | Giữ nguyên bán kính & target, đổi vị trí trên cầu |
| **TC-M5-20** | Camera Pitch Limits | Orbit cực đại lên/xuống | Kẹp chặt trong `[-85°, +85°]`, không kỳ dị |
| **TC-M5-21** | Camera Pan | Pan `(120, -60)` | `position` và `target` dịch chuyển cùng vector |
| **TC-M5-22** | Perspective Zoom | Zoom vào/ra quá ngưỡng min/max | Kẹp trong `[0.5, 60.0] m`, clip planes hợp lệ |
| **TC-M5-23** | Orthographic Zoom | Zoom vào/ra trong Top Ortho | Thay đổi `orthoHalfHeight`, không scale cảnh |
| **TC-M5-24** | Camera Preset Sync | Chuyển Preset / Fit rồi Orbit nhẹ `1 px` | Không nhảy ngược về vị trí cũ |
| **TC-M5-25** | Focus Selected | Gọi `fitBoundingBox` vào AABB đối tượng | Target chuyển tới tâm đối tượng |
| **TC-M5-26** | Ray–Plane Intersection | Giao tiêu chuẩn, song song, gốc trên mặt phẳng, phía sau tia | Đúng tất cả 4 trường hợp |
| **TC-M5-27** | Drag Preserves Y & Offset | Kéo vật ở `Y = 0.75 m` với con trỏ lệch tâm | Giữ nguyên `Y = 0.75 m` và `grabOffset` |
| **TC-M5-28** | Grid Snapping | Kéo vật sang tọa độ âm `(-1.18, -0.77)` khi bật Snap `0.5 m` | Làm tròn đúng `(-1.0, 0.75, -1.0)` |
| **TC-M5-29** | Rotation Interaction | Xoay `90°` & `180°` với angle snap `15°` | Đúng góc quay quanh trục `Y` |
| **TC-M5-30** | Scaling Interaction | Kéo phóng to & kéo cực âm | Luôn dương, kẹp `[0.1, 10.0]` |
| **TC-M5-31** | Cancel Transform | Hủy khi đang kéo / xoay / scale (`Esc`) | Khôi phục nguyên `initialTransform`, về `Idle` |
| **TC-M5-32** | Delete Selected Safely | Nhấn `Delete` khi đang tương tác | Xóa object, xóa selection, đưa state về `Idle` |
| **TC-M5-33** | Duplicate Selection | Nhấn `Ctrl + D` và giữ phím qua frame sau | Nhân bản đúng 1 lần (không auto-repeat), chọn bản mới |
| **TC-M5-34** | Input Routing & GUI Capture | Nhấn `Delete`/`G` khi `uiWantsTextInput = true` | Không xóa object, không đổi tool |
| **TC-M5-35** | Selection / Inspector Sync | Chọn từ Viewport / Hierarchy / Duplicate / Delete | Dùng chung `SelectionState`, đồng bộ cùng frame |
| **TC-M5-36** | Viewport Resize & Picking | Resize FBO sang `960×540` ở Top Ortho và pick Computer Setup | Chọn trúng chính xác `ObjectId` |
| **TC-M5-37** | Invalid Interaction Recovery | Xóa/khóa đối tượng trong lúc đang biến đổi | Tự hủy tương tác về `Idle` an toàn |
| **TC-M5-38** | GPU Render & Highlight | Orbit camera & render Selected BBox cyan vào FBO | Xác nhận pixel cyan `> 20 px` |
| **TC-M5-39** | OpenGL Errors & Shutdown | Quét `glGetError()` và đóng cửa sổ | `OpenGL errors: NONE`, thoát mã `0` |
| **TC-M5-TRI** | Ray–Triangle Refinement (P1) | Bắn tia qua khoảng trống dưới gầm bàn | AABB trúng nhưng Ray–Triangle trượt sạch |
| **TC-M5-OCC** | Wall Occlusion Policy | Bắn tia từ ngoài tường Nam ở `CutawayView` vs `AllWalls` | Tường ẩn cho chọn; tường hiện chặn chọn (`occludedByWall=true`) |
| **TC-M5-MANUAL** | Interactive Editor GUI | Người dùng kiểm thử chuột/phím trực tiếp trên `RoomCraft3D.exe` | Xác nhận trực quan bởi người dùng |
