# RoomCraft 3D

**RoomCraft 3D** là ứng dụng desktop thiết kế và bố trí nội thất 3D viết bằng **C++17**, **Modern OpenGL 3.3 Core Profile**, **FreeGLUT**, **GLAD**, **GLM**, **GLSL**, **Dear ImGui**, **Assimp** và **stb_image**, kết hợp **Hybrid Modeling** (Procedural C++ + Blender Assets), **Scene Management**, **Interactive 3D Editing** và **Professional CAD Editor UI/UX**.

> **Trạng thái hiện tại**: Đã hoàn thành triển khai & kiểm thử tự động **Milestone M6 — Professional Editor UI/UX (`v0.6.0`)** (**`13/13` CTest suites `PASS` trong `2.90s`**; các kiểm thử tương tác GUI và đánh giá thị giác thủ công chờ người dùng xác nhận).

## Tính năng hiện có (M0–M6)

- **Core Graphics Engine (M0–M1)**: `Window`, `Shader`, `Vertex`, `Mesh` (VAO/VBO/EBO), `Transform`, `Camera` (Perspective/Orthographic + `fitBoundingBox()`), `Framebuffer` (`RGBA8 + DEPTH24_STENCIL8`), `Renderer`.
- **Procedural Geometry Engine (`roomcraft_geometry` — M2)**: 7 Primitives, 3 Parametric Generators (`BezierCurve`, `BezierSurface`, `RevolutionSurface`) và 6 mẫu nội thất Procedural thuần CPU.
- **Blender Asset Pipeline (`roomcraft_assets` — M3)**: 8 mẫu nội thất Blender 5.2.3 LTS (`.blend` + `.obj`/`.mtl` + 11 `.png` textures), `ModelLoader` (Assimp v6.0.6), `AssetNormalizer`, `TextureManager` (`stb_image` + fallback checkerboard), `FurnitureCatalog` (14 mẫu), `AssetManager` (GPU cache + transactional reload).
- **Scene Management & Room Editor (`roomcraft_scene` + `SceneRenderer` + `RoomEditor` — M4)**: `ObjectId` đơn điệu, `SceneObject`, `RoomSettings`, `Room` (sàn + 4 tường không khe hở + trần + lưới), `Scene`, `SelectionState`, `PlacementSystem`, 3 căn phòng mẫu (`Modern Bedroom`, `Gaming Workspace`, `Minimalist Living Room`), benchmark 10/30/100 vật thể.
- **Interactive Camera, Ray Casting & Transformation (M5)**:
  - **`CameraController`**: Orbit (chuột phải), Pan (chuột giữa), Zoom (con lăn chuột, hỗ trợ cả Perspective & Orthographic), đồng bộ hoàn toàn với Presets (`1/2/3/F/Home`).
  - **`PickingSystem`**: Screen-to-World Ray (Perspective & Orthographic, hỗ trợ DPI scaling), Ray–AABB Broad-phase (Slab Method), **Ray–Triangle Narrow-phase (Möller–Trumbore)** xuyên qua khe hở chân bàn/ghế, và **Occlusion-Aware Wall Policy**.
  - **`TransformController` & `ViewportInteraction`**: Kéo thả trên mặt phẳng ngang giữ nguyên cao độ `Y` và `grabOffset`, xoay quanh `Y` (`R`, angle snap `15°`), phóng to/thu nhỏ an toàn (`S`), khóa trục (`X/Y/Z`), hủy thao tác (`Esc`), bảo vệ vật thể `Locked` và phím tắt `Q/G/R/S/F/Home/Delete/Ctrl+D/1/2/3`.
- **Professional Editor UI/UX & 3-Axis Move Gizmo (M6)**:
  - **Product Main Menu Bar**: `RoomCraft | File | Edit | View | Room | Tools | Help` — loại bỏ hoàn toàn các tab milestone; gom `Geometry Lab`, `Asset Inspector` và `Developer Tools` (`Graphics Reference Scene`, `OpenGL Diagnostics`, `Performance Monitor`) vào menu `Tools` và bảo toàn nguyên trạng cảnh phòng + camera khi chuyển công cụ.
  - **Modern Dark CAD Design System**: Bộ Design Tokens tại `EditorTheme`, font TrueType **`Roboto-Medium.ttf`** (Apache 2.0, `15 px` + tiếng Việt + fallback an toàn), **UI Scale `100/125/150%`** và bộ **21+ Vector Icons** vẽ qua `ImDrawList`.
  - **GPU Asset Thumbnail Renderer**: Render FBO `256×256` thật trên GPU cho cả **14 mẫu nội thất** (`~3.70 ms` tổng cho 14 mẫu), cache theo ID + asset token, tự động làm mới khi `Reload Asset`.
  - **Responsive Resizable Layout & Workspace Polish**: Thanh kéo chia panel (`EditorLayout`), Furniture Library dạng **Grid ảnh thu nhỏ 2 cột / List View**, Scene Hierarchy có biểu tượng **Eye / Lock** & **Context Menu chuột phải**, Properties Inspector thu gọn theo nhóm với ô Transform **X/Y/Z mã màu**, Status Bar sạch, **Toast Notifications** và **Confirmation Modals**.
  - **3-Axis Move Gizmo (P1)**: Tay cầm 3 trục Đỏ/Xanh lá/Xanh dương kích thước màn hình ổn định, ưu tiên hit-test trước picking cảnh, hỗ trợ khóa đối tượng và `Esc` khôi phục.

## Build và chạy nhanh trên Windows x64

```powershell
# Đưa WinLibs MinGW-w64 x64 (UCRT) lên đầu PATH của phiên PowerShell hiện tại
$winlibsBin = (Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*" -Directory | Select-Object -First 1).FullName + "\mingw64\bin"
$env:PATH = "$winlibsBin;$env:PATH"

# Configure & Build Debug
cmake --preset mingw-x64-debug
cmake --build --preset mingw-x64-debug

# Chạy toàn bộ 13 bộ kiểm thử tự động (M0–M6)
ctest --test-dir build\mingw-x64-debug --output-on-failure -V

# Chạy ứng dụng tương tác RoomCraft 3D (chạy độc lập, không cần PATH WinLibs)
.\build\mingw-x64-debug\RoomCraft3D.exe
```

## Tài liệu dự án

- [`AGENTS.md`](AGENTS.md) — Quy tắc kỹ thuật và chuẩn phát triển dự án
- [`docs/DESIGN_SYSTEM.md`](docs/DESIGN_SYSTEM.md) — Hệ thống màu sắc Dark CAD, Typography & Vector Icons (M6)
- [`docs/UI_ARCHITECTURE.md`](docs/UI_ARCHITECTURE.md) — Kiến trúc bố cục, Splitters, ThumbnailRenderer & 3-Axis Move Gizmo (M6)
- [`docs/EDITOR_USER_GUIDE.md`](docs/EDITOR_USER_GUIDE.md) — Hướng dẫn sử dụng RoomCraft 3D Editor đầy đủ (M6)
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — Kiến trúc phần mềm phân tầng
- [`docs/CAMERA_SYSTEM.md`](docs/CAMERA_SYSTEM.md), [`docs/OBJECT_PICKING.md`](docs/OBJECT_PICKING.md), [`docs/INTERACTION_GUIDE.md`](docs/INTERACTION_GUIDE.md) — Camera, Ray Casting & Interaction (M5)
- [`docs/SCENE_MANAGEMENT.md`](docs/SCENE_MANAGEMENT.md) & [`docs/ROOM_EDITOR_GUIDE.md`](docs/ROOM_EDITOR_GUIDE.md) — Scene Management & Room Editor (M4)
- [`docs/ASSET_PIPELINE.md`](docs/ASSET_PIPELINE.md) & [`docs/BLENDER_GUIDE.md`](docs/BLENDER_GUIDE.md) — Blender Asset Pipeline (M3)
- [`docs/GEOMETRY_ALGORITHMS.md`](docs/GEOMETRY_ALGORITHMS.md) & [`docs/GRAPHICS_THEORY.md`](docs/GRAPHICS_THEORY.md) — Toán học & thuật toán CG
- [`docs/ROADMAP.md`](docs/ROADMAP.md), [`docs/PROGRESS.md`](docs/PROGRESS.md), [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md), [`docs/TEST_RESULTS.md`](docs/TEST_RESULTS.md), [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md), [`docs/KNOWN_ISSUES.md`](docs/KNOWN_ISSUES.md), [`docs/ASSET_LICENSES.md`](docs/ASSET_LICENSES.md)
