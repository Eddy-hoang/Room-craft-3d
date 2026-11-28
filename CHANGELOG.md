# Changelog — RoomCraft 3D

Tất cả các thay đổi đáng chú ý của dự án được ghi nhận trong file này theo chuẩn [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

## [0.8.0] - 2026-11-28 (Milestone M8: Collision Detection & Project Persistence)
### Added
- **Hệ thống Phát hiện Va chạm & Xác thực Biên phòng thuần CPU (`roomcraft_math` + `roomcraft_scene`)**:
  - `math::OBB` (`OBB.hpp/cpp`): dựng World-space OBB chính xác từ `BoundingBox` + `Transform` TRS (hỗ trợ xoay 3D & non-uniform scale, từ chối scale `<= 0`/NaN), chiếu `toWorldAABB()` và `intersectAABB()` phân biệt `Separated / Touching / Penetrating`.
  - `math::SATIntersection` (`SATIntersection.hpp/cpp`): kiểm tra giao nhau 3D OBB bằng **Separating Axis Theorem (15 trục: 3+3+9)**, xử lý trục gần song song ổn định số học và đảm bảo đối xứng.
  - `systems::CollisionSystem` & `systems::RoomBoundaryValidator` (`CollisionSystem.hpp/cpp`): pipeline 2 giai đoạn AABB Broad-Phase → OBB SAT Narrow-Phase, kiểm tra 6 mặt biên phòng từ hộp bao xoay, bảo toàn vật đặt nâng hợp lệ (`y = 0.75 m`), re-validate sau khi resize phòng mà không xóa/dịch đồ, kiểm tra tăng dần $O(N)$, chính sách loại trừ vật ẩn và cho phép đặt đồ trên thảm trải sàn.
- **Hệ thống Lưu/Mở Dự án `.roomcraft` & Undo/Redo (`roomcraft_scene` + `roomcraft_engine`)**:
  - `persistence::Json` (`Json.hpp/cpp`): bộ phân tích/xuất JSON RFC 8259 thuần C++17 (không thêm phụ thuộc ngoài, bảo vệ kích thước `16 MB` & độ sâu `64`, từ chối `NaN`/ký tự thừa).
  - `persistence::ProjectData`, `ProjectSerializer`, `ProjectValidator`, `ProjectManager` (`include/roomcraft/persistence/*`, `src/persistence/*`): định dạng `.roomcraft` **schema v1** độc lập app version, mã hóa lossless `uint64_t` `objectId`/`nextObjectId` dạng chuỗi thập phân, lưu toàn bộ phòng, 14 asset, transform, `visible`/`locked`, `shininessScale`, cấu hình ánh sáng M7, MSAA, camera, lưới/snap/chế độ va chạm.
  - **Ghi tệp an toàn nguyên tử** (tệp tạm cùng thư mục + `MoveFileExW` trên Windows, hỗ trợ Unicode & đường dẫn có khoảng trắng, không phá file cũ khi lỗi) và **Nạp giao dịch** (`Scene::restoreFromSnapshot`, bảo toàn 100% dự án đang mở khi file lỗi).
  - `core::FileDialog` (`FileDialog.hpp/cpp`, Win32 `GetOpenFileNameW`/`GetSaveFileNameW` với `OFN_NOCHANGEDIR`) và chặn `WM_CLOSE` trong `Window.cpp` để đồng bộ hộp thoại **`Save / Discard / Cancel`** khi đóng cửa sổ có thay đổi chưa lưu.
  - **Recent Projects (P1)** trong menu `File -> Recent Projects` và **Bounded `CommandHistory` Undo/Redo (P2 — 64 bước)** gắn phím `Ctrl+Z` / `Ctrl+Y`.
  - Tích hợp `RoomEditor`, `SceneRenderer` (tô khung cảnh báo hổ phách/đỏ san hô mà vẫn giữ viền cyan đối tượng chọn), `ViewportInteraction` (`Prevent Invalid` rollback) và `Application` (`File` menu, dấu `*` dirty, `--m8-smoke-test`).
- **Kiểm thử & Tài liệu M8**:
  - Bổ sung `M8_CollisionUnitTests`, `M8_PersistenceUnitTests`, `M8_IntegrationSmokeTest` nâng tổng số lên **17 bộ CTest**.
  - Tạo mới [`docs/COLLISION_SYSTEM.md`](docs/COLLISION_SYSTEM.md), [`docs/PROJECT_FORMAT.md`](docs/PROJECT_FORMAT.md), [`docs/PROJECT_PERSISTENCE.md`](docs/PROJECT_PERSISTENCE.md), [`docs/M8_DELIVERY.md`](docs/M8_DELIVERY.md) và cập nhật toàn bộ tài liệu dự án.

### Verified
- Build Debug thành công (`0` errors, `0` project warnings dưới `-Wall -Wextra -Wpedantic`); `RoomCraft3D.exe` chỉ link 8 DLL hệ thống Windows.
- Toàn bộ **`17/17` CTest suites đạt `PASS` trong `6.65 s`** (`M0`–`M8`), chạy độc lập ngoài PATH WinLibs (`--m8-smoke-test` thoát mã `0`, `OpenGL errors: NONE`).
- Benchmark Collision CPU (Debug): `30` vật thể `0.119 ms` (`0.045 ms` incremental), `100` vật thể `0.510 ms` (`0.170 ms` incremental), `250` vật thể `1.874 ms` (`0.371 ms` incremental).
- Kiểm thử thủ công & trực quan (`M8-T32` và các mục tồn đọng M6–M7) giữ trạng thái `NOT VERIFIED` chờ người dùng xác nhận.

## [0.7.0] - 2026-11-28 (M7, chờ nghiệm thu thủ công)

- Shader Blinn–Phong world space, bốn preset, diffuse texture sRGB và Material Inspector.
- Lưới giải tích theo mét, vân sàn, MSAA Off/2x/4x và resolve sang viewport ImGui.
- Cache thumbnail kiểm tra vòng đời asset bằng weak_ptr. Bổ sung kiểm thử pixel OpenGL và số đo GPU/editor cùng ảnh thực tế trong [báo cáo M7](docs/M7_DELIVERY.md).

## [0.6.0] - 2026-11-28 (Milestone M6: Professional Editor UI/UX)
### Added
- **Hệ thống Thiết kế Giao diện, Typography & Vector Icons (`roomcraft_engine`)**:
  - `editor::EditorTheme` (`EditorTheme.hpp/cpp`): bộ Design Tokens chuẩn Modern Dark CAD (`#171A20`, `#20242B`, `#292E38`, `#799CE3`, mã màu trục X/Y/Z, spacing 4–24 px), hỗ trợ `UI Scale 100% / 125% / 150%`.
  - Bổ sung font TrueType **`assets/fonts/Roboto-Medium.ttf`** (Apache License 2.0, `15 px` + dải glyph tiếng Việt) kèm cơ chế fallback an toàn về font mặc định của Dear ImGui.
  - `editor::EditorIcons` (`EditorIcons.hpp/cpp`): bộ 21+ biểu tượng vector tự vẽ qua `ImDrawList` (MIT), loại bỏ hoàn toàn rủi ro ô vuông Unicode vỡ và sắc nét ở mọi mức DPI.
- **Bố cục Đáp ứng, Thông báo & 3-Axis Move Gizmo thuần CPU (`roomcraft_scene`)**:
  - `editor::EditorLayout` (`EditorLayout.hpp/cpp`): tính toán bố cục không chồng lấn cho `1280×720..1920×1080` & `100..150%` UI scale, thanh kéo đổi kích thước dọc/ngang (`Splitters`) và `Reset Workspace Layout`.
  - `editor::NotificationSystem` (`NotificationSystem.hpp`): hệ thống Toast không chặn thao tác (tối đa 4 mục góc dưới-phải Viewport, tự ẩn sau `3.0 s`).
  - `systems::TransformGizmo` (`TransformGizmo.hpp/cpp`): **3-Axis Move Gizmo (P1)** với kích thước màn hình ổn định, hit-testing 3 trục X/Y/Z ưu tiên hơn chọn vật thể phía sau, tương thích đầy đủ với `TransformController`, khóa đối tượng `Locked` và hủy thao tác `Esc`.
- **GPU Thumbnail Renderer & Giao diện Sản phẩm Thống nhất (`roomcraft_engine`)**:
  - `graphics::ThumbnailRenderer` (`ThumbnailRenderer.hpp/cpp`): render ảnh xem trước FBO `256×256` thật trên GPU cho cả 14 mẫu nội thất (`~3.70 ms` tổng cho 14 mẫu), cache theo ID + asset token, tự động invalidate khi `Reload Asset` và bảo toàn toàn bộ trạng thái OpenGL.
  - Thiết kế lại toàn diện `editor::RoomEditor` (`RoomEditor.hpp/cpp`): Main Toolbar có biểu tượng vector & `Room Presets` dropdown, Furniture Library hỗ trợ **Grid ảnh thu nhỏ 2 cột** & **List View** kèm `+ Add Furniture`, 3D Viewport có pill overlay gọn gàng, Scene Hierarchy có nút icon **Eye/Lock** & **Context Menu chuột phải**, Properties Inspector thu gọn theo nhóm với ô Transform X/Y/Z mã màu, Status Bar sạch và hộp thoại xác nhận `Confirm Replace Scene` / `Confirm Delete Object`.
  - Thay thế hoàn toàn thanh tab milestone (`M1`–`M5`) trong `Application` bằng **Product Main Menu Bar** (`RoomCraft | File | Edit | View | Room | Tools | Help`), chuyển `Geometry Lab`, `Asset Inspector` và `Developer Tools` (`Graphics Reference Scene`, `OpenGL Diagnostics`, `Performance Monitor`) vào menu **`Tools`** đồng thời bảo toàn nguyên trạng `Scene`, `Camera` và `SelectionState` của `Room Editor`.
  - Bổ sung hộp thoại `Performance Monitor`, `OpenGL Diagnostics`, `Keyboard Shortcuts`, `User Guide`, `System Information`, `About RoomCraft`.
- **Kiểm thử & Tài liệu M6**:
  - Bổ sung `RoomCraft3D_EditorUiUnitTests` (`M6_EditorUiUnitTests`) và `M6_IntegrationSmokeTest` (`--m6-smoke-test`) nâng tổng số lên **13 bộ CTest**.
  - Tạo mới [`docs/DESIGN_SYSTEM.md`](docs/DESIGN_SYSTEM.md), [`docs/UI_ARCHITECTURE.md`](docs/UI_ARCHITECTURE.md), [`docs/EDITOR_USER_GUIDE.md`](docs/EDITOR_USER_GUIDE.md) và cập nhật toàn bộ tài liệu dự án (kèm bảng ánh xạ 39 test case M5 trong `TEST_RESULTS.md`).

### Verified
- Build Debug thành công (`0` errors, `0` project warnings dưới `-Wall -Wextra -Wpedantic`); `RoomCraft3D.exe` chỉ link 7 DLL hệ thống Windows.
- Toàn bộ **`13/13` CTest suites đạt `PASS` trong `2.90s`** (`M0`–`M6`); chạy độc lập ngoài PATH WinLibs từ thư mục làm việc khác (`C:\`) nạp đúng font/shader/14 mẫu và sinh `14/14` GPU FBO thumbnails trong `3.70 ms`, `OpenGL errors: NONE`.
- Đo hiệu năng M6 30 vật thể off-screen `1280×720` (Debug, Intel UHD): `1.68 ms (595.3 FPS)` với `417` draw calls (không phát sinh draw call dư so với M4).
- Kiểm thử tương tác GUI & kiểm tra thị giác thủ công (`TC-M6-MANUAL` và các manual test M0–M5) giữ trạng thái `NOT VERIFIED` chờ người dùng xác nhận.

## [0.5.0] - 2026-11-28 (Milestone M5: Camera, Object Picking & Interactive Transformation)
### Added
- Mở rộng thư viện toán học thuần CPU **`roomcraft_math`**:
  - `math::ViewportRect` & `math::Ray` (`Ray.hpp`): ánh xạ tọa độ chuột → viewport cục bộ → DPI/framebuffer → OpenGL NDC `[-1, 1]` và `Ray::screenToWorldRay()` giải chiếu Near/Far thống nhất cho cả **Perspective** và **Orthographic**.
  - `math::Plane` (`Plane.hpp`) & `math::Intersection` (`Intersection.hpp/cpp`): **Ray–AABB Slab Method**, **Ray–Plane Intersection** và **Möller–Trumbore Ray–Triangle Intersection**.
- Mở rộng thư viện thuần CPU **`roomcraft_scene`**:
  - `core::InputManager` (`InputManager.hpp/cpp`): phân biệt sự kiện 1-frame (`Pressed`/`Released`) với `Held`, chống lặp phím auto-repeat.
  - `systems::InteractionState` (`InteractionState.hpp`): máy trạng thái tương tác đơn luồng (`Idle`, `Orbiting`, `Panning`, `Moving`, `Rotating`, `Scaling`) + `AxisConstraint` (`X/Y/Z`).
  - `systems::CameraController` (`CameraController.hpp/cpp`): Orbit Camera tọa độ cầu (clamp pitch `[-85°, +85°]`), Pan theo `right`/`trueUp`, Zoom cho cả Perspective & Orthographic, tự động `syncFromCamera()` sau mọi preset và `Fit Room` / `Fit Selected`.
  - `systems::PickingSystem` (`PickingSystem.hpp/cpp`): Picking 2 giai đoạn (**Ray–AABB Broad-phase** + **Möller–Trumbore Ray–Triangle Narrow-phase**) kết hợp **Wall Occlusion** (tường hiện chặn tia, tường ẩn Cutaway không chặn tia), bỏ qua hidden objects và tôn trọng chính sách locked objects.
  - `systems::TransformController` (`TransformController.hpp/cpp`): kéo thả trên mặt phẳng ngang giữ nguyên `grabOffset` và cao độ `Y`, Grid Snapping tính từ target chưa snap (không tích lũy sai số), xoay quanh `Y` kèm angle snap `15°`, phóng to/thu nhỏ dương kẹp `[0.1, 10.0]`, hủy thao tác (`Esc`) khôi phục nguyên `initialTransform`.
  - `editor::ViewportInteraction` (`ViewportInteraction.hpp/cpp`): định tuyến đầu vào chuột/phím tắt an toàn khỏi Dear ImGui (`WantTextInput`, Popup, active widgets, viewport image rect).
- Nâng cấp `RoomEditor`, `Application` & `main.cpp`:
  - Thanh công cụ `Sel(Q)`, `Move(G)`, `Rotate(R)`, `Scale(S)`, checkbox `Locked` trong Inspector, Status Bar hiển thị chế độ tương tác, trục khóa và tọa độ thế giới con trỏ `(X, Z)`.
  - Phím tắt `Q/G/R/S`, `X/Y/Z`, `F`, `Home`, `Delete`, `Ctrl+D`, `1/2/3`, `Esc` (ưu tiên hủy thao tác, không đóng app đột ngột).
  - Bổ sung bộ CPU Unit Test `RoomCraft3D_InteractionUnitTests` (`M5_InteractionUnitTests`, 30 checks) và `M5_IntegrationSmokeTest` (`--m5-smoke-test`) vào CTest.
- Tạo mới [`docs/CAMERA_SYSTEM.md`](docs/CAMERA_SYSTEM.md), [`docs/OBJECT_PICKING.md`](docs/OBJECT_PICKING.md), [`docs/INTERACTION_GUIDE.md`](docs/INTERACTION_GUIDE.md) và cập nhật toàn bộ tài liệu dự án (đồng thời đối chiếu đúng bằng chứng OBJ import của `TC-M3-02` trong `TEST_RESULTS.md`).

### Verified
- Build Debug thành công (`0` errors, `0` project warnings dưới `-Wall -Wextra -Wpedantic`); `RoomCraft3D.exe` chỉ link 7 DLL hệ thống Windows.
- Toàn bộ **`11/11` CTest suites đạt `PASS` trong `4.17s`** (`M0`–`M5`), chạy độc lập ngoài PATH WinLibs (`M5_UNIT_EXIT=0`, `M5_SMOKE_EXIT=0`), `OpenGL errors: NONE`.
- Kiểm thử tương tác GUI thủ công (`TC-M5-MANUAL` và các manual test M0–M4) giữ trạng thái `NOT VERIFIED` chờ người dùng xác nhận.

## [0.4.0] - 2026-11-28 (Milestone M4: Scene Management & Room Editor)
### Added
- Tạo thư viện tĩnh thuần CPU **`roomcraft_scene`** (KHÔNG phụ thuộc OpenGL/FreeGLUT/ImGui):
  - `scene::ObjectId` (`ObjectId.hpp`): định danh `uint64_t` ổn định, tăng đơn điệu, không dùng vector index và không tái sử dụng ID đã xóa.
  - `scene::SceneObject` (`SceneObject.hpp`): tách biệt instance trong phòng khỏi `GpuAsset` dùng chung; tính `getWorldBounds()` từ 8 góc AABB.
  - `scene::RoomSettings` (`RoomSettings.hpp`) & `scene::Room` (`Room.hpp/cpp`): tự sinh sàn (`y = 0`), 4 bức tường dựng ngoài biên sử dụng không khe hở góc, trần tùy chọn và lưới sàn từ `PrimitiveGenerator`; transactional rebuild (đổi màu/visibility không rebuild mesh); 4 chế độ ẩn tường (`AllWalls`, `HideCeiling`, `CutawayView`, `Manual`).
  - `scene::Scene` (`Scene.hpp/cpp`): quản lý `Room` và danh sách `SceneObject` (`addObject`, `removeObject`, `duplicateObject`, `findObject`, `renameObject`, `setObjectVisibility`, `setObjectTransform`, `clearObjects`, kiểm tra sơ bộ `isBoundsWithinRoom`).
  - `systems::SelectionState` (`SelectionState.hpp`) & `systems::PlacementSystem` (`PlacementSystem.hpp/cpp`): quản lý đối tượng đang chọn theo `ObjectId`, đặt nội thất vào phòng và hỗ trợ Grid Snapping (`snapToGrid`).
  - `scene::ScenePresets` (`ScenePresets.hpp/cpp`): 3 căn phòng mẫu (`Modern Bedroom 5×6×3m`, `Gaming Workspace 4×5×3m`, `Minimalist Living Room 6×7×3m`) dựng từ 14 asset hiện có + bộ dựng cảnh benchmark 10 / 30 / 100 vật thể.
- Mở rộng tầng GPU & giao diện (`roomcraft_engine`):
  - `graphics::SceneRenderer` (`SceneRenderer.hpp/cpp`): đồng bộ lưới phòng theo `geometryRevision`, render nhiều đối tượng dùng chung `GpuAsset` từ `AssetManager` theo `M_world_part = M_object * M_asset_part`, thống kê tách biệt Unique GPU vs Submitted/Frame.
  - `editor::RoomEditor` (`RoomEditor.hpp/cpp`): giao diện Room Editor hoàn chỉnh (Toolbar, Furniture Library, Scene Hierarchy, 3D Room Viewport, Properties Inspector, Room Settings, Status Bar, hộp thoại xác nhận khi đổi preset).
  - Nâng cấp `Application` & `main.cpp` hỗ trợ chế độ mặc định **M4 Room Editor** (giữ nguyên M3/M2/M1), `--m4-smoke-test` và `--m4-benchmark`.
- Bổ sung bộ CPU Unit Test `RoomCraft3D_SceneUnitTests` (`M4_SceneUnitTests`) và `M4_IntegrationSmokeTest` vào CTest.
- Khóa bổ sung `URL_HASH SHA256` cho archive FetchContent của Assimp `v6.0.6` và `stb` trong `CMakeLists.txt`, đồng thời sửa chính xác commit SHA của tag Assimp `v6.0.6` (`a79ff6bbce54d3e01186a918dc6374ed55317426`) và thống kê `11` texture PNG trong tài liệu.
- Tạo mới [`docs/SCENE_MANAGEMENT.md`](docs/SCENE_MANAGEMENT.md), [`docs/ROOM_EDITOR_GUIDE.md`](docs/ROOM_EDITOR_GUIDE.md) và cập nhật toàn bộ tài liệu dự án.

### Verified
- Build Debug thành công (`0` errors, `0` project warnings dưới `-Wall -Wextra -Wpedantic`); `RoomCraft3D.exe` chỉ link 7 DLL hệ thống Windows.
- Toàn bộ **`9/9` CTest suites đạt `PASS` trong `2.76s`** (`M0`–`M4`), `OpenGL errors: NONE`.
- Benchmark thực tế `--m4-benchmark` (Debug, `1280×720`, Intel UHD Graphics, off-screen FBO + `glFinish()`): 10 objects `0.68 ms (1,470.8 FPS)`, 30 objects `1.50 ms (667.8 FPS)`, 100 objects `5.03 ms (198.7 FPS)` với chỉ `6` unique GPU assets được cache.
- Kiểm thử tương tác GUI thủ công (`TC-M4-MANUAL` và các manual test M0–M3) giữ trạng thái `NOT VERIFIED` chờ người dùng xác nhận.

## [0.3.0] - 2026-11-27 (Milestone M3: Blender Asset Pipeline)
### Added
- Tích hợp **Assimp `v6.0.6`** (static, chỉ bật OBJ importer, tắt exporters/tests/tools) và **`stb_image`** (`6b5799ba...`) qua CMake `FetchContent` khóa commit archive (kèm cập nhật `vcpkg.json` và tự động thiết lập `CMAKE_TLS_CAINFO` cho WinLibs CMake trên Windows).
- Tạo thư viện tĩnh thuần CPU **`roomcraft_assets`** (chỉ phụ thuộc `roomcraft_geometry` và `assimp`, KHÔNG phụ thuộc OpenGL/FreeGLUT/ImGui):
  - `assets::AssetData` (`AssetData.hpp`): biểu diễn CPU thống nhất (`SubMeshData`, `MaterialData`, `BoundingBox`) cho cả nội thất Procedural và Blender.
  - `assets::ModelLoader` (`ModelLoader.hpp/cpp`): nạp `.obj`/`.mtl` qua Assimp, đệ quy cây node, bake ma trận node lên vị trí và $(M^{-1})^T$ lên pháp tuyến, chuẩn hóa đường dẫn texture tương đối.
  - `assets::AssetNormalizer` (`AssetNormalizer.hpp/cpp`): chuẩn hóa tỷ lệ đơn vị (`unitScale`) và quy chiếu pivot về **tâm đáy (`min.y = 0`, tâm XZ = `0`)**.
  - `assets::FurnitureCatalog` (`FurnitureCatalog.hpp/cpp`): đăng ký hợp nhất **14 mẫu nội thất** (6 Procedural `procedural-*` + 8 Blender `blender-*`) theo stable ID kèm tìm kiếm/lọc.
- Mở rộng tầng GPU `roomcraft_engine`:
  - `graphics::Material` (`Material.hpp`), `graphics::Texture` (`Texture.hpp/cpp`, `stbi_set_flip_vertically_on_load(1)`, mipmap, RAII move-only), `graphics::TextureManager` (`TextureManager.hpp/cpp`, cache tái sử dụng handle + fallback checkerboard tím–đen khi thiếu file).
  - `assets::AssetManager` (`AssetManager.hpp/cpp`): cache `GpuAsset` theo ID và **transactional reload** (giữ nguyên asset cũ nếu reload thất bại).
  - `graphics::Camera::fitBoundingBox()` (`Camera.hpp/cpp`): tự động căn khung hình cho cả Perspective và Orthographic.
  - Nâng cấp `assets/shaders/mesh.frag` (`uDiffuseMap`, `uHasTexture`) và `Renderer::drawMeshTextured()`.
- Thiết kế và xuất **8 mô hình nội thất Blender thực tế** bằng 10 script `bpy` (`blender/scripts/*.py`) chạy trên **Blender 5.2.3 LTS**:
  - `Modern Sofa`, `Modern Bed`, `Gaming Chair`, `Computer Setup`, `Indoor Plant`, `Floor Lamp`, `Wardrobe`, `Minimalist Carpet` (đủ `.blend` trong `blender/source/` và `.obj`/`.mtl`/`.png` trong `assets/models/`).
- Nâng cấp `Application` với **M3 Furniture Asset Browser** (bố cục 3 cột `Asset Library` · `3D Asset Preview` · `Asset Inspector`, tìm kiếm/lọc, `Fit Model`, `Reload Asset`, Wireframe/BoundingBox/Normals overlay), đồng thời giữ nguyên menu chuyển sang **M2 Geometry Playground** và **M1 Reference Scene**.
- Bổ sung fixtures kiểm thử (`tests/fixtures/`), bộ CPU Unit Test `RoomCraft3D_AssetUnitTests` (`M3_AssetUnitTests`) và `M3_IntegrationSmokeTest` (`--m3-smoke-test`).
- Tạo mới 3 tài liệu chuyên sâu: [`docs/ASSET_PIPELINE.md`](docs/ASSET_PIPELINE.md), [`docs/BLENDER_GUIDE.md`](docs/BLENDER_GUIDE.md), [`blender/README.md`](blender/README.md) và cập nhật toàn bộ tài liệu dự án.

### Verified
- Build Debug thành công (`0` errors, `0` project warnings dưới `-Wall -Wextra -Wpedantic`); `RoomCraft3D.exe` chỉ link 7 DLL hệ thống Windows.
- Toàn bộ **`7/7` CTest suites đạt `PASS` trong `1.45s`** (`M0_SmokeTest`, `M1_UnitTests`, `M1_IntegrationSmokeTest`, `M2_GeometryUnitTests`, `M2_IntegrationSmokeTest`, `M3_AssetUnitTests`, `M3_IntegrationSmokeTest`), `OpenGL errors: NONE`.
- Kiểm thử tương tác GUI thủ công (`TC-M3-MANUAL` và các manual test M0–M2) giữ trạng thái `NOT VERIFIED` chờ người dùng xác nhận.

## [0.2.0] - 2026-11-27 (Milestone M2: Procedural Geometry & Modeling)
### Added
- Tạo thư viện tĩnh thuần CPU `roomcraft_geometry` (không phụ thuộc OpenGL/FreeGLUT/ImGui):
  - `math::BoundingBox` (`BoundingBox.hpp/cpp`): tính AABB từ đỉnh, hợp nhất và biến đổi 8 góc bằng ma trận 4×4.
  - `geometry::MeshData` (`MeshData.hpp/cpp`) và `geometry::GeometryUtils` (`GeometryUtils.hpp/cpp`): dữ liệu mesh CPU kèm kiểm tra pháp tuyến đơn vị, UV hữu hạn, chỉ mục hợp lệ và chống tam giác suy biến; sinh overlay BoundingBox và Vertex Normals.
  - `geometry::PrimitiveGenerator` (`PrimitiveGenerator.hpp/cpp`): tự sinh thuật toán cho `Plane`, `Cube`, `Cuboid`, `Cylinder`, `Sphere`, `Cone`, `Frustum` với pivot tâm đáy (`y = 0`), winding CCW, pháp tuyến chuẩn hóa và UV seam.
  - `geometry::BezierCurve` (`BezierCurve.hpp/cpp`): Cubic Bézier (Bernstein + de Casteljau + đạo hàm bậc nhất + control polygon).
  - `geometry::BezierSurface` (`BezierSurface.hpp/cpp`): Bicubic Bézier Surface `4×4` (đạo hàm riêng `dS/du`, `dS/dv`, pháp tuyến bề mặt, control net).
  - `geometry::RevolutionSurface` (`RevolutionSurface.hpp/cpp`): mặt tròn xoay quanh trục `+Y` từ profile 2D hoặc Bézier Curve, xử lý cực `r = 0` và nắp phẳng.
  - `geometry::FurnitureGenerator` (`FurnitureGenerator.hpp/cpp`): mô hình hóa phân cấp (`M_world_part = M_parent * M_local_part`) cho 6 mẫu nội thất: `Desk`, `Chair`, `Bookshelf`, `Desk Lamp`, `Vase` (Revolution) và `Curved Chair` (Bicubic Bézier Surface).
- Nâng cấp `Renderer` hỗ trợ `drawMeshWithMatrix()` và `setWireframeMode()` tự khôi phục `GL_FILL` tại `endFrame()`.
- Nâng cấp `Application` thành **M2 Geometry Playground** (chọn danh mục/loại, chỉnh tham số, auto/manual regenerate, Solid/Wireframe, Grid/Axes/BoundingBox/Control Net/Vertex Normals overlay, thống kê thực tế) và bổ sung chế độ `--m2-smoke-test`.
- Bổ sung bộ Unit Test thuần CPU `RoomCraft3D_GeometryUnitTests` (`M2_GeometryUnitTests`) và `M2_IntegrationSmokeTest` trong CTest.
- Tạo tài liệu học thuật [`docs/GEOMETRY_ALGORITHMS.md`](docs/GEOMETRY_ALGORITHMS.md) giải thích thuật toán kèm sơ đồ Mermaid và mã giả.

### Verified
- Build Debug thành công (`24/24` targets, `0` errors, `0` project warnings).
- Toàn bộ `5/5` CTest suites đạt `PASS` trong `3.24s` (`M0_SmokeTest`, `M1_UnitTests`, `M1_IntegrationSmokeTest`, `M2_GeometryUnitTests`, `M2_IntegrationSmokeTest`).
- Kiểm thử tương tác GUI thủ công (`TC-M2-MANUAL`) giữ trạng thái `NOT VERIFIED` chờ người dùng xác nhận.

## [0.1.0] - 2026-11-27 (Milestone M1: Core Graphics Engine)
### Added
- Tích hợp `GLM 1.0.1`, tách `Window` khỏi `Application`, triển khai `Shader`, `Vertex`, `Mesh`, `Transform`, `Camera`, `Framebuffer`, `Renderer`, demo 3 cube + Grid/XYZ Axes, `M1 Diagnostics`, unit tests và GPU integration smoke test.

## [0.0.1] - 2026-11-27 (Milestone M0: Project Foundation)
### Added
- Khởi tạo cấu trúc dự án, CMake C++17, FreeGLUT `v3.8.0`, GLAD `v0.1.36`, Dear ImGui `v1.93.0-docking`, render tam giác OpenGL 3.3 Core và `M0_SmokeTest`.
