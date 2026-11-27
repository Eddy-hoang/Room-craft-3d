# Kết quả kiểm thử thực tế — RoomCraft 3D (Test Results)

Cập nhật lần cuối: **2026-11-28**

## 1. Cấu hình máy kiểm thử

- **OS**: Microsoft Windows 11 Home Single Language 64-bit (`10.0.26200`)
- **GPU**: `Intel(R) UHD Graphics` (Vendor: `Intel`, Driver: `32.0.101.7088`)
- **OpenGL Runtime**: `3.3.0 - Build 32.0.101.7088` (`Core Profile`), GLSL `3.30 - Build 32.0.101.7088`
- **Toolchain**: `x86_64-w64-mingw32-g++ 16.2.0` (WinLibs UCRT), `CMake 4.4.4`, `Ninja 1.13.2`, `GLM 1.0.1`, `Assimp v6.0.6`, `stb_image v2.34`
- **CTest tổng hợp**: `ctest --test-dir build/mingw-x64-debug --output-on-failure -V` → **`9/9 Passed (100%, 2.76 sec)`**

---

## 2. Kết quả kiểm thử Milestone M4 (Scene Management & Room Editor)

| Test ID | Hạng mục | Kết quả thực tế & Bằng chứng | Trạng thái |
|---|---|---|---|
| **TC-M4-01** | Regression M0–M3 | `M0_SmokeTest`, `M1_UnitTests`, `M1_IntegrationSmokeTest`, `M2_GeometryUnitTests`, `M2_IntegrationSmokeTest`, `M3_AssetUnitTests`, `M3_IntegrationSmokeTest` — toàn bộ `7/7` PASS trong đợt chạy `9/9` CTest | **PASS** |
| **TC-M4-02** | Default Room Dimensions | `M4_SceneUnitTests`: `[Room] TC-M4-02 Default room dimensions (5x6x3m, area=30, vol=90): PASS` | **PASS** |
| **TC-M4-03** | Room Interior Bounds | `M4_SceneUnitTests`: `[Room] TC-M4-03 Room interior bounds match [-W/2, W/2] x [0, H] x [-L/2, L/2]: PASS` | **PASS** |
| **TC-M4-04** | Floor Geometry & `y = 0` | `M4_SceneUnitTests`: `[Room] TC-M4-04 Floor geometry top at y=0 and extends downward by floorThickness: PASS` | **PASS** |
| **TC-M4-05** | Wall Geometry & Dimensions | `M4_SceneUnitTests`: `[Room] TC-M4-05 Four walls outside interior bounds with exact dimensions & sealed corners: PASS` | **PASS** |
| **TC-M4-06** | Room Resize | `M4_SceneUnitTests` (`color/visibility skip rebuild`, resize tăng revision) + `M4_IntegrationSmokeTest` (`[TC-M4-06/25] Room resize GPU sync & Camera Fit Room: PASS`) | **PASS** |
| **TC-M4-07** | Invalid Room Parameters | `M4_SceneUnitTests`: `[Room] TC-M4-07 Invalid room parameters (negative, zero, NaN) rejected & state preserved: PASS` | **PASS** |
| **TC-M4-08** | Wall Visibility | `M4_SceneUnitTests`: `[Room] TC-M4-08 Wall visibility modes & manual toggles preserve interior bounds: PASS` | **PASS** |
| **TC-M4-09** | Add Procedural Object | `M4_SceneUnitTests`: `[Scene] TC-M4-09 Add procedural object: PASS` | **PASS** |
| **TC-M4-10** | Add Imported Object | `M4_SceneUnitTests`: `[Scene] TC-M4-10 Add imported Blender object: PASS` | **PASS** |
| **TC-M4-11** | Unique Object ID | `M4_SceneUnitTests`: `[Scene] TC-M4-11/13` & `TC-M4-12/19/20` xác nhận ID duy nhất, đơn điệu, không tái sử dụng | **PASS** |
| **TC-M4-12** | Remove Object | `M4_SceneUnitTests`: xóa hợp lệ trả `true`, xóa trùng / ID `999999` trả `false` an toàn | **PASS** |
| **TC-M4-13** | Duplicate Object | `M4_SceneUnitTests`: tạo ID mới và dịch đúng offset `(0.5, 0, 0.5)` | **PASS** |
| **TC-M4-14** | Rename Object | `M4_SceneUnitTests`: `[Scene] TC-M4-14 Rename object trims whitespace and rejects empty/invalid IDs: PASS` | **PASS** |
| **TC-M4-15** | Object Visibility | `M4_SceneUnitTests` + `M4_IntegrationSmokeTest` (`[TC-M4-15/20] Hide/Show, safe delete of selected object & post-reload rendering: PASS`) | **PASS** |
| **TC-M4-16** | Transform Independence | `M4_SceneUnitTests`: `[Scene] TC-M4-16 Transform independence across multiple instances of same asset: PASS` | **PASS** |
| **TC-M4-17** | Invalid Transform Rejection | `M4_SceneUnitTests`: `[Scene] TC-M4-17 Invalid transforms (0/neg scale, NaN) rejected cleanly: PASS` | **PASS** |
| **TC-M4-18** | Invalid Asset ID Rejection | `M4_SceneUnitTests`: `[Scene] TC-M4-18 Invalid Asset ID rejected without modifying scene: PASS` | **PASS** |
| **TC-M4-19** | SelectionState | `M4_SceneUnitTests`: `selectObject`, `isSelected`, `clearSelection`, `reconcileWithScene` PASS | **PASS** |
| **TC-M4-20** | Delete Selected Safely | `M4_SceneUnitTests` + `M4_IntegrationSmokeTest`: xóa đối tượng đang chọn tự xóa selection và render an toàn sau `reloadAsset()` | **PASS** |
| **TC-M4-21** | Shared GPU Asset Reuse | `M4_IntegrationSmokeTest`: `[TC-M4-21] Shared GPU asset reuse across 3 gaming chairs + desk + computer: PASS` (`uniqueAssetsUsed=3` cho 5 objects) | **PASS** |
| **TC-M4-22** | Multi-Object Rendering | `M4_IntegrationSmokeTest`: `[TC-M4-22/23]` (`roomDrawCalls=3`, `objectDrawCalls>50`, `coveredPx > 5000`) | **PASS** |
| **TC-M4-23** | Parent & Submesh Transforms | `M4_IntegrationSmokeTest`: `M_object * M_asset_part` với Computer đặt tại `y=0.75` trên bàn, render PASS | **PASS** |
| **TC-M4-24** | Scene Preset Validation | `M4_SceneUnitTests` + `M4_IntegrationSmokeTest`: Bedroom (`96 draws, 4256 tris, 70241 px`), Gaming (`93 draws, 3872 tris, 67888 px`), Living (`73 draws, 3852 tris, 67240 px`) → **3/3 PASS** | **PASS** |
| **TC-M4-25** | Room Camera Fit | `M4_SceneUnitTests` (Perspective + Top Ortho) + `M4_IntegrationSmokeTest` (`[TC-M4-06/25]: PASS`) | **PASS** |
| **TC-M4-26** | Clear Scene & Lifecycle | `M4_IntegrationSmokeTest`: `[TC-M4-26] Clear Scene empties furniture draw calls & preserves clean lifecycle: PASS` | **PASS** |
| **TC-M4-27** | Framebuffer Resize | `M4_IntegrationSmokeTest`: `[TC-M4-27] Framebuffer resize to 960x540 & re-render: PASS` | **PASS** |
| **TC-M4-28** | 30-Object Benchmark Execution | `M4_IntegrationSmokeTest` (`1.87 ms / 534.5 FPS`, 15 frames) + `--m4-benchmark` (`1.50 ms / 667.8 FPS`, 30 frames) | **PASS** |
| **TC-M4-29** | OpenGL Errors & Safe Shutdown | `M4_IntegrationSmokeTest`: 10 frames (`97 draws, 4256 tris`), `OpenGL errors: NONE`; chạy độc lập ngoài PATH WinLibs: `SCENE_UNIT_EXIT=0`, `M4_BENCH_EXIT=0` | **PASS** |
| **TC-M4-MANUAL** | Room Editor Manual GUI | Chưa được người dùng chạy kiểm chứng trực quan | **NOT VERIFIED** |

---

## 3. Kết quả Benchmark Hiệu năng thực tế (`RoomCraft3D.exe --m4-benchmark`)

- **Cấu hình đo**: Build **`Debug`** (`mingw-x64-debug`), GPU **`Intel(R) UHD Graphics`**, FBO **`1280×720`**, đo off-screen FBO đồng bộ **`glFinish()`** (VSync: `N/A`), Warm-up: `5 frames`, Measured: `30 frames` (10 & 30 objects) / `20 frames` (100 objects).

| Kịch bản | Objects (Visible) | Unique Assets (Cache) | Unique GPU Stored (Verts / Tris) | Submitted / Frame (Draws / Verts / Tris) | Avg Frame Time | Avg FPS | So với mục tiêu 60 FPS |
|---|---:|---:|---|---:|---:|---:|---|
| **10 objects** | 10 (10) | 6 (6 assets, 6 tex) | `7,121 v / 4,968 t` | `163 draws / 12,597 v / 8,100 t` | **0.68 ms** | **1,470.8 FPS** | Vượt mục tiêu (Debug) |
| **30 objects** | 30 (30) | 6 (6 assets, 6 tex) | `7,121 v / 4,968 t` | `417 draws / 35,759 v / 24,876 t` | **1.50 ms** | **667.8 FPS** | **Đạt (>11× mục tiêu 60 FPS, Debug)** |
| **100 objects** *(stress)* | 100 (100) | 6 (6 assets, 6 tex) | `7,121 v / 4,968 t` | `1,393 draws / 119,488 v / 82,620 t` | **5.03 ms** | **198.7 FPS** | Stress test mượt (>3× 60 FPS, Debug) |

---

## 4. Tóm tắt trạng thái M0, M1, M2 & M3 (Giữ nguyên không tự ý đổi `NOT VERIFIED`)

- **M0 (`TC-M0-01`..`TC-M0-07`)**: **7/7 PASS** (tự động). `TC-M0-MSVC`: **NOT RUN**. `TC-M0-MANUAL`: **NOT VERIFIED**.
- **M1 (`TC-M1-01`..`TC-M1-16`)**: **16/16 PASS** (tự động). `TC-M1-MANUAL`: **NOT VERIFIED**.
- **M2 (`TC-M2-01`..`TC-M2-23`)**: **23/23 PASS** (tự động). `TC-M2-MANUAL`: **NOT VERIFIED**.
- **M3 (`TC-M3-01`..`TC-M3-27`)**: **27/27 PASS** (tự động). `TC-M3-MANUAL`: **NOT VERIFIED**.
  - *Bằng chứng `TC-M3-02`*: Build `0` errors, `0` project warnings; `ntldd RoomCraft3D.exe` chỉ phụ thuộc 7 DLL hệ thống Windows (`ADVAPI32`, `GDI32`, `KERNEL32`, `OPENGL32`, `SHELL32`, `USER32`, `WINMM`).
