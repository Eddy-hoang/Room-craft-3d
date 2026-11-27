# Kết quả kiểm thử thực tế — RoomCraft 3D (Test Results)

Cập nhật lần cuối: **2026-11-27**

## 1. Cấu hình máy kiểm thử

- **OS**: Microsoft Windows 11 Home Single Language 64-bit (`10.0.26200`)
- **GPU**: `Intel(R) UHD Graphics` (Vendor: `Intel`, Driver: `32.0.101.7088`)
- **OpenGL Runtime**: `3.3.0 - Build 32.0.101.7088` (`Core Profile`), GLSL `3.30 - Build 32.0.101.7088`
- **Toolchain**: `x86_64-w64-mingw32-g++ 16.2.0` (WinLibs UCRT), `CMake 4.4.4`, `Ninja 1.13.2`, `GLM 1.0.1`
- **CTest tổng hợp**: `ctest --test-dir build/mingw-x64-debug --output-on-failure -V` → **`5/5 Passed (3.24 sec)`**

---

## 2. Kết quả kiểm thử Milestone M2 (Procedural Geometry & Modeling)

| Test ID | Hạng mục | Kết quả mong đợi | Kết quả thực tế & Bằng chứng | Trạng thái |
|---|---|---|---|---|
| **TC-M2-01** | Regression M0/M1 | `M0_SmokeTest`, `M1_UnitTests`, `M1_IntegrationSmokeTest` PASS | Cả 3 bộ test đạt `PASS` trong đợt chạy `5/5` CTest (`M0_SmokeTest`, `M1_UnitTests` 23/23, `M1_IntegrationSmokeTest` 10/10) | **PASS** |
| **TC-M2-02** | Plane | `3×2m`, normal `+Y`, CCW, UV | `M2_GeometryUnitTests`: `[Primitives] TC-M2-02 Plane ... PASS` (12 verts, 36 indices) | **PASS** |
| **TC-M2-03** | Cube / Cuboid | 24 verts, 36 indices, bottom pivot | `[Primitives] TC-M2-03 Cuboid ... PASS` + `Cube delegates to Cuboid ... PASS` | **PASS** |
| **TC-M2-04** | Cylinder | Capped/open topology, UV seam, normals | `[Primitives] TC-M2-04 Cylinder capped/open topology, UV seam, outward normals & winding: PASS` | **PASS** |
| **TC-M2-05** | Sphere | Radius, analytic normals, non-degenerate poles | `[Primitives] TC-M2-05 Sphere radius, analytic normals, non-degenerate poles & CCW winding: PASS` | **PASS** |
| **TC-M2-06** | Cone / Frustum | Apex hợp lệ, sloped normals | `[Primitives] TC-M2-06 Cone apex (finite sloped normals, no degenerate tris) & Frustum: PASS` | **PASS** |
| **TC-M2-07** | Bézier Curve Endpoints | `B(0)=P0`, `B(1)=P3`, khớp de Casteljau | `[BezierCurve] TC-M2-07 Endpoints B(0)==P0, B(1)==P3 and de Casteljau agreement: PASS` | **PASS** |
| **TC-M2-08** | Bézier Curve Derivatives | Đạo hàm, thẳng hàng, lấy mẫu, reject `t` lỗi | `[BezierCurve] TC-M2-08 Derivatives, collinearity, sampling count & invalid t rejection: PASS` | **PASS** |
| **TC-M2-09** | Bézier Surface Corners | Khớp 4 góc control grid | `[BezierSurface] TC-M2-09 BezierSurface matches all 4 corner control points: PASS` | **PASS** |
| **TC-M2-10** | Bézier Surface Normals & Topology | Flat/curved normals, winding, reject suy biến | `[BezierSurface] TC-M2-10 Flat/Curved normals, topology counts, winding & invalid patch rejection: PASS` | **PASS** |
| **TC-M2-11** | Revolution Surface | Rings, UV seam, cap, cực `r=0` | `[RevolutionSurface] TC-M2-11 Bezier revolution rings, UV seam, bottom cap & zero-radius pole: PASS` | **PASS** |
| **TC-M2-12** | Normal Validation | Độ dài `≈ 1`, hữu hạn trên mọi mesh | Đạt trong `MeshData::validate()` + `checkAllNormalsUnitAndUvFinite()` ở tất cả generator | **PASS** |
| **TC-M2-13** | UV Validation | UV hữu hạn `[0,1]` + UV seam | Đạt trên toàn bộ primitives, Bézier Surface và Revolution Surface | **PASS** |
| **TC-M2-14** | Index Bounds & Degenerates | Không vượt biên, không tam giác diện tích 0 | `MeshData::validate(..., checkDegenerateTriangles=true)` đạt 100% trên mọi mesh sinh ra | **PASS** |
| **TC-M2-15** | Invalid Parameter Handling | Từ chối tham số âm/0/NaN/vượt giới hạn | 3 nhóm test `TC-M2-15` (Primitives, RevolutionSurface, Furniture) đều `PASS` | **PASS** |
| **TC-M2-16** | BoundingBox | `min/max/center/size` + xoay 90° Y | `[BoundingBox] TC-M2-16` (2 assertions) `PASS` | **PASS** |
| **TC-M2-17** | Furniture Hierarchy | `M_parent * M_local` cho toàn bộ Desk parts | `[Furniture] TC-M2-17 Hierarchy M_parent * M_local propagates accurately to all desk parts: PASS` | **PASS** |
| **TC-M2-18** | ≥ 5 Procedural Furniture Models | 6 mô hình hợp lệ, đứng trên `y = 0` | Desk (`144v/72t`), Chair (`492v/420t`), Bookshelf (`192v/96t`), Desk Lamp (`529v/672t`), Vase (`727v/1312t`), Curved Chair (`447v/508t`) đều `PASS` | **PASS** |
| **TC-M2-19** | Mesh Regeneration | Regen `8->32` segs & giữ mesh cũ khi lỗi | `[TC-M2-19] Mesh regeneration (8->32 segs) and prior-mesh preservation on invalid input: PASS` | **PASS** |
| **TC-M2-20** | GPU Rendering Categories | Render & kiểm tra BBox pixel coverage 12 mục | Cả 12 mục (Cuboid `19058px`, Cylinder `14216px`, Sphere `10963px`, Frustum `11234px`, BezierSurface `15849px`, Revolution `7471px`, Desk `6407px`, Chair `2846px`, Bookshelf `15182px`, Lamp `4295px`, Vase `4889px`, CurvedChair `3835px`, `cornerBg=OK`) đều `PASS` | **PASS** |
| **TC-M2-21** | Wireframe State Restoration | Khôi phục `GL_FILL` sau `endFrame()` | `[TC-M2-21] Wireframe polygon mode restored to GL_FILL after endFrame(): PASS` | **PASS** |
| **TC-M2-22** | GPU Resource Lifecycle | Tái sinh 5 chu kỳ qua 6 mô hình & shutdown | `[TC-M2-22] Repeated GPU upload/release lifecycle across furniture models: PASS` | **PASS** |
| **TC-M2-23** | OpenGL Errors & Safe Exit | `GL_NO_ERROR`, thoát mã `0` | `[M2-SMOKE] Frames rendered: 10`, `OpenGL errors: NONE`, `M2_SMOKE_EXIT=0`, `M2_UNIT_EXIT=0` (chạy độc lập ngoài PATH WinLibs) | **PASS** |
| **TC-M2-MANUAL** | Geometry Playground GUI | Kiểm thử tương tác thủ công M2 | Chưa được người dùng chạy kiểm chứng trực quan | **NOT VERIFIED** |

---

## 3. Tóm tắt trạng thái M0 & M1 (Giữ nguyên không tự ý đổi `NOT VERIFIED`)

- **M0 (`TC-M0-01`..`TC-M0-07`)**: **7/7 PASS** (tự động). `TC-M0-MSVC`: **NOT RUN**. `TC-M0-MANUAL`: **NOT VERIFIED**.
- **M1 (`TC-M1-01`..`TC-M1-16`)**: **16/16 PASS** (tự động). `TC-M1-MANUAL`: **NOT VERIFIED**.
