# Kiến trúc phần mềm — RoomCraft 3D (Architecture)

## 1. Kiến trúc tổng thể theo tầng (Milestone M0–M6)

```mermaid
flowchart TD
    UI["Presentation Layer (Dear ImGui)\nProduct Menu Bar · RoomEditor (Toolbar, Thumbnail Library, Viewport, Hierarchy, Inspector, Status Bar, Toasts, Modals) · Tools (Geometry Lab, Asset Inspector, DevTools)"] --> APP["Application Layer\n(Application + Window + EditorTheme + EditorIcons + RoomEditor)"]
    APP --> GFX["Core Graphics & Runtime Layer (roomcraft_engine)\n(Renderer, SceneRenderer + Move Gizmo, ThumbnailRenderer, Shader, Mesh, TextureManager, AssetManager, Framebuffer)"]
    GFX --> SCENE["Scene, Interaction & UI-Logic Layer (roomcraft_scene - CPU only)\n(Scene, Room, SceneObject, SelectionState, PlacementSystem, CameraController, PickingSystem, TransformController, TransformGizmo, InputManager, ViewportInteraction, EditorLayout, NotificationSystem)"]
    SCENE --> ASSETS["Asset Pipeline Layer (roomcraft_assets - CPU only)\n(AssetData, ModelLoader [Assimp], AssetNormalizer, FurnitureCatalog)"]
    ASSETS --> GEO["Procedural Geometry Engine (roomcraft_geometry - CPU only)\n(MeshData, PrimitiveGenerator, BezierCurve/Surface, RevolutionSurface, FurnitureGenerator)"]
    GEO --> MATH["Math & Intersection Layer (roomcraft_math - CPU only)\n(Transform, Camera + Auto-Fit, BoundingBox, Ray, Plane, Slab/Plane/Triangle Intersection, Vertex)"]
    GFX --> MATH
    GFX --> GL["OpenGL 3.3 Core · GLSL 330 · GLAD · GLM · stb_image"]
    APP --> GLUT["FreeGLUT Window & Event Loop"]
```

## 2. Phân tách thư viện trong CMake (Acyclic Dependency Graph)

1. **`roomcraft_math` (CPU-only, chỉ phụ thuộc `GLM`)**:
   - `Transform`, `Camera` (`fitBoundingBox`), `BoundingBox`, `Ray`, `ViewportRect`, `Plane`, `Intersection` (Slab AABB, Ray–Plane, Möller–Trumbore Triangle), `Vertex`, `MeshValidation`.
   - Kiểm thử bởi `RoomCraft3D_UnitTests` (`M1_UnitTests`).
2. **`roomcraft_geometry` (CPU-only, phụ thuộc `roomcraft_math`)**:
   - `MeshData`, `GeometryUtils`, `PrimitiveGenerator`, `BezierCurve`, `BezierSurface`, `RevolutionSurface`, `FurnitureGenerator`.
   - Kiểm thử bởi `RoomCraft3D_GeometryUnitTests` (`M2_GeometryUnitTests`).
3. **`roomcraft_assets` (CPU-only, phụ thuộc `roomcraft_geometry` + `assimp`)**:
   - `AssetData`, `ModelLoader`, `AssetNormalizer`, `FurnitureCatalog`.
   - Kiểm thử bởi `RoomCraft3D_AssetUnitTests` (`M3_AssetUnitTests`).
4. **`roomcraft_scene` (CPU-only, phụ thuộc `roomcraft_assets`, KHÔNG phụ thuộc OpenGL/FreeGLUT/ImGui)**:
   - Quản lý cảnh M4: `ObjectId`, `SceneObject`, `RoomSettings`, `Room`, `Scene`, `SelectionState`, `PlacementSystem`, `ScenePresets`.
   - Tương tác M5: `InputManager`, `InteractionState`, `CameraController`, `PickingSystem`, `TransformController`, `ViewportInteraction`.
   - Logic UI & Gizmo M6: `EditorLayout`, `NotificationSystem`, `TransformGizmo`.
   - Kiểm thử bởi `RoomCraft3D_SceneUnitTests` (`M4_SceneUnitTests`), `RoomCraft3D_InteractionUnitTests` (`M5_InteractionUnitTests`) và `RoomCraft3D_EditorUiUnitTests` (`M6_EditorUiUnitTests`).
5. **`roomcraft_engine` (GPU + Window + ImGui, phụ thuộc `roomcraft_scene`, `stb_image`, `GLAD`, `FreeGLUT`, `ImGui`, `OpenGL`)**:
   - Graphics & Assets GPU: `Window`, `Shader`, `Mesh`, `Material`, `Texture`, `TextureManager`, `AssetManager`, `Framebuffer`, `ThumbnailRenderer`, `Renderer`, `SceneRenderer`.
   - Giao diện sản phẩm: `EditorTheme`, `EditorIcons`, `RoomEditor`, `Application`.
6. **`RoomCraft3D`**: Executable chính hỗ trợ 9 chế độ chạy:
   - Mặc định: **RoomCraft 3D — Interactive Room Editor** (menu sản phẩm `Tools` truy cập `Geometry Lab`, `Asset Inspector` và `Developer Tools` mà không mất trạng thái phòng).
   - `--smoke-test`, `--m1-smoke-test`, `--m2-smoke-test`, `--m3-smoke-test`, `--m4-smoke-test`, `--m4-benchmark`, `--m5-smoke-test`, `--m6-smoke-test`.

## 3. Luồng Định tuyến Đầu vào & Workspace Router

- FreeGLUT callbacks trong `Application` chuyển tiếp đầy đủ mọi sự kiện chuột/bàn phím tới `ImGui_ImplGLUT_*` đồng thời cập nhật `InputManager` (phân biệt `Pressed` 1-frame với `Held`).
- `RoomEditor::renderViewportPanel()` đo `ImGui::GetItemRectMin()` / `Max()` và `IsItemHovered()`, gọi `ViewportInteraction::processInput()` với ưu tiên **3-Axis Move Gizmo → Picking → Transform** trong cùng khung hình để Viewport, Hierarchy và Inspector đồng bộ tức thì.
- `Application::switchWorkspace()` lưu trữ riêng `m_roomCameraState` trước khi chuyển sang các công cụ trong menu `Tools` và khôi phục trọn vẹn khi quay về `Room Editor`.

Xem thêm chi tiết tại [`docs/DESIGN_SYSTEM.md`](DESIGN_SYSTEM.md), [`docs/UI_ARCHITECTURE.md`](UI_ARCHITECTURE.md), [`docs/CAMERA_SYSTEM.md`](CAMERA_SYSTEM.md), [`docs/OBJECT_PICKING.md`](OBJECT_PICKING.md) và [`docs/INTERACTION_GUIDE.md`](INTERACTION_GUIDE.md).

## Dòng dữ liệu M7

RoomEditor giữ LightingSettings theo phiên. Mỗi khung, SceneRenderOptions mang một bản giá trị tới SceneRenderer. Renderer gửi uniform nguồn sáng theo camera và gửi normal matrix, Ks, Ns theo submesh. AssetManager giữ mesh và texture dùng chung. Framebuffer sở hữu cả đích resolve và renderbuffer đa mẫu. ThumbnailRenderer dùng cùng shader với ánh sáng studio cố định. Xem [pipeline M7](RENDERING_PIPELINE.md).

## Kiến trúc M8 — Collision Detection & Project Persistence (`v0.8.0`)

- **`roomcraft_math` (CPU-only)**: Bổ sung `OBB` (`OBB.hpp/cpp`, `intersectAABB`, `ContactState`) và `SATIntersection` (`SATIntersection.hpp/cpp`, 15-axis 3D OBB SAT).
- **`roomcraft_scene` (CPU-only)**:
  - **Collision**: `CollisionSystem` & `RoomBoundaryValidator` (`CollisionSystem.hpp/cpp`) — kiểm tra 2 giai đoạn AABB Broad-Phase → OBB SAT Narrow-Phase, kiểm tra biên phòng 6 mặt, chính sách ẩn/thảm và kiểm tra tăng dần $O(N)$.
  - **Persistence**: `Json` (`Json.hpp/cpp`, RFC 8259 nội bộ có giới hạn kích thước/độ sâu), `ProjectData` (`ProjectData.hpp`), `ProjectValidator` (`ProjectValidator.hpp/cpp`), `ProjectSerializer` (`ProjectSerializer.hpp/cpp`, schema v1 `.roomcraft`, `ObjectId` chuỗi thập phân lossless), `ProjectManager` (`ProjectManager.hpp/cpp`, ghi nguyên tử qua tệp tạm + `MoveFileExW`, nạp giao dịch bảo toàn scene cũ, dirty-state & Recent Projects P1).
  - **Undo/Redo (P2)**: `CommandHistory` (`CommandHistory.hpp`, hàng đợi giới hạn 64 mục).
- **`roomcraft_engine` (Win32 + ImGui + OpenGL)**:
  - `FileDialog` (`FileDialog.hpp/cpp`, Win32 `GetOpenFileNameW`/`GetSaveFileNameW` hỗ trợ Unicode + `OFN_NOCHANGEDIR`), `Window` (chặn `WM_CLOSE` gọi hộp thoại Save/Discard/Cancel khi dirty), `SceneRenderer` (tô khung cảnh báo va chạm hổ phách / vượt biên phòng đỏ san hô mà vẫn giữ nguyên viền cyan chọn đối tượng), `RoomEditor` & `Application` (File menu `New/Open/Save/Save As/Recent`, `Edit -> Undo/Redo`, `Room Settings -> Collision & Placement`).
- Xem chi tiết tại [`docs/COLLISION_SYSTEM.md`](COLLISION_SYSTEM.md), [`docs/PROJECT_FORMAT.md`](PROJECT_FORMAT.md) và [`docs/PROJECT_PERSISTENCE.md`](PROJECT_PERSISTENCE.md).
