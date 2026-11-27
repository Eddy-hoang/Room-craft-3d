# Kết quả kiểm thử thực tế — RoomCraft 3D (Test Results)

Cập nhật lần cuối: **2026-11-27**

## 1. Cấu hình máy kiểm thử

- **OS**: Microsoft Windows 11 Home Single Language 64-bit (`10.0.26200`)
- **GPU**: `Intel(R) UHD Graphics` (Vendor: `Intel`, Driver: `32.0.101.7088`)
- **OpenGL Runtime**: `3.3.0 - Build 32.0.101.7088` (`Core Profile`), GLSL `3.30 - Build 32.0.101.7088`
- **Toolchain**: `x86_64-w64-mingw32-g++ 16.2.0` (WinLibs UCRT), `CMake 4.4.4`, `Ninja 1.13.2`, `GLM 1.0.1`, `Assimp v6.0.6`, `stb_image`, `Blender 5.2.3 LTS` (`e13b728c6880`)
- **CTest tổng hợp**: `ctest --test-dir build/mingw-x64-debug --output-on-failure -V` → **`7/7 Passed (100%, 1.45 sec)`**

---

## 2. Kết quả kiểm thử Milestone M3 (Blender Asset Pipeline)

| Test ID | Hạng mục | Kết quả mong đợi | Kết quả thực tế & Bằng chứng | Trạng thái |
|---|---|---|---|---|
| **TC-M3-01** | Regression M0/M1/M2 | 5 bộ test M0–M2 PASS | `M0_SmokeTest` (`0.24s`), `M1_UnitTests` (`0.02s`), `M1_IntegrationSmokeTest`, `M2_GeometryUnitTests`, `M2_IntegrationSmokeTest` đều `PASS` trong đợt chạy `7/7` CTest | **PASS** |
| **TC-M3-02** | Build & Link Assimp + stb | `0` errors, `0` warnings, không phụ thuộc DLL MinGW | `cmake --build --preset mingw-x64-debug`: `0` errors, `0` project warnings; `ntldd RoomCraft3D.exe` chỉ link 7 DLL hệ thống Windows (`ADVAPI32`, `GDI32`, `KERNEL32`, `OPENGL32`, `SHELL32`, `USER32`, `WINMM`) | **PASS** |
| **TC-M3-03** | OBJ Import cơ bản | Nạp `modern_sofa.obj` hợp lệ | `M3_AssetUnitTests`: `modern_sofa.obj` nạp 15 sub-meshes, 6 materials, 2 textures, 1,344 verts, 836 tris (`11.19 ms`), `validate() == true` | **PASS** |
| **TC-M3-04** | Triangulation & GenNormals/UV | Tự chia tam giác & sinh normal/UV | `M3_AssetUnitTests`: `quad_no_normals_no_uv.obj` → 2 tam giác (6 indices), normals đơn vị, UV hữu hạn | **PASS** |
| **TC-M3-05** | Multi-material OBJ/MTL | Tách 2 sub-mesh đúng màu `Kd` | `M3_AssetUnitTests`: `multi_material.obj` tách đúng 2 sub-meshes ứng với `MatRed` và `MatBlue` | **PASS** |
| **TC-M3-06** | Node Transform & Normal Matrix | Đúng vị trí và pháp tuyến với non-uniform scale | `M3_AssetUnitTests`: `convertRowMajor4x4` và biến đổi $(M^{-1})^T$ đạt sai số `< 1e-5` | **PASS** |
| **TC-M3-07** | Đường dẫn texture tương đối | Giải phân tương đối theo `.obj` | `M3_AssetUnitTests`: `textured_quad.obj` và toàn bộ 8 Blender `.mtl` dùng `map_Kd` tương đối, không hardcode đường dẫn máy | **PASS** |
| **TC-M3-08** | Xử lý file không tồn tại | Trả `false`, thông báo lỗi, không crash | `M3_AssetUnitTests`: `loadFromFile(".../does_not_exist.obj")` trả `false` kèm thông báo lỗi | **PASS** |
| **TC-M3-09** | Xử lý file OBJ rỗng / lỗi | Từ chối sạch `invalid_empty.obj` | `M3_AssetUnitTests`: `invalid_empty.obj` trả `false`, `subMeshes.empty()` | **PASS** |
| **TC-M3-10** | Normalization: Pivot tâm đáy | `min.y == 0`, tâm XZ `== 0` | `M3_AssetUnitTests`: mesh lệch `(10, 5, -3)` sau chuẩn hóa có `min.y = 0`, `center.x = 0`, `center.z = 0` | **PASS** |
| **TC-M3-11** | Normalization: Idempotent | Gọi 2 lần cho kết quả trùng nhau | `M3_AssetUnitTests`: sai khác BBox giữa lần 1 và lần 2 `< 1e-6` | **PASS** |
| **TC-M3-12** | Normalization: Unit Scale | `unitScale = 0.01f` đổi cm → m | `M3_AssetUnitTests`: cuboid `(200, 80, 90)` cm quy về đúng `(2.0, 0.8, 0.9)` m với pivot tâm đáy | **PASS** |
| **TC-M3-13** | Normalization: Bảo toàn Normals | Normals giữ đơn vị và đúng hướng | `M3_AssetUnitTests`: `|length(n) - 1| <= 1e-4` trước và sau chuẩn hóa | **PASS** |
| **TC-M3-14** | Camera Auto-Fit | Khung hình chứa đủ 8 góc AABB | `M1_UnitTests` / `M3_AssetUnitTests`: `fitBoundingBox()` cho AABB nhỏ & lớn ở cả Perspective và Orthographic đều đặt 8 góc trong NDC `[-1, 1]` | **PASS** |
| **TC-M3-15** | FurnitureCatalog Registry | 14 mục (6 Procedural + 8 Blender), ID duy nhất | `M3_AssetUnitTests`: 6 `procedural-*` + 8 `blender-*`, không trùng ID, lọc theo nguồn & tìm kiếm đúng | **PASS** |
| **TC-M3-16** | Kiểm tra 8 Blender Assets (CPU) | 8/8 hợp lệ & dưới budget tam giác | Sofa `836/5000`, Bed `656/4000`, Gaming Chair `1292/8000`, Computer `764/3000`, Plant `896/5000`, Floor Lamp `560/2000`, Wardrobe `588/3000`, Carpet `44/500` — tất cả `PASS` | **PASS** |
| **TC-M3-17** | Texture UV Orientation (GPU) | Nửa trên Đỏ, nửa dưới Xanh (không lật ngược) | `M3_IntegrationSmokeTest`: `TopRGB=(255,0,0) BotRGB=(0,0,255) -> PASS` | **PASS** |
| **TC-M3-18** | Missing Texture Fallback (GPU) | Tự kích hoạt checkerboard, không crash | `M3_IntegrationSmokeTest`: `missing_tex_quad.obj` dùng fallback checkerboard texture thành công | **PASS** |
| **TC-M3-19** | Texture Cache Reuse (GPU) | Tái sử dụng cùng `GLuint` handle | `M3_IntegrationSmokeTest`: tải lặp cùng ảnh trả về cùng `textureId`, số lượng cache không đổi | **PASS** |
| **TC-M3-20** | Asset GPU Cache Reuse | Tái sử dụng cùng `GpuAsset*` | `M3_IntegrationSmokeTest`: `getOrLoad` lặp lại trả cùng địa chỉ cache, không tăng tài nguyên GPU | **PASS** |
| **TC-M3-21** | Transactional Reload (GPU) | Đổi mới khi OK; giữ asset cũ khi lỗi | `M3_IntegrationSmokeTest`: reload thành công cập nhật asset; reload với nguồn lỗi giữ nguyên asset trước đó | **PASS** |
| **TC-M3-22** | Render 6 Procedural Assets (GPU) | 6/6 render FBO có pixel & nền góc sạch | `M3_IntegrationSmokeTest`: 6/6 (`procedural-desk`..`procedural-curved-chair`) `cornerBg=OK`, `coveredPx > 0` | **PASS** |
| **TC-M3-23** | Render 8 Blender Assets (GPU) | 8/8 render FBO có pixel & nền góc sạch | Sofa `15,566px`, Bed `13,383px`, Chair `9,458px`, Computer `14,008px`, Plant `6,107px`, Lamp `3,973px`, Wardrobe `20,092px`, Carpet `6,098px` (`cornerBg=OK`) | **PASS** |
| **TC-M3-24** | Multi-Material Render (GPU) | Trái Đỏ `(255,0,0)`, Phải Xanh `(0,0,255)` | `M3_IntegrationSmokeTest`: `LeftRGB=(255,0,0) RightRGB=(0,0,255) -> PASS` | **PASS** |
| **TC-M3-25** | M3 Asset Browser Frame Loop | Render 10 frames ImGui + FBO ổn định | `M3_IntegrationSmokeTest`: 10 frames (`drawCalls=20, vertices=1410, indices=2590, triangles=836` trên `blender-modern-sofa`) | **PASS** |
| **TC-M3-26** | GPU Resource Cleanup | Giải phóng cache trước hủy context | `M3_IntegrationSmokeTest`: sau `shutdownGraphics()`, cache về `0 assets, 0 textures`, không double-delete | **PASS** |
| **TC-M3-27** | OpenGL Errors & Standalone Exit | `GL_NO_ERROR`, exit `0` ngoài PATH WinLibs | `OpenGL errors: NONE`; chạy độc lập ngoài PATH WinLibs: `ASSET_UNIT_EXIT=0` (`0.18s`), `M3_SMOKE_EXIT=0` (`0.57s`) | **PASS** |
| **TC-M3-MANUAL** | Furniture Asset Browser GUI | Kiểm thử tương tác thủ công M3 | Chưa được người dùng chạy kiểm chứng trực quan | **NOT VERIFIED** |

---

## 3. Tóm tắt trạng thái M0, M1 & M2 (Giữ nguyên không tự ý đổi `NOT VERIFIED`)

- **M0 (`TC-M0-01`..`TC-M0-07`)**: **7/7 PASS** (tự động). `TC-M0-MSVC`: **NOT RUN**. `TC-M0-MANUAL`: **NOT VERIFIED**.
- **M1 (`TC-M1-01`..`TC-M1-16`)**: **16/16 PASS** (tự động). `TC-M1-MANUAL`: **NOT VERIFIED**.
- **M2 (`TC-M2-01`..`TC-M2-23`)**: **23/23 PASS** (tự động). `TC-M2-MANUAL`: **NOT VERIFIED**.
