# Kết quả kiểm thử thực tế — RoomCraft 3D (Test Results)

Cập nhật lần cuối: **2026-11-28**

## Kết quả M9 ngày 28/11/2026 (`v1.0.0-rc1` — Release Candidate, Reliability, Shadows & Packaging)

- **Debug Build (`mingw-x64-debug`)**: `0` lỗi, `0` cảnh báo dự án, **`19/19` CTest PASS (`4.82 s`)**.
- **Release Build (`mingw-x64-release`)**: `0` lỗi, `0` cảnh báo dự án, `7,960,281` bytes, **`19/19` CTest PASS (`3.56 s`)**, chỉ link `8` DLL Windows (`ntldd`).
- **M9_ReliabilityUnitTests (16/16 PASS)**: Kiểm toán JSON UTF-8 / surrogate / duplicate key / overflow / node cap + **1.500 ca fuzzing**, tiêm lỗi khóa tệp độc quyền Win32, tương thích ngược schema v1 M8, exact Undo/Redo + Lighting dirty-state, Prevent Invalid không kẹt, rug policy chính xác, `PreferencesManager` round-trip/clamping/corrupt fallback.
- **M9_IntegrationSmokeTest (5/5 PASS)**: Directional Shadow Mapping (`512/1024/2048`) tạo hiệu ứng tối bóng đo được (`AvgLuma Off=55.74 → On(1024)=54.98`), tương thích MSAA 4×, `OpenGL errors: NONE`.
- **Benchmark Release (`1280×720`, Intel UHD, `--m9-benchmark`)**: `30 obj + Shadows 1024` đạt **`366.5 FPS` (`2.73 ms` avg / `3.56 ms` p95)**; `MSAA 4× + Shadows 1024` đạt **`292.2 FPS` (`3.42 ms`)**; `250 obj Stress` đạt **`110.6 FPS` (`9.04 ms`)**. Chi tiết tại [`docs/PERFORMANCE_REPORT.md`](PERFORMANCE_REPORT.md) và [`docs/FINAL_QA_REPORT.md`](FINAL_QA_REPORT.md).
- **Portable Isolated Test**: Giải nén `dist/RoomCraft3D-v1.0.0-rc1-win64.zip` vào đường dẫn Unicode + khoảng trắng ngoài `D:\CG` với `PATH=C:\Windows\System32;C:\Windows` → `--version`, `--m8-smoke-test`, `--m9-smoke-test` đều **PASS** (`Exit 0`).
- **Ảnh chụp GPU thật**: `12/12` ảnh PNG `1280×720` tại `docs/evidence/m9/`.
- **Kiểm thử thủ công tương tác**: Giữ `NOT VERIFIED` trong [`docs/RELEASE_CHECKLIST.md`](RELEASE_CHECKLIST.md) chờ người dùng duyệt `v1.0.0`.

---

## Kết quả M8 ngày 28/11/2026 (`v0.8.0` — Collision Detection & Project Persistence)

- **Build Debug (`mingw-x64-debug`)**: `cmake --preset mingw-x64-debug -DFETCHCONTENT_UPDATES_DISCONNECTED=ON` + `cmake --build --preset mingw-x64-debug` → **Exit `0`**, `0` compile errors, `0` project warnings dưới `-Wall -Wextra -Wpedantic`.
- **CTest tổng hợp**: `ctest --test-dir build/mingw-x64-debug --output-on-failure` → **`17/17 Passed (100%, 6.65 sec)`** (`14/14` regression M0–M7 + `M8_CollisionUnitTests` + `M8_PersistenceUnitTests` + `M8_IntegrationSmokeTest`).
- **Kiểm tra độc lập ngoài PATH WinLibs**: `RoomCraft3D.exe --help` (`v0.8.0`) và `RoomCraft3D.exe --m8-smoke-test` thoát mã `0`, `OpenGL errors: NONE`; `ntldd` chỉ link **8 DLL hệ thống Windows** (`ADVAPI32`, `COMDLG32`, `GDI32`, `KERNEL32`, `OPENGL32`, `SHELL32`, `USER32`, `WINMM`).
- **Benchmark Collision CPU (Debug, 20 vòng đo)**:
  - `N = 30` (`435` broad pairs, `10` narrow SAT): **`0.1191 ms`** full scene / **`0.0451 ms`** incremental 1-object.
  - `N = 100` (`4,950` broad pairs, `6` narrow SAT): **`0.5097 ms`** full scene / **`0.1702 ms`** incremental 1-object.
  - `N = 250` (`31,125` broad pairs, `303` narrow SAT): **`1.8744 ms`** full scene / **`0.3709 ms`** incremental 1-object.
- **Trạng thái kiểm thử thủ công**: `M8-T32` và các mục manual/visual tồn đọng M6–M7 giữ **`NOT VERIFIED`**. Xem ma trận truy vết đầy đủ `M8-T01`..`M8-T32` tại [`docs/M8_DELIVERY.md`](M8_DELIVERY.md).

---

## Kết quả M7 ngày 28/11/2026

Baseline M6 đạt 13/13 trong 4,77 s. M7 đạt 14/14 trong 6,63 s. Cả hai benchmark thoát mã 0. M7 đạt 6,08 ms trung bình toàn editor với 30 vật thể, p95 7,36 ms. Dữ liệu gốc, phương pháp, traceability và ảnh thật nằm trong [báo cáo M7](M7_DELIVERY.md). Phần bên dưới là kết quả lịch sử M6.

## 1. Cấu hình máy kiểm thử

- **OS**: Microsoft Windows 11 Home Single Language 64-bit (`10.0.26200`)
- **GPU**: `Intel(R) UHD Graphics` (Vendor: `Intel`, Driver: `32.0.101.7088`)
- **OpenGL Runtime**: `3.3.0 - Build 32.0.101.7088` (`Core Profile`), GLSL `3.30 - Build 32.0.101.7088`
- **Toolchain**: `x86_64-w64-mingw32-g++ 16.2.0` (WinLibs UCRT), `CMake 4.4.4`, `Ninja 1.13.2`, `GLM 1.0.1`, `Dear ImGui v1.93.0-docking`, `Assimp v6.0.6`, `stb_image v2.34`
- **Build Debug**: `0` compile errors, `0` project warnings dưới `-Wall -Wextra -Wpedantic` (chỉ còn 2 cảnh báo CMake deprecation nội bộ từ `GLM 1.0.1`).
- **CTest tổng hợp**: `ctest --test-dir build/mingw-x64-debug --output-on-failure` → **`13/13 Passed (100%, 2.90 sec)`**
- **Kiểm tra phụ thuộc DLL (`ntldd RoomCraft3D.exe`)**: Chỉ link **7 DLL hệ thống Windows** (`ADVAPI32.dll`, `GDI32.dll`, `KERNEL32.dll`, `OPENGL32.dll`, `SHELL32.dll`, `USER32.dll`, `WINMM.dll`), chạy độc lập hoàn toàn ngoài PATH WinLibs.

---

## 2. Kết quả kiểm thử Milestone M6 (Professional Editor UI/UX)

> **Lưu ý trung thực**: Các test đánh dấu **`PASS`** bên dưới là các kiểm thử tự động đã chạy thực tế trên CPU hoặc GPU OpenGL 3.3 Core. Mọi test trực quan/thủ công (`Manual-Visual`) và chụp ảnh màn hình UI giữ nguyên trạng thái **`NOT VERIFIED`** cho tới khi người dùng trực tiếp mở `RoomCraft3D.exe` và xác nhận.

### 2.1. Các test tự động đã chạy & đạt (`Auto-CPU` / `Auto-GPU`)

| Test ID | Tầng | Kết quả thực tế & Bằng chứng | Trạng thái |
|---|---|---|---|
| **TC-M6-01** | Regression | Toàn bộ **`11/11` CTest suites từ M0–M5 đạt `PASS`** trong đợt chạy `13/13` (`2.90s`), gồm cả `TC-M5-38` (cyan selection highlight) | **PASS** |
| **TC-M6-06** | Auto-GPU | `M6_IntegrationSmokeTest`: `[TC-M6-06] Custom TrueType UI font loaded (Roboto-Medium.ttf): PASS` (`15 px`, chạy cả từ `build/` lẫn từ `C:\`) | **PASS** |
| **TC-M6-08..10** | Auto-CPU | `M6_EditorUiUnitTests`: `[M6-UI] TC-M6-08..10 Layout non-overlapping & viewport largest at 1280x720, 1366x768, 1600x900, 1920x1080: PASS` | **PASS** |
| **TC-M6-11/12** | Auto-CPU | `M6_EditorUiUnitTests`: `[M6-UI] TC-M6-11/12 DPI scale (100%/125%/150%) & splitter resizing keep viewport usable: PASS` | **PASS** |
| **TC-M6-13..15** | Auto-CPU | `M6_EditorUiUnitTests`: `[M6-UI] TC-M6-13..15 FurnitureCatalog has 14 assets (6 C++ + 8 Blender) with accurate search/category filter: PASS` | **PASS** |
| **TC-M6-16** | Auto-GPU | `M6_IntegrationSmokeTest`: `[M6-Thumbnails] Generated 14/14 (256x256) in 3.7043 ms (nonBlank=14/14, cacheCount=14)` → `[TC-M6-16]: PASS` | **PASS** |
| **TC-M6-17/18/34** | Auto-GPU | `M6_IntegrationSmokeTest`: `[TC-M6-17/18/34] Thumbnail cache reuse, reload invalidation & OpenGL state restoration: PASS` | **PASS** |
| **TC-M6-25** | Auto-CPU | `M6_EditorUiUnitTests`: `[M6-UI] TC-M6-25 NotificationSystem queue cap, expiration & clear lifecycle: PASS` | **PASS** |
| **TC-M6-27/28** | Auto-CPU | `M6_EditorUiUnitTests`: `[M6-UI] TC-M6-27/28 & TC-M6-GIZMO 3-Axis Move Gizmo hit, X-constrained drag after resize/DPI, Esc cancel & locked block: PASS` | **PASS** |
| **TC-M6-30..32** | Auto-GPU | `M6_IntegrationSmokeTest`: `[TC-M6-30..33] Tools workspace switching (Geometry Lab/Asset Inspector/DevTools) preserves Scene & Camera: PASS` | **PASS** |
| **TC-M6-GIZMO** | Auto-CPU + GPU | `M6_EditorUiUnitTests` + `M6_IntegrationSmokeTest`: `[TC-M6-GIZMO] 3-Axis Move Gizmo hit-testing & GPU rendering verified: PASS` | **PASS** |
| **TC-M6-PERF** | Auto-GPU | `M6_IntegrationSmokeTest`: 30 vật thể `1280×720` off-screen `1.68 ms (595.3 FPS)`; 14 thumbnails cache 1 lần `3.70 ms (0.265 ms/thumb)` → `PASS` | **PASS** |
| **TC-M6-35/36** | Auto-GPU | `M6_IntegrationSmokeTest`: `[TC-M6-35/36] ThumbnailRenderer GPU cleanup & zero unexpected OpenGL errors: PASS`; chạy độc lập ngoài PATH WinLibs từ `C:\` thoát mã `0`, `OpenGL errors: NONE` | **PASS** |

### 2.2. Các test trực quan & thủ công (`Manual-Visual` — Chưa xác nhận)

| Test ID | Hạng mục | Trạng thái | Ghi chú |
|---|---|---|---|
| **TC-M6-02..05, 07, 19..24, 26, 29, 33** | Kiểm tra trực quan Menu Bar, Theme, Axis Color, Vector Icons, Grid/List Library, Viewport Overlays, Hierarchy Context Menu, Inspector, Status Bar, Modals, Dialogs | **NOT VERIFIED** | Đã triển khai trong mã nguồn; cần người dùng mở `RoomCraft3D.exe` xác nhận trực quan |
| **TC-M6-MANUAL** | Checklist trải nghiệm UI/UX thủ công + ảnh chụp thực tế | **NOT VERIFIED** | Chờ người dùng chạy thực tế và xác nhận |

---

## 3. So sánh Hiệu năng M6 với M4 (30 vật thể, `1280×720`, Intel UHD Graphics, Debug)

- **Điều kiện đo giống nhau**: Build `Debug` (`mingw-x64-debug`), GPU `Intel(R) UHD Graphics`, FBO `1280×720`, off-screen FBO + `glFinish()` (VSync: `N/A`), Warm-up `5` frames, Measured `30` frames.

| Chỉ số | M4 Baseline (`--m4-benchmark`) | M6 (`--m6-smoke-test`) | Nhận xét |
|---|---:|---:|---|
| **Visible Objects / Unique GPU Assets** | `30 / 6` | `30 / 6` (`CacheAssets=14, CacheTex=11`) | Tái sử dụng tài nguyên GPU ổn định |
| **Submitted / Frame (Draws / Verts / Tris)** | `417 / 35,759 / 24,876` | `417 / 35,759 / 24,876` | **0 draw call tăng thêm** cho cảnh 3D |
| **Avg Frame Time / Avg FPS (Off-screen)** | `1.50 ms / 667.8 FPS` | `1.68 ms / 595.3 FPS` | Dao động tự nhiên (~0.18 ms trong Debug), không suy giảm hiệu năng |
| **Sinh 14 GPU Thumbnails `256×256`** | Chưa có | **`3.70 ms` tổng (`0.265 ms/thumb`)** | Chỉ chạy 1 lần lúc warm-up/hiển thị, sau đó 100% cache `GLuint` |

*(Ghi chú: Số liệu phản ánh hiệu năng render off-screen FBO của cảnh hiện tại ở bản Debug, chưa bao gồm chi phí ánh sáng đa nguồn/bóng đổ của M7.)*

---

## 4. Ánh xạ chi tiết 39 Test Case Milestone M5 (Giải trình kiểm toán theo phản hồi Review M5)

Tại M5, **39 test case định nghĩa trong `TEST_PLAN.md`** (`TC-M5-01`..`TC-M5-39` kèm 2 hạng mục nâng cao `TC-M5-TRI`, `TC-M5-OCC`) được kiểm chứng trọn vẹn bởi **`30` nhóm assertion CPU** trong `M5_InteractionUnitTests` (`tests/unit/InteractionTests.cpp`), **`6` nhóm kiểm tra GPU** trong `M5_IntegrationSmokeTest` (`Application::runM5IntegrationSuiteOnce()`) và **`9/9` bộ regression CTest M0–M4**:

| Nhóm Test ID (`TEST_PLAN.md`) | Tổng số TC | Bộ kiểm thử thực thi | Tên nhóm assertion / kiểm tra cụ thể | Trạng thái |
|---|---:|---|---|---|
| `TC-M5-01` | 1 | CTest M0–M4 (`#1`–`#9`) | Toàn bộ `9/9` CTest từ M0 đến M4 | **PASS** |
| `TC-M5-02..08` | 7 | `M5_InteractionUnitTests` (5 checks) | `TC-M5-02/03` (NDC+Y-flip), `TC-M5-04` (2× DPI), `TC-M5-05/08` (Persp+Norm), `TC-M5-06` (Ortho), `TC-M5-07` (Invalid) | **PASS** |
| `TC-M5-09..13` | 5 | `M5_InteractionUnitTests` (5 checks) | `TC-M5-09`, `TC-M5-10`, `TC-M5-11`, `TC-M5-12`, `TC-M5-13` (Ray–AABB Slab) | **PASS** |
| `TC-M5-14..18` | 5 | `M5_InteractionUnitTests` (5 checks) + `M5_IntegrationSmokeTest` (2 checks) | `TC-M5-14`, `TC-M5-15`, `TC-M5-16` (Locked), `TC-M5-17`, `TC-M5-18` + GPU Bookshelf pick & hidden exclusion | **PASS** |
| `TC-M5-19..25` | 7 | `M5_InteractionUnitTests` (6 checks) | `TC-M5-19`, `TC-M5-20`, `TC-M5-21`, `TC-M5-22`, `TC-M5-23`, `TC-M5-24/25` (CameraController) | **PASS** |
| `TC-M5-26..31` | 6 | `M5_InteractionUnitTests` (5 checks) + `M5_IntegrationSmokeTest` (1 check) | `TC-M5-26` (Ray–Plane), `TC-M5-27`, `TC-M5-28`, `TC-M5-29/31`, `TC-M5-30` + GPU drag/cancel | **PASS** |
| `TC-M5-32..35, 37` | 5 | `M5_InteractionUnitTests` (1 grouped check) | `TC-M5-32..35/37`: Delete during drag, one-shot `Ctrl+D`, `uiWantsTextInput`, Selection sync & safe recovery | **PASS** |
| `TC-M5-36, 38, 39` | 3 | `M5_IntegrationSmokeTest` (3 checks) | `TC-M5-06/36` (Ortho 960×540 pick), `TC-M5-38` (post-orbit cyan BBox pixels), `TC-M5-39` (zero GL errors) | **PASS** |
| `TC-M5-TRI`, `TC-M5-OCC` *(P1)* | +2 | `M5_InteractionUnitTests` (3 checks) + `M5_IntegrationSmokeTest` | Möller–Trumbore unit + hollow desk pass-through + Cutaway/AllWalls occlusion | **PASS** |
| **Tổng M5** | **39 (+2 P1)** | **`30` CPU checks + `6` GPU checks + `9` Regression** | `M5_InteractionUnitTests` (`0.01s`) + `M5_IntegrationSmokeTest` (`0.37s`) | **PASS** |

---

## 5. Tóm tắt trạng thái M0–M5

- **M0 (`TC-M0-01`..`TC-M0-07`)**: **7/7 PASS** (tự động). `TC-M0-MSVC`: **NOT RUN**. `TC-M0-MANUAL`: **NOT VERIFIED**.
- **M1 (`TC-M1-01`..`TC-M1-16`)**: **16/16 PASS** (tự động). `TC-M1-MANUAL`: **NOT VERIFIED**.
- **M2 (`TC-M2-01`..`TC-M2-23`)**: **23/23 PASS** (tự động). `TC-M2-MANUAL`: **NOT VERIFIED**.
- **M3 (`TC-M3-01`..`TC-M3-27`)**: **27/27 PASS** (tự động; `TC-M3-02` đối chiếu trực tiếp từ `ModelLoaderTests.cpp` + `AssetCatalogTests.cpp`). `TC-M3-MANUAL`: **NOT VERIFIED**.
- **M4 (`TC-M4-01`..`TC-M4-29`)**: **29/29 PASS** (tự động; benchmark 10/30/100 đạt `0.68 / 1.50 / 5.03 ms`). `TC-M4-MANUAL`: **NOT VERIFIED**.
- **M5 (`TC-M5-01`..`TC-M5-39` + `TRI`/`OCC`)**: **39/39 (+2 P1) PASS** (tự động). `TC-M5-MANUAL`: **NOT VERIFIED**.

## Final QA M7

Bản M7 chuẩn đạt 14/14 CTest trong 11,81 s. Benchmark kiểm số mẫu thật 1/2/4 và ghi đủ chín lượt, p95 11,71–49,84 ms theo cấu hình và vòng đo. [CTest](evidence/m7/qa-ctest.txt) · [Benchmark](evidence/m7/qa-bench.txt) · [Ma trận truy vết](M7_DELIVERY.md#kiểm-toán-kỹ-thuật-cuối-m7)


---

## Kết quả Kiểm thử Thực tế — M9 Visual Quality Refinement (`v1.0.0-rc2`)

- **Release (`mingw-x64-release`)**: `ctest --test-dir build/mingw-x64-release --output-on-failure` → **`19/19 PASS` (`5.95 s`, exit `0`, 0 errors, 0 project warnings)**.
- **Debug (`mingw-x64-debug`)**: `ctest --test-dir build/mingw-x64-debug --output-on-failure` → **`19/19 PASS` (`6.62 s`, exit `0`, 0 errors, 0 project warnings)**.
- **Kiểm chứng Portable ngoài repo có đường dẫn Unicode (`%TEMP%\RoomCraft3D_RC2_Phòng 3D\RoomCraft3D.exe --m9-smoke-test`)**: **`PASS` (`exit 0`, `OpenGL errors: NONE`, `WarmInterior Clipped=0.000%`, `UpperWestWall ShadowAcneMeanDiff=0.938`)**.
- **Chụp bằng chứng GPU (`--m9-capture-evidence docs/evidence/rc2`)**: **`18/18 ảnh PASS` (`OpenGL errors: NONE`)** + 4 bảng so sánh RC1 vs RC2.
- **Manual Desktop Acceptance & Independent QA lần 2**: `NOT VERIFIED / PENDING`.
