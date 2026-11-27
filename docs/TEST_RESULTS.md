# Kết quả kiểm thử thực tế — RoomCraft 3D (Test Results)

Cập nhật lần cuối: **2026-11-28**

## 1. Cấu hình máy kiểm thử

- **OS**: Microsoft Windows 11 Home Single Language 64-bit (`10.0.26200`)
- **GPU**: `Intel(R) UHD Graphics` (Vendor: `Intel`, Driver: `32.0.101.7088`)
- **OpenGL Runtime**: `3.3.0 - Build 32.0.101.7088` (`Core Profile`), GLSL `3.30 - Build 32.0.101.7088`
- **Toolchain**: `x86_64-w64-mingw32-g++ 16.2.0` (WinLibs UCRT), `CMake 4.4.4`, `Ninja 1.13.2`, `GLM 1.0.1`, `Assimp v6.0.6`, `stb_image v2.34`
- **CTest tổng hợp**: `ctest --test-dir build/mingw-x64-debug --output-on-failure -V` → **`11/11 Passed (100%, 4.17 sec)`**

---

## 2. Kết quả kiểm thử Milestone M5 (Camera, Object Picking & Interactive Transformation)

| Test ID | Hạng mục | Kết quả thực tế & Bằng chứng | Trạng thái |
|---|---|---|---|
| **TC-M5-01** | Regression M0–M4 | Toàn bộ `9/9` CTest từ M0 đến M4 đạt `PASS` trong đợt chạy `11/11` CTest (`4.17s`) | **PASS** |
| **TC-M5-02..04** | Mouse → Viewport → NDC & 2× DPI | `M5_InteractionUnitTests`: `[Ray] TC-M5-02/03` & `TC-M5-04 Framebuffer vs display size (2x DPI)` PASS | **PASS** |
| **TC-M5-05..08** | Perspective/Ortho Ray, Invalid & Norm | `M5_InteractionUnitTests`: `[Ray] TC-M5-05/08`, `TC-M5-06` (song song + gốc riêng), `TC-M5-07` PASS | **PASS** |
| **TC-M5-09..13** | Ray–AABB Slab Method | `M5_InteractionUnitTests`: `TC-M5-09` (`t=4.0`), `TC-M5-10`, `TC-M5-11` (parallel in/out), `TC-M5-12` (`startedInside, tHit=0`), `TC-M5-13` PASS | **PASS** |
| **TC-M5-14..18** | Picking (Nearest, Hidden, Locked, Empty, Rot/Scale) | `M5_InteractionUnitTests`: `TC-M5-14`, `TC-M5-15`, `TC-M5-16`, `TC-M5-17`, `TC-M5-18` + `M5_IntegrationSmokeTest` (`Bookshelf` pick & hidden exclusion) PASS | **PASS** |
| **TC-M5-TRI** | Ray–Triangle Refinement (P1) | `M5_InteractionUnitTests`: `[Intersection] Möller-Trumbore` + `[Picking] Ray-Triangle refinement passes cleanly through hollow space under desk` PASS | **PASS** |
| **TC-M5-OCC** | Wall Occlusion Policy | `M5_InteractionUnitTests`: `[Picking] Occlusion-aware picking: hidden cutaway wall allows picking, visible wall blocks picking` PASS | **PASS** |
| **TC-M5-19..25** | Orbit, Pitch Clamp, Pan, Zoom, Preset Sync, Focus | `M5_InteractionUnitTests`: `[CameraController]` 6/6 kiểm tra (`TC-M5-19..25`, clamp `[-85, +85]`, không nhảy sau Fit) PASS | **PASS** |
| **TC-M5-26** | Ray–Plane Intersection | `M5_InteractionUnitTests`: `[Intersection] TC-M5-26` (hit, parallel, origin-on-plane, behind-origin) PASS | **PASS** |
| **TC-M5-27..31** | Drag Y/Offset, Grid Snap, Rotate, Scale, Cancel | `M5_InteractionUnitTests`: `TC-M5-27` (`Y=0.75m`), `TC-M5-28` (tọa độ âm), `TC-M5-29/31` (`90/180°`, `15°` snap, Cancel restore), `TC-M5-30` (`[0.1, 10.0]`) + `M5_IntegrationSmokeTest` (`TC-M5-27/31`) PASS | **PASS** |
| **TC-M5-32..35/37** | Delete, Ctrl+D, GUI Capture, Sync, Recovery | `M5_InteractionUnitTests`: `TC-M5-32..35/37` (chặn phím khi `uiWantsTextInput`, chống auto-repeat `Ctrl+D`, xóa an toàn khi đang tương tác) PASS | **PASS** |
| **TC-M5-36** | Viewport Resize & Ortho Picking | `M5_IntegrationSmokeTest`: `[TC-M5-06/36] Orthographic Top View & viewport resize (960x540) picking on Computer Setup: PASS` | **PASS** |
| **TC-M5-38** | GPU Orbit Render & Cyan Highlight | `M5_IntegrationSmokeTest`: `[TC-M5-38] Post-orbit FBO render & selected object cyan BoundingBox highlight pixels verified: PASS` | **PASS** |
| **TC-M5-39** | OpenGL Errors & Standalone Exit | `M5_IntegrationSmokeTest`: 10 frames (`97 draws, 4256 tris`), `OpenGL errors: NONE`; chạy độc lập ngoài PATH WinLibs: `M5_UNIT_EXIT=0`, `M5_SMOKE_EXIT=0`; `ntldd` chỉ 7 DLL hệ thống Windows | **PASS** |
| **TC-M5-MANUAL** | Interactive Editor Manual GUI | Chưa được người dùng chạy kiểm chứng tương tác chuột/phím trực tiếp | **NOT VERIFIED** |

---

## 3. Kết quả Benchmark Hiệu năng M4 (`RoomCraft3D.exe --m4-benchmark`)

- **Cấu hình đo**: Build **`Debug`** (`mingw-x64-debug`), GPU **`Intel(R) UHD Graphics`**, FBO **`1280×720`**, đo off-screen FBO đồng bộ **`glFinish()`** (VSync: `N/A`), Warm-up: `5 frames`, Measured: `30 frames` (10 & 30 objects) / `20 frames` (100 objects). *(Phản ánh hiệu năng render off-screen của bài benchmark hiện tại, không phải FPS end-to-end khi bật thêm ánh sáng/bóng đổ ở M7).*

| Kịch bản | Visible Objects | Unique Assets (Cache) | Unique GPU Stored (Verts / Tris) | Submitted / Frame (Draws / Verts / Tris) | Avg Frame Time | Avg FPS |
|---|---:|---:|---|---:|---:|---:|
| **10 objects** | 10 | 6 (6 assets, 6 tex) | `7,121 v / 4,968 t` | `163 draws / 12,597 v / 8,100 t` | **0.68 ms** | **1,470.8 FPS** |
| **30 objects** | 30 | 6 (6 assets, 6 tex) | `7,121 v / 4,968 t` | `417 draws / 35,759 v / 24,876 t` | **1.50 ms** | **667.8 FPS** |
| **100 objects** *(stress)* | 100 | 6 (6 assets, 6 tex) | `7,121 v / 4,968 t` | `1,393 draws / 119,488 v / 82,620 t` | **5.03 ms** | **198.7 FPS** |

---

## 4. Tóm tắt trạng thái M0–M4 (Giữ nguyên `NOT VERIFIED` đối với các manual test chưa xác nhận đủ)

- **M0 (`TC-M0-01`..`TC-M0-07`)**: **7/7 PASS** (tự động). `TC-M0-MSVC`: **NOT RUN**. `TC-M0-MANUAL`: **NOT VERIFIED**.
- **M1 (`TC-M1-01`..`TC-M1-16`)**: **16/16 PASS** (tự động). `TC-M1-MANUAL`: **NOT VERIFIED**.
- **M2 (`TC-M2-01`..`TC-M2-23`)**: **23/23 PASS** (tự động). `TC-M2-MANUAL`: **NOT VERIFIED**.
- **M3 (`TC-M3-01`..`TC-M3-27`)**: **27/27 PASS** (tự động). `TC-M3-MANUAL`: **NOT VERIFIED**.
  - *Bằng chứng chính xác cho `TC-M3-02` (Import file OBJ hợp lệ)*: `M3_AssetUnitTests` → `[ModelLoader] TC-M3-02/05 Valid quad OBJ triangulated to 12 triangles: PASS` (`tests/fixtures/quad_no_normals_no_uv.obj`) kết hợp nhập thành công cả 8 file `.obj` Blender trong `AssetCatalogTests.cpp` (`TC-M3-23`).
- **M4 (`TC-M4-01`..`TC-M4-29`)**: **29/29 PASS** (tự động). `TC-M4-MANUAL`: **NOT VERIFIED**.
