# RoomCraft 3D

**RoomCraft 3D** là ứng dụng desktop thiết kế và bố trí nội thất 3D viết bằng **C++17**, **Modern OpenGL 3.3 Core Profile**, **FreeGLUT**, **GLAD**, **GLM**, **GLSL**, **Dear ImGui**, **Assimp** và **stb_image**, theo kiến trúc **Hybrid Modeling** (Procedural Modeling bằng C++ kết hợp Blender Asset Pipeline) và **Scene Management** phân tầng thuần CPU.

> **Trạng thái hiện tại**: Đã hoàn thành triển khai & kiểm thử tự động **Milestone M4 — Scene Management & Room Editor (`v0.4.0`)** (`9/9` CTest suites `PASS` trong `2.76s`; benchmark 30 vật thể tại `1280×720` đạt `667.8 FPS` ở Debug trên Intel UHD Graphics; các kiểm thử tương tác GUI thủ công chờ người dùng xác nhận).

## Tính năng hiện có (M0 + M1 + M2 + M3 + M4)

- **Core Graphics Engine (M0–M1)**: `Window`, `Shader`, `Vertex`, `Mesh` (VAO/VBO/EBO), `Transform`, `Camera` (Perspective/Orthographic + `fitBoundingBox()`), `Framebuffer` (`RGBA8 + DEPTH24_STENCIL8`), `Renderer` (Solid/Wireframe/Textured + `RenderStats`).
- **Procedural Geometry Engine (`roomcraft_geometry` — M2)**: 7 Primitives, 3 Parametric Generators (`BezierCurve`, `BezierSurface`, `RevolutionSurface`) và 6 mẫu nội thất Procedural thuần CPU.
- **Blender Asset Pipeline (`roomcraft_assets` — M3)**: 8 mẫu nội thất Blender 5.2.3 LTS (`.blend` + `.obj`/`.mtl` + 11 `.png` textures), `ModelLoader` (Assimp v6.0.6), `AssetNormalizer`, `TextureManager` (`stb_image` + fallback checkerboard), `FurnitureCatalog` (14 mẫu), `AssetManager` (GPU cache + transactional reload).
- **Scene Management & Room Editor (`roomcraft_scene` + `SceneRenderer` + `RoomEditor` — M4)**:
  - **Thư viện thuần CPU `roomcraft_scene`**: `ObjectId` ổn định đơn điệu, `SceneObject`, `RoomSettings`, `Room` (tự sinh sàn, 4 tường không khe hở góc, trần tùy chọn, lưới sàn), `Scene`, `SelectionState`, `PlacementSystem` (hỗ trợ Grid Snapping), `ScenePresets`.
  - **3 Căn phòng mẫu dựng từ 14 asset độc lập**: `Modern Bedroom (5×6×3 m)`, `Gaming Workspace (4×5×3 m)`, `Minimalist Living Room (6×7×3 m)`.
  - **Wall Visibility System**: `All Walls`, `Hide Ceiling`, `Cutaway View` (góc nhìn dollhouse), `Manual` (bật/tắt từng bức tường Bắc/Nam/Tây/Đông & trần).
  - **Shared GPU Resources**: Nhiều `SceneObject` dùng chung một `GpuAsset` trong `AssetManager`, mỗi đối tượng có Transform và Visibility hoàn toàn độc lập.
  - **Giao diện 4 chế độ**: `Room Editor (M4)` (mặc định), `Asset Browser (M3)`, `Geometry Playground (M2)`, `M1 Reference Scene`.

## Build và chạy nhanh trên Windows x64

Xem hướng dẫn chi tiết tại [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md) và [`docs/ROOM_EDITOR_GUIDE.md`](docs/ROOM_EDITOR_GUIDE.md).

```powershell
# Đưa WinLibs MinGW-w64 x64 (UCRT) lên đầu PATH của phiên PowerShell hiện tại
$winlibsBin = (Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*" -Directory | Select-Object -First 1).FullName + "\mingw64\bin"
$env:PATH = "$winlibsBin;$env:PATH"

# Configure & Build Debug
cmake --preset mingw-x64-debug
cmake --build --preset mingw-x64-debug

# Chạy toàn bộ 9 bộ kiểm thử tự động (M0, M1, M2, M3, M4)
ctest --test-dir build\mingw-x64-debug --output-on-failure -V

# Chạy ứng dụng tương tác Room Editor (chạy độc lập, không cần PATH WinLibs)
.\build\mingw-x64-debug\RoomCraft3D.exe

# Chạy benchmark hiệu năng 10 / 30 / 100 vật thể tại 1280x720
.\build\mingw-x64-debug\RoomCraft3D.exe --m4-benchmark
```

## Tài liệu dự án

- [`AGENTS.md`](AGENTS.md) — Quy tắc kỹ thuật và chuẩn phát triển dự án
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — Kiến trúc phần mềm & sơ đồ phân tầng
- [`docs/SCENE_MANAGEMENT.md`](docs/SCENE_MANAGEMENT.md) — Kiến trúc Scene Management, Class/Sequence/Render Flow (M4)
- [`docs/ROOM_EDITOR_GUIDE.md`](docs/ROOM_EDITOR_GUIDE.md) — Hướng dẫn sử dụng Room Editor (M4)
- [`docs/ASSET_PIPELINE.md`](docs/ASSET_PIPELINE.md) & [`docs/BLENDER_GUIDE.md`](docs/BLENDER_GUIDE.md) — Blender Asset Pipeline (M3)
- [`docs/GEOMETRY_ALGORITHMS.md`](docs/GEOMETRY_ALGORITHMS.md) — Thuật toán Procedural & Parametric Geometry (M2)
- [`docs/GRAPHICS_THEORY.md`](docs/GRAPHICS_THEORY.md) — Kiến thức Computer Graphics áp dụng
- [`docs/ROADMAP.md`](docs/ROADMAP.md) & [`docs/PROGRESS.md`](docs/PROGRESS.md) — Lộ trình và tiến độ
- [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md) & [`docs/TEST_RESULTS.md`](docs/TEST_RESULTS.md) — Kế hoạch và kết quả kiểm thử thực tế
- [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md), [`docs/KNOWN_ISSUES.md`](docs/KNOWN_ISSUES.md), [`docs/ASSET_LICENSES.md`](docs/ASSET_LICENSES.md)
