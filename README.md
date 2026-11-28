# RoomCraft 3D

**RoomCraft 3D** là ứng dụng desktop thiết kế và bố trí nội thất 3D viết bằng **C++17**, **Modern OpenGL 3.3 Core Profile**, **FreeGLUT**, **GLAD**, **GLM**, **GLSL**, **Dear ImGui**, **Assimp** và **stb_image**, kết hợp **Hybrid Modeling** (Procedural C++ + Blender Assets), **Scene Management**, **Interactive 3D Editing**, **Blinn–Phong Lighting**, **3D OBB SAT Collision Detection** và **`.roomcraft` Project Persistence**.

**M9 `v1.0.0-rc1` (Release Candidate) đã hoàn tất kiểm thử tự động & đóng gói portable (`19/19` CTest PASS trên cả Debug và Release).** Bổ sung **Directional Shadow Mapping (`512/1024/2048` + 3×3 PCF)**, lưu bố cục UI (`PreferencesManager`), kiểm toán an toàn JSON/Persistence/Undo-Redo/Collision (kèm 1.500 ca fuzzing), biểu tượng ứng dụng Windows, 3 dự án mẫu `examples/*.roomcraft`, `12/12` ảnh GPU thật (`docs/evidence/m9/`), bộ tài liệu học thuật và gói phát hành portable `dist/RoomCraft3D-v1.0.0-rc1-win64.zip`. Chờ người dùng nghiệm thu thủ công cuối cùng trước khi gắn thẻ `v1.0.0` (xem [báo cáo M9](docs/M9_DELIVERY.md) và [Release Checklist](docs/RELEASE_CHECKLIST.md)).

## Tính năng hiện có (M0–M9)

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
- **Lighting, Materials & Visual Quality (M7)**:
  - Chiếu sáng **Blinn–Phong** (Ambient + Directional + Point Light suy giảm theo khoảng cách + Normal Matrix + Gamma Correction `2.2`), 4 preset ánh sáng (`Daylight`, `Warm Interior`, `Neutral Studio`, `Evening`), điều chỉnh `Shininess scale` theo từng đối tượng, lưới sàn khử răng cưa theo pixel (`fwidth`) và **MSAA `Off / 2x / 4x`**.
- **Collision Detection & Project Persistence (M8)**:
  - **Collision System (CPU-only)**: World AABB + World OBB từ TRS, **3D OBB SAT 15 trục** ổn định với góc song song, phân biệt `Touching` và `Penetrating`, kiểm tra biên phòng 6 mặt phẳng (hỗ trợ đặt nội thất trên mặt bàn và đổi kích thước phòng không phá hủy), 3 chế độ (`Off` / `Warning Only` / `Prevent Invalid`), quy tắc thảm (`Allow Rug Overlap`) và khung cảnh báo trực quan tách biệt màu selection.
  - **Project Persistence (`.roomcraft` JSON v1)**: Bộ phân tích JSON RFC-8259 nội bộ nghiêm ngặt, lưu/tải đầy đủ Phòng + Nội thất + ID `uint64_t` không mất độ chính xác + Ánh sáng M7 + MSAA + Camera + Cài đặt Editor, **Transactional Load** không phá dự án hiện tại khi lỗi, **Atomic Save** qua tệp tạm cùng thư mục + `MoveFileExW`, hộp thoại Win32 native (`comdlg32`, hỗ trợ Unicode/khoảng trắng, `OFN_NOCHANGEDIR`), theo dõi trạng thái Dirty (`*` + `Save / Discard / Cancel` kể cả khi đóng cửa sổ `WM_CLOSE`), **`Recent Projects`** (P1) và **`Undo/Redo`** tối đa 64 bước (P2).
- **Testing, Optimization, Shadow Mapping & Portable Packaging (M9 — `v1.0.0-rc1`)**:
  - **Directional Shadow Mapping (`ShadowMap` + `shadow_depth` GLSL 330)**: Độ phân giải `512 / 1024 / 2048`, lọc mềm 3×3 PCF, slope-scaled bias, mạch ván gỗ tinh tế & Corner AO (chỉ `+0.61 ms` trên Intel UHD Release, giữ `366.5 FPS` ở 30 vật thể).
  - **UI Layout Persistence & Polish**: `PreferencesManager` lưu bố cục panel, UI Scale, Grid/List và Recent Projects riêng tại `%APPDATA%\RoomCraft3D\preferences.json`; biểu tượng Windows `.ico`/`.rc`, sửa tiêu đề UTF-8 (`SetWindowTextW`), nạp Unicode toàn diện và 3 dự án mẫu `examples/*.roomcraft`.
  - **Release Build & Portable Distribution**: Preset `mingw-x64-release` (`7.59 MB`, `19/19` CTest PASS trong `3.56 s`), đóng gói `dist/RoomCraft3D-v1.0.0-rc1-win64.zip` kèm `SHA256SUMS.txt` và `licenses/THIRD_PARTY_NOTICES.txt`.

## Build và chạy nhanh trên Windows x64

```powershell
# Đưa WinLibs MinGW-w64 x64 (UCRT) lên đầu PATH của phiên PowerShell hiện tại
$winlibsBin = (Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*" -Directory | Select-Object -First 1).FullName + "\mingw64\bin"
$env:PATH = "$winlibsBin;$env:PATH"

# Configure & Build Release (hoặc mingw-x64-debug)
cmake --preset mingw-x64-release
cmake --build --preset mingw-x64-release

# Chạy toàn bộ 19 bộ kiểm thử tự động (M0–M9)
ctest --test-dir build\mingw-x64-release --output-on-failure -V

# Chạy ứng dụng tương tác RoomCraft 3D (chạy độc lập, không cần PATH WinLibs)
.\build\mingw-x64-release\RoomCraft3D.exe

# Đóng gói bản Portable Windows x64 (.zip + SHA256SUMS.txt)
powershell -ExecutionPolicy Bypass -File scripts\package_portable.ps1
```

## Tài liệu dự án

- [`AGENTS.md`](AGENTS.md) — Quy tắc kỹ thuật và chuẩn phát triển dự án
- [`docs/M9_DELIVERY.md`](docs/M9_DELIVERY.md), [`docs/FINAL_QA_REPORT.md`](docs/FINAL_QA_REPORT.md), [`docs/RELEASE_CHECKLIST.md`](docs/RELEASE_CHECKLIST.md) — Báo cáo M9 Release Candidate (`v1.0.0-rc1`), QA & Checklist phát hành
- [`docs/PERFORMANCE_REPORT.md`](docs/PERFORMANCE_REPORT.md) & [`docs/PACKAGING_GUIDE.md`](docs/PACKAGING_GUIDE.md) — Benchmark Debug/Release trên Intel UHD & Hướng dẫn đóng gói Portable
- [`docs/ACADEMIC_REPORT_GUIDE.md`](docs/ACADEMIC_REPORT_GUIDE.md) & [`licenses/THIRD_PARTY_NOTICES.txt`](licenses/THIRD_PARTY_NOTICES.txt) — Tài liệu báo cáo/slide/demo bảo vệ đồ án & Giấy phép thư viện bên thứ ba
- [`docs/COLLISION_SYSTEM.md`](docs/COLLISION_SYSTEM.md), [`docs/PROJECT_FORMAT.md`](docs/PROJECT_FORMAT.md), [`docs/PROJECT_PERSISTENCE.md`](docs/PROJECT_PERSISTENCE.md) — Hệ thống Va chạm 3D OBB SAT & Định dạng `.roomcraft` (M8)
- [`docs/M8_DELIVERY.md`](docs/M8_DELIVERY.md) — Báo cáo nghiệm thu Milestone M8 (`v0.8.0`)
- [`docs/M7_DELIVERY.md`](docs/M7_DELIVERY.md) — Báo cáo nghiệm thu Milestone M7 (`v0.7.0`)
- [`docs/DESIGN_SYSTEM.md`](docs/DESIGN_SYSTEM.md) — Hệ thống màu sắc Dark CAD, Typography & Vector Icons (M6)
- [`docs/UI_ARCHITECTURE.md`](docs/UI_ARCHITECTURE.md) — Kiến trúc bố cục, Splitters, ThumbnailRenderer & 3-Axis Move Gizmo (M6)
- [`docs/EDITOR_USER_GUIDE.md`](docs/EDITOR_USER_GUIDE.md) — Hướng dẫn sử dụng RoomCraft 3D Editor đầy đủ (M6–M8)
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — Kiến trúc phần mềm phân tầng
- [`docs/CAMERA_SYSTEM.md`](docs/CAMERA_SYSTEM.md), [`docs/OBJECT_PICKING.md`](docs/OBJECT_PICKING.md), [`docs/INTERACTION_GUIDE.md`](docs/INTERACTION_GUIDE.md) — Camera, Ray Casting & Interaction (M5)
- [`docs/SCENE_MANAGEMENT.md`](docs/SCENE_MANAGEMENT.md) & [`docs/ROOM_EDITOR_GUIDE.md`](docs/ROOM_EDITOR_GUIDE.md) — Scene Management & Room Editor (M4)
- [`docs/ASSET_PIPELINE.md`](docs/ASSET_PIPELINE.md) & [`docs/BLENDER_GUIDE.md`](docs/BLENDER_GUIDE.md) — Blender Asset Pipeline (M3)
- [`docs/GEOMETRY_ALGORITHMS.md`](docs/GEOMETRY_ALGORITHMS.md) & [`docs/GRAPHICS_THEORY.md`](docs/GRAPHICS_THEORY.md) — Toán học & thuật toán CG
- [`docs/ROADMAP.md`](docs/ROADMAP.md), [`docs/PROGRESS.md`](docs/PROGRESS.md), [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md), [`docs/TEST_RESULTS.md`](docs/TEST_RESULTS.md), [`docs/BUILD_GUIDE.md`](docs/BUILD_GUIDE.md), [`docs/KNOWN_ISSUES.md`](docs/KNOWN_ISSUES.md), [`docs/DECISION_LOG.md`](docs/DECISION_LOG.md), [`docs/SESSION_HANDOFF.md`](docs/SESSION_HANDOFF.md), [`docs/ASSET_LICENSES.md`](docs/ASSET_LICENSES.md)
