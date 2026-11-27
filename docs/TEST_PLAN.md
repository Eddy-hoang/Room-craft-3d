# Kế hoạch kiểm thử — RoomCraft 3D (Test Plan)

## 1. Chiến lược kiểm thử

Dự án áp dụng mô hình kiểm thử 3 tầng:
1. **Unit Testing thuần CPU**:
   - `RoomCraft3D_UnitTests` (`M1_UnitTests`): `Transform`, `Camera` (bao gồm `fitBoundingBox`), `MeshValidation`.
   - `RoomCraft3D_GeometryUnitTests` (`M2_GeometryUnitTests`): Primitives, Bézier Curve/Surface, Revolution Surface, BoundingBox, 6 Procedural Furniture models.
   - `RoomCraft3D_AssetUnitTests` (`M3_AssetUnitTests`): `ModelLoader` (Assimp OBJ/MTL), `AssetNormalizer`, `FurnitureCatalog`, kiểm tra toàn bộ 8 file Blender asset và fixtures mà không cần OpenGL context.
2. **Integration & Runtime Smoke Testing**:
   - `--smoke-test` (`M0_SmokeTest`), `--m1-smoke-test` (`M1_IntegrationSmokeTest`), `--m2-smoke-test` (`M2_IntegrationSmokeTest`), `--m3-smoke-test` (`M3_IntegrationSmokeTest`): Kiểm thử trên ngữ cảnh OpenGL 3.3 Core thật (shader, VAO/VBO/EBO, Texture/UV orientation, fallback texture, multi-material, cache reuse, transactional reload, FBO rendering 14/14 asset, giải phóng tài nguyên GPU, `GL_NO_ERROR`).
3. **Manual GUI Testing**: Người dùng chạy trực tiếp `RoomCraft3D.exe` và xác nhận checklist trực quan cho từng milestone.

## 2. Danh mục Test Case — Regression M0, M1, M2

- **M0 (`TC-M0-01`..`TC-M0-07`)**: Configure, Build Debug, OpenGL 3.3 Core, Render Triangle, Resize `960x540`, Dear ImGui, Clean Exit.
- **M1 (`TC-M1-01`..`TC-M1-16`)**: M0 Regression, Shader valid/invalid, Indexed Mesh & Invalid Indices, Transform (T/R/S), Camera (Perspective/Orthographic), Depth Testing độc lập thứ tự draw, Multi-cube FBO, Framebuffer completeness/resize/`0x0`, RAII move lifecycle, Zero GL errors.
- **M2 (`TC-M2-01`..`TC-M2-23`)**: Regression M0/M1, 7 Primitives, Bézier Curve/Surface, Revolution Surface, Normal/UV/Index/Degenerate validation, Invalid params, BoundingBox, Hierarchy, 6 Furniture models, Mesh regeneration, GPU rendering, Wireframe restoration, GPU lifecycle, Zero GL errors.

## 3. Danh mục Test Case — Milestone M3 (Blender Asset Pipeline)

| Test ID | Hạng mục | Kịch bản kiểm thử | Kết quả mong đợi |
|---|---|---|---|
| **TC-M3-01** | Regression M0/M1/M2 | Chạy lại `M0_SmokeTest`, `M1_UnitTests`, `M1_IntegrationSmokeTest`, `M2_GeometryUnitTests`, `M2_IntegrationSmokeTest` | Toàn bộ 5 bộ regression đạt `PASS` |
| **TC-M3-02** | Build & Link Assimp + stb | CMake configure + build `roomcraft_assets`, `roomcraft_engine`, `RoomCraft3D`, `RoomCraft3D_AssetUnitTests` | `0` errors, `0` project warnings; binary độc lập không phụ thuộc DLL MinGW ngoài |
| **TC-M3-03** | OBJ Import cơ bản | Nạp `modern_sofa.obj` qua `ModelLoader::loadFromFile` | Có đầy đủ `subMeshes`, `materials`, đỉnh/chỉ mục hợp lệ, `validate() == true` |
| **TC-M3-04** | Triangulation & GenNormals/UV | Nạp fixture `quad_no_normals_no_uv.obj` (mặt tứ giác không normal, không UV) | Tự động chia thành 2 tam giác (6 indices), sinh normal đơn vị hợp lệ, UV hữu hạn |
| **TC-M3-05** | Multi-material OBJ/MTL | Nạp fixture `multi_material.obj` gồm 2 nhóm vật liệu (`MatRed`, `MatBlue`) | Tách thành 2 sub-mesh ứng với 2 vật liệu đúng màu khuếch tán `Kd` |
| **TC-M3-06** | Node Transform & Normal Matrix | Kiểm tra `convertRowMajor4x4` và biến đổi pháp tuyến bằng $(M^{-1})^T$ với non-uniform scale | Vị trí và pháp tuyến bảo toàn trực giao với tiếp tuyến, độ dài normal `= 1` |
| **TC-M3-07** | Đường dẫn texture tương đối | Nạp `textured_quad.obj` + `modern_sofa.obj` có `map_Kd` tương đối | Giải phân đường dẫn texture theo thư mục chứa `.obj`, không chứa đường dẫn tuyệt đối cứng |
| **TC-M3-08** | Xử lý file không tồn tại | Gọi `ModelLoader::loadFromFile` với đường dẫn không tồn tại | Trả về `false`, điền thông báo lỗi rõ ràng, không crash |
| **TC-M3-09** | Xử lý file OBJ rỗng / lỗi | Nạp fixture `invalid_empty.obj` | Trả về `false`, từ chối sạch, không để lại dữ liệu rác |
| **TC-M3-10** | Normalization: Pivot tâm đáy | Chuẩn hóa mesh đặt lệch `(10, 5, -3)` qua `AssetNormalizer::normalize` | Sau chuẩn hóa: `min.y == 0`, `(min.x + max.x)/2 == 0`, `(min.z + max.z)/2 == 0` |
| **TC-M3-11** | Normalization: Idempotent | Gọi `AssetNormalizer::normalize` 2 lần liên tiếp | Kết quả lần 2 trùng khớp lần 1 (không bị trôi tọa độ/kích thước) |
| **TC-M3-12** | Normalization: Unit Scale | Chuẩn hóa cuboid phóng to `100×` (đơn vị cm) với `unitScale = 0.01f` | Kích thước về đúng mét `(2.0, 0.8, 0.9)` và giữ nguyên pivot tâm đáy |
| **TC-M3-13** | Normalization: Bảo toàn Normals | So sánh pháp tuyến trước và sau `AssetNormalizer::normalize` | Pháp tuyến giữ độ dài `= 1` và đúng hướng |
| **TC-M3-14** | Camera Auto-Fit | Gọi `Camera::fitBoundingBox()` cho AABB nhỏ (`0.3m`) và lớn (`4.0m`) ở Perspective & Orthographic | Mục tiêu tại tâm BBox, 8 góc AABB chiếu vào bên trong NDC `[-1, 1]` |
| **TC-M3-15** | FurnitureCatalog Registry | Khởi tạo `FurnitureCatalog` mặc định | Chứa đủ 14 mục (6 Procedural `procedural-*` + 8 Blender `blender-*`), ID duy nhất, tìm kiếm/lọc đúng |
| **TC-M3-16** | Kiểm tra 8 Blender Assets (CPU) | Nạp + chuẩn hóa + kiểm tra polygon budget cả 8 file trong `assets/models/` | 8/8 file hợp lệ, kích thước thực tế hợp lý, số tam giác `<=` ngân sách từng mẫu |
| **TC-M3-17** | Texture UV Orientation (GPU) | Render `textured_quad.obj` với `asymmetric_uv.png` (nửa trên Đỏ, nửa dưới Xanh) vào FBO | Đọc pixel FBO: nửa trên `RGB ≈ (255,0,0)`, nửa dưới `RGB ≈ (0,0,255)` (không lật ngược) |
| **TC-M3-18** | Missing Texture Fallback (GPU) | Render `missing_tex_quad.obj` trỏ tới texture không tồn tại | Không crash, tự kích hoạt fallback checkerboard texture từ `TextureManager` |
| **TC-M3-19** | Texture Cache Reuse (GPU) | Gọi `TextureManager::getOrLoad` 2 lần cho cùng file texture | Trả về cùng `GLuint` handle, không tăng số texture trong cache |
| **TC-M3-20** | Asset GPU Cache Reuse | Gọi `AssetManager::getOrLoad` 2 lần cho cùng `assetId` | Trả về cùng con trỏ `GpuAsset*` và giữ nguyên VAO/VBO handle |
| **TC-M3-21** | Transactional Reload (GPU) | Reload thành công (cập nhật asset) và reload thất bại khi nguồn lỗi | Thành công thì hoán đổi asset mới; thất bại thì **giữ nguyên** asset hợp lệ trước đó |
| **TC-M3-22** | Render 6 Procedural Assets (GPU) | Nạp qua `AssetManager`, Auto-Fit và render từng mẫu Procedural vào FBO | 6/6 render thành công, góc nền sạch (`cornerBg=OK`), diện tích phủ pixel thực tế `> 0` |
| **TC-M3-23** | Render 8 Blender Assets (GPU) | Nạp qua `AssetManager`, Auto-Fit và render từng mẫu Blender vào FBO | 8/8 render thành công, góc nền sạch (`cornerBg=OK`), diện tích phủ pixel thực tế `> 0` |
| **TC-M3-24** | Multi-Material Render (GPU) | Render `multi_material.obj` vào FBO | Đọc pixel trái = Đỏ `(255,0,0)`, phải = Xanh dương `(0,0,255)` |
| **TC-M3-25** | M3 Asset Browser Frame Loop | Chạy vòng lặp render 10 khung hình với ImGui + FBO preview ở chế độ M3 | Không lỗi, thống kê drawCalls/vertices/triangles thực tế khớp asset đang chọn |
| **TC-M3-26** | GPU Resource Cleanup | Giải phóng `AssetManager`, `TextureManager`, `Framebuffer`, `Shader`, ImGui trước khi hủy GL context | Cache về `0`, không crash, không double-delete |
| **TC-M3-27** | OpenGL Errors & Standalone Exit | Kiểm tra `glGetError()` và chạy binary độc lập ngoài PATH WinLibs | `OpenGL errors: NONE`, mã thoát `0` |
| **TC-M3-MANUAL** | Furniture Asset Browser GUI | Người dùng chạy `RoomCraft3D.exe` kiểm tra trực quan 14 mẫu, search/filter, Fit Model, Wireframe, Normals, Reload Asset, chuyển mode M3/M2/M1 | Xác nhận trực quan bởi người dùng |
