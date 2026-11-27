# Kế hoạch kiểm thử — RoomCraft 3D (Test Plan)

## 1. Chiến lược kiểm thử

Dự án áp dụng mô hình kiểm thử 3 tầng:
1. **Unit Testing thuần CPU** (`RoomCraft3D_UnitTests`, `RoomCraft3D_GeometryUnitTests`): Kiểm thử độc lập toán học (`Transform`, `Camera`, `BoundingBox`), kiểm tra dữ liệu lưới và toàn bộ thuật toán sinh hình học M2 (`PrimitiveGenerator`, `BezierCurve`, `BezierSurface`, `RevolutionSurface`, `FurnitureGenerator`) không cần OpenGL context.
2. **Integration & Runtime Smoke Testing** (`--smoke-test`, `--m1-smoke-test`, `--m2-smoke-test`): Kiểm thử trên ngữ cảnh OpenGL 3.3 Core thật (shader, VAO/VBO/EBO, Depth Testing, FBO, regeneration, render từng nhóm hình học M2, khôi phục trạng thái wireframe, vòng đời GPU).
3. **Manual GUI Testing**: Người dùng chạy trực tiếp `RoomCraft3D.exe` và xác nhận checklist trực quan.

## 2. Danh mục Test Case — Milestone M0 & M1 (Regression)

- **M0 (`TC-M0-01`..`TC-M0-07`)**: Configure, Build Debug, OpenGL 3.3 Core, Render Triangle, Resize `960x540`, Dear ImGui, Clean Exit.
- **M1 (`TC-M1-01`..`TC-M1-16`)**: M0 Regression, Shader valid/invalid, Indexed Mesh & Invalid Indices, Transform (T/R/S), Camera (Perspective/Orthographic), Depth Testing độc lập thứ tự draw, Multi-cube FBO, Framebuffer completeness/resize/`0x0`, RAII move lifecycle, Zero GL errors.

## 3. Danh mục Test Case — Milestone M2 (Procedural Geometry & Modeling)

| Test ID | Hạng mục | Kịch bản kiểm thử | Kết quả mong đợi |
|---|---|---|---|
| **TC-M2-01** | Regression M0/M1 | Chạy lại `M0_SmokeTest`, `M1_UnitTests`, `M1_IntegrationSmokeTest` | Toàn bộ 3 bộ regression trước đạt `PASS` |
| **TC-M2-02** | Plane | Sinh mặt phẳng `3×2m` chia lưới `2×3` | Đúng kích thước, `y=0`, pháp tuyến `+Y`, winding CCW, UV `[0,1]` |
| **TC-M2-03** | Cube / Cuboid | Sinh Cuboid `1.6×0.9×0.7m` và Cube `1.2m` | 24 vertices, 36 indices, pivot tâm đáy (`min.y=0`), pháp tuyến phẳng hướng ngoài |
| **TC-M2-04** | Cylinder | Sinh Cylinder có nắp & không nắp (`16` segments) | Khớp công thức vertex/index từng phần, UV seam (`u=0` vs `u=1`), pháp tuyến hướng ngoài |
| **TC-M2-05** | Sphere | Sinh Sphere `r=0.75m`, `12×20` segments | Đúng bán kính, pháp tuyến giải tích hướng tâm cầu, xử lý 2 cực không sinh tam giác suy biến |
| **TC-M2-06** | Cone / Frustum | Sinh Cone (`r=0.6, h=1.2`) và Frustum (`0.6 -> 0.25, h=1.0`) | Apex hợp lệ không NaN/suy biến, pháp tuyến mặt bên nghiêng đúng hướng |
| **TC-M2-07** | Bézier Curve Endpoints | Đánh giá tại `t=0`, `t=1` và đối chiếu Bernstein với de Casteljau tại `t=0.37` | `B(0)==P0`, `B(1)==P3`, Bernstein khớp de Casteljau |
| **TC-M2-08** | Bézier Curve Derivatives | Đạo hàm `B'(0)=3(P1-P0)`, `B'(1)=3(P3-P2)`, tính thẳng hàng, lấy mẫu `10/25` đoạn, từ chối `t` lỗi | Đạo hàm chính xác, điểm thẳng hàng giữ thẳng hàng, `sample(N)` trả `N+1` điểm |
| **TC-M2-09** | Bézier Surface Corners | Kiểm tra 4 góc `(0,0)`, `(1,0)`, `(0,1)`, `(1,1)` của mặt cong `4×4` | Khớp chính xác `P[0][0]`, `P[3][0]`, `P[0][3]`, `P[3][3]` |
| **TC-M2-10** | Bézier Surface Normals & Topology | Kiểm tra flat patch (`+Y`), curved patch (`(uSeg+1)*(vSeg+1)` đỉnh), winding khớp pháp tuyến, từ chối patch suy biến | Đúng topology, pháp tuyến chuẩn hóa, từ chối segment lỗi và patch sụp về 1 điểm |
| **TC-M2-11** | Revolution Surface | Xoay profile Bézier + profile có cực `r=0` quanh `+Y` | Đúng rings, UV seam, nắp đáy phẳng, không tam giác suy biến ở `r=0` |
| **TC-M2-12** | Normal Validation | Kiểm tra `|length(normal) - 1| <= 1e-2` và hữu hạn trên mọi surface mesh | 100% vector pháp tuyến chuẩn hóa và hữu hạn |
| **TC-M2-13** | UV Validation | Kiểm tra UV hữu hạn trong `[0,1]` và UV seam tách biệt `u=0/1` | 100% tọa độ UV hợp lệ |
| **TC-M2-14** | Index Bounds & Degenerate Check | Kiểm tra `MeshData::validate()` (chỉ mục `< vertexCount`, không trùng chỉ mục, diện tích tam giác `> 0`) | Toàn bộ mesh sinh ra không có lỗi biên hoặc tam giác diện tích 0 |
| **TC-M2-15** | Invalid Parameter Handling | Truyền kích thước âm/0/NaN, segment ngoài biên, bán kính revolution âm, tham số furniture lỗi | Từ chối sạch, trả về `false` kèm thông báo lỗi |
| **TC-M2-16** | BoundingBox | Kiểm tra `expand`, `min/max/center/size` và `transformed()` khi xoay 90° quanh `+Y` | Kích thước chính xác; xoay 90° hoán đổi đúng chiều X ↔ Z |
| **TC-M2-17** | Furniture Hierarchy | Áp dụng `M_parent` (tịnh tiến + xoay 90° Y) lên toàn bộ parts của `Desk` | `M_world_part = M_parent * M_local_part` dịch chuyển và xoay đồng bộ cả 6 thành phần, giữ `min.y = 0` |
| **TC-M2-18** | Procedural Furniture Models | Sinh và xác thực tối thiểu 5 (thực tế 6) loại nội thất: Desk, Chair, Bookshelf, Desk Lamp, Vase, Curved Chair | Toàn bộ 6 mô hình hợp lệ, đứng trên mặt sàn `y = 0`, kích thước khớp tham số |
| **TC-M2-19** | Mesh Regeneration | Tái sinh Cylinder `8 -> 32` segments và thử tham số lỗi (`r = -1`) | Số đỉnh tăng đúng khi tăng segment; khi gặp lỗi giữ nguyên GPU mesh hợp lệ trước đó |
| **TC-M2-20** | GPU Rendering Categories | Upload & render từng nhóm (Primitives, Bézier Surface, Revolution Surface, 6 Furniture models) vào FBO | FBO complete, đúng draw call, góc nền sạch (`cornerBg=OK`) và vùng chiếu BBox phủ pixel thực tế |
| **TC-M2-21** | Wireframe State Restoration | Bật `setWireframeMode(true)` vẽ 1 pass rồi gọi `endFrame()` | `GL_POLYGON_MODE` là `GL_LINE` trong pass và tự khôi phục `GL_FILL` sau `endFrame()` |
| **TC-M2-22** | GPU Resource Lifecycle | Tái sinh liên tục nhiều vòng qua 6 mô hình nội thất + giải phóng trước `onClose` | Không rò rỉ handle, không crash, không double-delete |
| **TC-M2-23** | OpenGL Errors & Shutdown | Quét `glGetError()` sau suite và 10 khung hình render | `OpenGL errors: NONE`, thoát với mã `0` |
| **TC-M2-MANUAL** | Geometry Playground GUI | Người dùng chạy `RoomCraft3D.exe` kiểm tra tương tác tất cả danh mục, tham số, wireframe, normal overlay | Xác nhận trực quan bởi người dùng |
