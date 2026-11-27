# Theo dõi tiến độ dự án — RoomCraft 3D (Progress)

Cập nhật lần cuối: **2026-11-27**

## Tổng quan các Milestone

| Milestone | Mô tả | Trạng thái | Ghi chú |
|---|---|---|---|
| **M0** | Project Foundation | **VERIFIED** | 7/7 tự động PASS; `TC-M0-MANUAL` giữ `NOT VERIFIED` |
| **M1** | Core Graphics Engine | **VERIFIED** | 16/16 tự động PASS; `TC-M1-MANUAL` giữ `NOT VERIFIED` |
| **M2** | Procedural Geometry & Modeling | **VERIFIED** | 23/23 tự động PASS (`5/5 CTest`); `TC-M2-MANUAL` giữ `NOT VERIFIED` chờ người dùng xác nhận |
| **M3** | Blender Asset Pipeline | TODO | Chờ xác nhận cổng M2 |
| **M4** | Scene Management & Room Editor | TODO | Chưa bắt đầu |
| **M5** | Camera, Transformation & Object Picking | TODO | Chưa bắt đầu |
| **M6** | Professional Editor UI/UX | TODO | Chưa bắt đầu |
| **M7** | Lighting, Materials & Visual Quality | TODO | Chưa bắt đầu |
| **M8** | Collision Detection & Project Persistence | TODO | Chưa bắt đầu |
| **M9** | Testing, Optimization & Release | TODO | Chưa bắt đầu |

## Chi tiết công việc Milestone M2 — Procedural Geometry & Modeling

| Task ID | Công việc | Trạng thái | Bằng chứng / Ghi chú |
|---|---|---|---|
| **M2-T01** | Audit và regression M0/M1 | DONE | `M0_SmokeTest`, `M1_UnitTests`, `M1_IntegrationSmokeTest` PASS trước và sau M2 |
| **M2-T02** | Thiết kế `MeshData` | DONE | `MeshData.hpp/cpp` thuần CPU (không GPU handle), kiểm tra normal/UV/degenerate triangles |
| **M2-T03** | Plane Generator | DONE | `PrimitiveGenerator::createPlane` (XZ, `+Y`, CCW, subdivisions tùy chỉnh) |
| **M2-T04** | Cube/Cuboid Generator | DONE | `PrimitiveGenerator::createCube/createCuboid` (24 verts, 36 indices, pivot tâm đáy) |
| **M2-T05** | Cylinder Generator | DONE | `PrimitiveGenerator::createCylinder` (tách đỉnh thân và 2 nắp, UV seam) |
| **M2-T06** | Sphere Generator | DONE | `PrimitiveGenerator::createSphere` (tọa độ cầu, pháp tuyến giải tích, xử lý 2 cực) |
| **M2-T07** | Cone/Frustum Generator | DONE | `PrimitiveGenerator::createCone/createFrustum` (pháp tuyến nghiêng, apex không suy biến) |
| **M2-T08** | Bézier Curve | DONE | `BezierCurve.hpp/cpp` (Bernstein + de Casteljau + đạo hàm + control polygon) |
| **M2-T09** | Bézier Surface | DONE | `BezierSurface.hpp/cpp` (4×4 patch, đạo hàm riêng, normal, control net) |
| **M2-T10** | Revolution Surface | DONE | `RevolutionSurface.hpp/cpp` (profile 2D hoặc Bézier Curve, xử lý `r=0`, nắp phẳng) |
| **M2-T11** | Normal và UV Generation | DONE | Triển khai riêng theo bản chất hình học từng loại, xác thực đơn vị & hữu hạn |
| **M2-T12** | Bounding Box Calculation | DONE | `BoundingBox.hpp/cpp` (expand, center, size, `transformed(mat4)`) |
| **M2-T13** | Hierarchical Furniture Modeling | DONE | `FurniturePart` + `FurnitureModel`, `M_world_part = M_parent * M_local_part`, root tâm đáy `y=0` |
| **M2-T14** | Furniture Generator | DONE | 5 mẫu bắt buộc (Desk, Chair, Bookshelf, Desk Lamp, Vase) |
| **M2-T15** | Geometry Playground UI | DONE | Danh mục, tham số, Regenerate, Reset, thống kê thời gian sinh/đỉnh/tam giác/BBox thật |
| **M2-T16** | Solid/Wireframe View | DONE | `Renderer::setWireframeMode` + khôi phục `GL_FILL` ở `endFrame()` (`TC-M2-21`) |
| **M2-T17** | Normal Visualization | DONE | `GeometryUtils::buildVertexNormalLines` + checkbox `Vertex Normals` trong Playground |
| **M2-T18** | Curved Chair (Optional P1) | DONE | `FurnitureGenerator::createCurvedChair` kết hợp tựa lưng Bicubic Bézier Surface |
| **M2-T19** | Unit & GPU Integration Testing | DONE | `M2_GeometryUnitTests` (18/18 PASS) + `M2_IntegrationSmokeTest` (`TC-M2-19..23` PASS) |
| **M2-T20** | Documentation | DONE | Tạo mới `docs/GEOMETRY_ALGORITHMS.md` và cập nhật toàn bộ 10 tài liệu dự án |
