# RoomCraft 3D

**RoomCraft 3D** là ứng dụng desktop thiết kế và bố trí nội thất 3D viết bằng **C++17**, **Modern OpenGL 3.3 Core Profile**, **FreeGLUT**, **GLAD**, **GLM**, **GLSL**, **Dear ImGui**, **Assimp** và **stb_image**, kết hợp **Hybrid Modeling** (Procedural C++ + Blender Assets), **Scene Management** và **Interactive 3D Editing**.

> **Trạng thái hiện tại**: Đã hoàn thành triển khai & kiểm thử tự động **Milestone M5 — Camera, Object Picking & Interactive Transformation (`v0.5.0`)** (`11/11` CTest suites `PASS` trong `4.17s`; các kiểm thử tương tác GUI thủ công chờ người dùng xác nhận).

## Tính năng hiện có (M0–M5)

- **Core Graphics Engine (M0–M1)**: `Window`, `Shader`, `Vertex`, `Mesh` (VAO/VBO/EBO), `Transform`, `Camera` (Perspective/Orthographic + `fitBoundingBox()`), `Framebuffer` (`RGBA8 + DEPTH24_STENCIL8`), `Renderer`.
- **Procedural Geometry Engine (`roomcraft_geometry` — M2)**: 7 Primitives, 3 Parametric Generators (`BezierCurve`, `BezierSurface`, `RevolutionSurface`) và 6 mẫu nội thất Procedural thuần CPU.
- **Blender Asset Pipeline (`roomcraft_assets` — M3)**: 8 mẫu nội thất Blender 5.2.3 LTS (`.blend` + `.obj`/`.mtl` + 11 `.png` textures), `ModelLoader` (Assimp v6.0.6), `AssetNormalizer`, `TextureManager` (`stb_image` + fallback checkerboard), `FurnitureCatalog` (14 mẫu), `AssetManager` (GPU cache + transactional reload).
- **Scene Management & Room Editor (`roomcraft_scene` + `SceneRenderer` + `RoomEditor` — M4)**: `ObjectId` đơn điệu, `SceneObject`, `RoomSettings`, `Room` (sàn + 4 tường không khe hở + trần + lưới), `Scene`, `SelectionState`, `PlacementSystem`, 3 căn phòng mẫu (`Modern Bedroom`, `Gaming Workspace`, `Minimalist Living Room`), benchmark 10/30/100 vật thể.
- **Interactive Camera, Ray Casting & Transformation (M5)**:
  - **`CameraController`**: Orbit (chuột phải), Pan (chuột giữa), Zoom (con lăn chuột, hỗ trợ cả Perspective & Orthographic), đồng bộ hoàn toàn với Presets (`1/2/3/F/Home`).
  - **`PickingSystem`**: Screen-to-World Ray (Perspective & Orthographic, hỗ trợ DPI scaling), Ray–AABB Broad-phase (Slab Method), **Ray–Triangle Narrow-phase (Möller–Trumbore)** xuyên qua khe hở chân bàn/ghế, và **Occlusion-Aware Wall Policy** (tường hiện chặn chọn, tường ẩn Cutaway cho chọn xuyên qua).
  - **`TransformController` & `ViewportInteraction`**: Kéo thả trên mặt phẳng ngang giữ nguyên cao độ `Y` và `grabOffset`, xoay quanh `Y` (`R`, angle snap `15°`), phóng to/thu nhỏ an toàn (`S`), giới hạn trục (`X/Y/Z`), hủy thao tác bằng `Esc` khôi phục Transform ban đầu, `Locked` object protection và phím tắt `Q/G/R/S/F/Home/Delete/Ctrl+D/1/2/3`.

## Build và chạy nhanh trên Windows x64

```powershell
# Đưa WinLibs MinGW-w64 x64 (UCRT) lên đầu PATH của phiên PowerShell hiện tại
$winlibsBin = (Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*" -Directory | Select-Object -First 1).FullName + "\mingw64\bin"
$env:PATH = "$winlibsBin;$env:PATH"

# Configure & Build Debug
cmake --preset mingw-x64-debug
cmake --build --preset mingw-x64-debug

# Chạy toàn bộ 11 bộ kiểm thử tự động (M0–M5)
ctest --test-dir build\mingw-x64-debug --output-on-failure -V

# Chạy ứng dụng tương tác RoomCraft 3D (chạy độc lập, không cần PATH WinLibs)
.\build\mingw-x64-debug\RoomCraft3D.exe
```

## Tài liệu dự án

- [`AGENTS.md`](AGENTS.md) — Quy tắc kỹ thuật và chuẩn phát triển dự án
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — Kiến trúc phần mềm phân tầng
- [`docs/CAMERA_SYSTEM.md`](docs/CAMERA_SYSTEM.md) — Hệ thống Orbit/Pan/Zoom Camera (M5)
- [`docs/OBJECT_PICKING.md`](docs/OBJECT_PICKING.md) — Ray Casting, Slab Method, Möller–Trumbore & Wall Occlusion (M5)
- [`docs/INTERACTION_GUIDE.md`](docs/INTERACTION_GUIDE.md) — Phím tắt, thao tác chuột, Ray–Plane Drag & State Machine (M5)
- [`docs/SCENE_MANAGEMENT.md`](docs/SCENE_MANAGEMENT.md) & [`docs/ROOM_EDITOR_GUIDE.md`](docs/ROOM_EDITOR_GUIDE.md) — Scene Management & Room Editor (M4)
- [`docs/ASSET_PIPELINE.md`](docs/ASSET_PIPELINE.md) & [`docs/BLENDER_GUIDE.md`](docs/BLENDER_GUIDE.md) — Blender Asset Pipeline (M3)
- [`docs/GEOMETRY_ALGORITHMS.md`](docs/GEOMETRY_ALGORITHMS.md) & [`docs/GRAPHICS_THEORY.md`](docs/GRAPHICS_THEORY.md) — Toán học & thuật toán CG
- [`docs/ROADMAP.md`](docs/ROADMAP.md), [`docs/PROGRESS.md`](docs/PROGRESS.md), [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md), [`docs/TEST_RESULTS.md`](docs/TEST_RESULTS.md), [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md), [`docs/KNOWN_ISSUES.md`](docs/KNOWN_ISSUES.md), [`docs/ASSET_LICENSES.md`](docs/ASSET_LICENSES.md)
