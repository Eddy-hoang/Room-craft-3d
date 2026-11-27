# Theo dõi tiến độ dự án — RoomCraft 3D (Progress)

Cập nhật lần cuối: **2026-11-27**

## Tổng quan các Milestone

| Milestone | Mô tả | Trạng thái | Ghi chú |
|---|---|---|---|
| **M0** | Project Foundation | **VERIFIED** | 7/7 tự động PASS; `TC-M0-MANUAL` giữ `NOT VERIFIED` |
| **M1** | Core Graphics Engine | **VERIFIED** | 16/16 tự động PASS; `TC-M1-MANUAL` giữ `NOT VERIFIED` |
| **M2** | Procedural Geometry & Modeling | **VERIFIED** | 23/23 tự động PASS (`5/5 CTest`); `TC-M2-MANUAL` giữ `NOT VERIFIED` |
| **M3** | Blender Asset Pipeline | **VERIFIED** | 27/27 tự động PASS (`7/7 CTest`, `1.45s`); `TC-M3-MANUAL` giữ `NOT VERIFIED` chờ người dùng xác nhận |
| **M4** | Scene Management & Room Editor | TODO | Chờ xác nhận cổng M3 |
| **M5** | Camera, Transformation & Object Picking | TODO | Chưa bắt đầu |
| **M6** | Professional Editor UI/UX | TODO | Chưa bắt đầu |
| **M7** | Lighting, Materials & Visual Quality | TODO | Chưa bắt đầu |
| **M8** | Collision Detection & Project Persistence | TODO | Chưa bắt đầu |
| **M9** | Testing, Optimization & Release | TODO | Chưa bắt đầu |

---

## Chi tiết công việc Milestone M3 — Blender Asset Pipeline

| Task ID | Công việc | Ưu tiên | Trạng thái | Bằng chứng / Ghi chú |
|---|---|---|---|---|
| **M3-T01** | Audit repo và regression M0/M1/M2 | P0 | DONE | Toàn bộ `5/5` bộ test M0–M2 PASS trước và sau M3 |
| **M3-T02** | Cài đặt & tích hợp Assimp (`v6.0.6`) | P0 | DONE | CMake FetchContent release archive khóa commit `a79ff6bb...`, static OBJ-only; cập nhật `vcpkg.json` |
| **M3-T03** | Cài đặt & tích hợp `stb_image` | P0 | DONE | CMake FetchContent commit `6b5799ba...`, biên dịch implementation trong `Texture.cpp` |
| **M3-T04** | Thiết kế `AssetData` (Unified CPU representation) | P0 | DONE | `AssetData.hpp` tách CPU trong `roomcraft_assets` (SubMeshData, MaterialData, BoundingBox) |
| **M3-T05** | `ModelLoader` (Assimp OBJ + MTL) | P0 | DONE | `ModelLoader.hpp/cpp`: node recursion, normal inverse-transpose baking, relative texture paths |
| **M3-T06** | `AssetNormalizer` (Unit scale & Bottom-center pivot) | P0 | DONE | `AssetNormalizer.hpp/cpp`: scale đơn vị + `min.y = 0`, tâm XZ = `0`, cập nhật AABB |
| **M3-T07** | `Texture` & `TextureManager` (GPU + Cache + Fallback) | P0 | DONE | `Texture.hpp/cpp`, `TextureManager.hpp/cpp`: flip-Y, mipmap, RAII move-only, fallback checkerboard tím-đen |
| **M3-T08** | `Material` & Shader hỗ trợ Diffuse Texture | P0 | DONE | `Material.hpp`, `mesh.frag` (`uDiffuseMap`, `uHasTexture`), `Renderer::drawMeshTextured()` |
| **M3-T09** | `FurnitureCatalog` (Unified Procedural + Blender) | P0 | DONE | `FurnitureCatalog.hpp/cpp`: 14 mục (6 `procedural-*` + 8 `blender-*`) với stable ID & metadata |
| **M3-T10** | `AssetManager` (Cache + Transactional Reload) | P0 | DONE | `AssetManager.hpp/cpp`: load theo ID, tái sử dụng GPU handle, swap an toàn khi reload |
| **M3-T11** | Camera Auto-Fit (`fitBoundingBox`) | P0 | DONE | `Camera::fitBoundingBox()` cho cả Perspective & Orthographic + nút `Fit Model` |
| **M3-T12** | Môi trường Blender 5.2.3 LTS & Script `bpy` | P0 | DONE | Giải nén portable `Blender 5.2.3 LTS` từ `.msi`, tạo 10 script `blender/scripts/*.py` |
| **M3-T13** | Tạo & xuất 8 Blender furniture assets | P0 | DONE | 8 `.blend` (`blender/source/`) + 8 `.obj`/`.mtl` + 11 `.png` (`assets/models/`), 100% dưới budget tam giác |
| **M3-T14** | Furniture Asset Browser UI (3-panel Dear ImGui) | P0 | DONE | `Asset Library` (search/filter), `3D Asset Preview` (`Fit Model`), `Asset Inspector` (`Reload Asset`, stats, overlays) |
| **M3-T15** | Chuyển đổi chế độ M3 / M2 / M1 | P0 | DONE | Menu chuyển chế độ bảo toàn đầy đủ M2 Geometry Playground và M1 Reference Scene |
| **M3-T16** | Unit Tests thuần CPU (`M3_AssetUnitTests`) | P0 | DONE | `AssetTestMain.cpp`, `ModelLoaderTests.cpp`, `AssetNormalizerTests.cpp`, `AssetCatalogTests.cpp` (18/18 checks PASS) |
| **M3-T17** | GPU Integration Smoke Test (`M3_IntegrationSmokeTest`) | P0 | DONE | `--m3-smoke-test` kiểm chứng UV orientation, fallback, multi-material, cache, reload, render 14/14 asset, `GL_NO_ERROR` |
| **M3-T18** | Thumbnail Preview (ảnh thu nhỏ pre-rendered) | P1 | TODO | Tùy chọn nâng cao; hiện Asset Library dùng danh sách/thẻ có nhãn nguồn rõ ràng |
| **M3-T19** | Hỗ trợ định dạng glTF 2.0 / `.glb` | P2 | TODO | Tùy chọn mở rộng; M3 khóa OBJ/MTL để giữ build Assimp gọn nhẹ |
| **M3-T20** | Tài liệu Asset Pipeline & Blender Guide | P0 | DONE | Tạo mới `docs/ASSET_PIPELINE.md`, `docs/BLENDER_GUIDE.md`, `blender/README.md` |
| **M3-T21** | Cập nhật toàn bộ tài liệu dự án & Changelog | P0 | DONE | Cập nhật `README.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `GRAPHICS_THEORY.md`, `PROGRESS.md`, `TEST_PLAN.md`, `TEST_RESULTS.md`, `KNOWN_ISSUES.md`, `BUILD_GUIDE.md`, `ASSET_LICENSES.md`, `CHANGELOG.md` |

---

## Tóm tắt Milestone đã hoàn thành trước đó (M0–M2)

- **M0 (Project Foundation)**: `M0_SmokeTest` (7/7 PASS); `TC-M0-MSVC` (`NOT RUN`), `TC-M0-MANUAL` (`NOT VERIFIED`).
- **M1 (Core Graphics Engine)**: `M1_UnitTests` (23/23 assertions PASS) + `M1_IntegrationSmokeTest` (10/10 GPU checks PASS); `TC-M1-MANUAL` (`NOT VERIFIED`).
- **M2 (Procedural Geometry & Modeling)**: `M2_GeometryUnitTests` (18/18 checks PASS) + `M2_IntegrationSmokeTest` (`TC-M2-19..23` PASS); `TC-M2-MANUAL` (`NOT VERIFIED`).
