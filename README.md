# RoomCraft 3D

**RoomCraft 3D** là ứng dụng desktop thiết kế và bố trí nội thất 3D viết bằng **C++17**, **Modern OpenGL 3.3 Core Profile**, **FreeGLUT**, **GLAD**, **GLM**, **GLSL**, **Dear ImGui**, **Assimp** và **stb_image**, theo kiến trúc **Hybrid Modeling** kết hợp giữa **Procedural Modeling** bằng C++ và **Blender Asset Pipeline**.

> **Trạng thái hiện tại**: Đã hoàn thành triển khai & kiểm thử tự động **Milestone M3 — Blender Asset Pipeline (`v0.3.0`)** (`7/7` CTest suites `PASS` trong `1.45s`; các kiểm thử tương tác GUI thủ công chờ người dùng xác nhận).

## Tính năng hiện có (M0 + M1 + M2 + M3)

- **Nền tảng OpenGL 3.3 Core + Core Graphics Engine (M0–M1)**: `Window`, `Shader`, `Vertex`, `Mesh` (VAO/VBO/EBO), `Transform`, `Camera` (Perspective/Orthographic + `fitBoundingBox()` Auto-Fit), `Framebuffer` (off-screen `RGBA8 + DEPTH24_STENCIL8`), `Renderer` (Solid/Wireframe/Textured + `RenderStats` thực tế).
- **Procedural Geometry Engine thuần CPU (`roomcraft_geometry` — M2)**:
  - Độc lập hoàn toàn với OpenGL/FreeGLUT/ImGui.
  - **7 Primitives**: `Plane`, `Cube`, `Cuboid`, `Cylinder`, `Sphere`, `Cone`, `Frustum`.
  - **3 Parametric Generators**: `BezierCurve`, `BezierSurface` (4×4), `RevolutionSurface`.
  - **6 mẫu nội thất Procedural (`FurnitureGenerator`)**: `Procedural Desk`, `Procedural Chair`, `Procedural Bookshelf`, `Procedural Desk Lamp`, `Procedural Ceramic Vase`, `Procedural Curved Chair`.
- **Blender Asset Pipeline & Texture System (`roomcraft_assets` + `roomcraft_engine` — M3)**:
  - **8 mẫu nội thất Blender thực tế** sinh qua tập lệnh `bpy` (`blender/scripts/*.py`) chạy trên **Blender 5.2.3 LTS**, lưu kèm file nguồn `.blend` (`blender/source/`) và xuất chuẩn hóa `.obj` + `.mtl` + `.png` (`assets/models/`): `Modern Sofa`, `Modern Bed`, `Gaming Chair`, `Computer Setup`, `Indoor Plant`, `Floor Lamp`, `Wardrobe`, `Minimalist Carpet`.
  - **`ModelLoader` (Assimp v6.0.6, OBJ-only, thuần CPU)**: Đệ quy cây node Assimp, bake biến đổi node bằng $M_{\text{world}}$ cho vị trí và ma trận nghịch đảo chuyển vị $(M^{-1})^T$ cho pháp tuyến, chuẩn hóa đường dẫn texture tương đối.
  - **`AssetNormalizer`**: Đồng bộ đơn vị mét, quy chiếu pivot về **tâm đáy (`min.y = 0`, tâm XZ = `0`)**, tính lại AABB thống nhất.
  - **`Texture` & `TextureManager` (`stb_image`)**: Lật ảnh theo trục Y (`stbi_set_flip_vertically_on_load(1)`), mipmap, cache tái sử dụng texture trên GPU, fallback checkerboard tím–đen khi thiếu file ảnh.
  - **`FurnitureCatalog` & `AssetManager`**: Hợp nhất **14 món nội thất** (6 Procedural + 8 Blender) theo stable ID (`procedural-*`, `blender-*`), cache GPU asset và **transactional reload** (giữ nguyên asset cũ khi reload gặp lỗi).
- **Giao diện 3 chế độ (Dear ImGui)**:
  - **M3 Furniture Asset Browser** (mặc định): Bố cục 3 cột (`Asset Library` tìm kiếm/lọc + `3D Asset Preview` có `Fit Model` tự động + `Asset Inspector` hiển thị thống kê thực tế, BoundingBox, Wireframe, Normals, `Reload Asset`).
  - **M2 Geometry Playground** & **M1 Reference Scene**: Chuyển đổi trực tiếp trên thanh menu, bảo toàn toàn bộ tính năng cũ.

## Build và chạy nhanh trên Windows x64

Xem hướng dẫn chi tiết tại [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md) và hướng dẫn Blender tại [`docs/BLENDER_GUIDE.md`](docs/BLENDER_GUIDE.md).

```powershell
# Đưa WinLibs MinGW-w64 x64 (UCRT) lên đầu PATH của phiên PowerShell hiện tại
$winlibsBin = (Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*" -Directory | Select-Object -First 1).FullName + "\mingw64\bin"
$env:PATH = "$winlibsBin;$env:PATH"

# Configure & Build Debug
cmake --preset mingw-x64-debug
cmake --build --preset mingw-x64-debug

# Chạy toàn bộ 7 bộ kiểm thử tự động (M0, M1, M2, M3)
ctest --test-dir build\mingw-x64-debug --output-on-failure -V

# Chạy ứng dụng tương tác M3 Furniture Asset Browser (chạy độc lập, không cần PATH WinLibs)
.\build\mingw-x64-debug\RoomCraft3D.exe
```

## Tài liệu dự án

- [`AGENTS.md`](AGENTS.md) — Quy tắc kỹ thuật và chuẩn phát triển dự án
- [`docs/PROJECT_SCOPE.md`](docs/PROJECT_SCOPE.md) — Phạm vi và phân loại ưu tiên P0/P1/P2
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — Kiến trúc phần mềm & sơ đồ lớp/pipeline
- [`docs/ASSET_PIPELINE.md`](docs/ASSET_PIPELINE.md) — Kiến trúc và toán học Blender Asset Pipeline (M3)
- [`docs/BLENDER_GUIDE.md`](docs/BLENDER_GUIDE.md) & [`blender/README.md`](blender/README.md) — Quy chuẩn xuất asset và script `bpy` trên Blender 5.2.3 LTS
- [`docs/GEOMETRY_ALGORITHMS.md`](docs/GEOMETRY_ALGORITHMS.md) — Giải thích thuật toán Procedural & Parametric Geometry (M2)
- [`docs/GRAPHICS_THEORY.md`](docs/GRAPHICS_THEORY.md) — Kiến thức Computer Graphics áp dụng
- [`docs/ROADMAP.md`](docs/ROADMAP.md) — Lộ trình 10 milestone
- [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md) — Hướng dẫn cài môi trường và biên dịch trên Windows
- [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md) & [`docs/TEST_RESULTS.md`](docs/TEST_RESULTS.md) — Kế hoạch và kết quả kiểm thử thực tế
- [`docs/PROGRESS.md`](docs/PROGRESS.md) — Theo dõi tiến độ từng task
- [`docs/KNOWN_ISSUES.md`](docs/KNOWN_ISSUES.md) — Danh sách lỗi và giới hạn hiện tại
- [`docs/ASSET_LICENSES.md`](docs/ASSET_LICENSES.md) — Nguồn gốc và giấy phép thư viện/tài nguyên
