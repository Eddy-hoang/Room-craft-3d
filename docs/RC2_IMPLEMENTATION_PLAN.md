# Kế hoạch Triển khai RC2 — Nâng cao Chất lượng Thị giác (`v1.0.0-rc2` · Giai đoạn Tinh chỉnh thuộc M9)

- **Phiên bản nguồn**: `v1.0.0-rc1` (Independent QA Overall Visual Quality `5.1/10`, Release Gate `NO-GO`)
- **Phiên bản mục tiêu**: `v1.0.0-rc2` (M9 Visual Quality Refinement — không tạo M10, không đổi tech stack)
- **Định hướng thiết kế (Design DNA)**: **Scandinavian Modern × Stylized Real-time Rendering**
  - Bảng màu chuẩn: Warm White `#DAD5CA`, Natural Oak `#B58D60`, Soft Taupe `#827A6E`, Slate Blue `#465A6E`, Sage Green `#70866B`, Charcoal `#25282E`.

---

## 1. Sổ đăng ký Lỗi Thị giác RC1 (Visual Defect Register — RC2.0)

| Bug ID | Mức độ | Tái lập trên RC1 | Nguyên nhân gốc xác nhận trong mã nguồn | Trạng thái RC2 |
|---|---|---|---|---|
| **BUG-01** | P0 | Chụp `12_shadows_off` vs `04_lighting_daylight`, khuếch đại hiệu 4× → vân Moiré đồng tâm khắp 2 tường | `SceneRenderer::renderShadowPass` vẽ tường không có `GL_CULL_FACE(GL_FRONT)` / `glPolygonOffset`; `ShadowMap` dùng `GL_NEAREST`; thiếu normal-offset bias | **FIXED & VERIFIED** (`meanDiff = 0.938 < 1.5`) |
| **BUG-02** | P0 | `Warm Interior` có `7,620 px (8.47%)`, `Gaming Workspace` có `5,905 px (5.98%)` cháy sáng `≥ 248` | `pointIntensity = 2.0` đặt sát tường `(-1.0, 2.3, 0.7)` với suy giảm yếu `(1.0, 0.18, 0.12)` và `mesh.frag` không có tone mapping trước `displayColor()` | **FIXED & VERIFIED** (`0 px = 0.00%` trên mọi preset) |
| **BUG-03** | P0 | `07_lighting_evening` zoom 2× cho thấy bóng tủ răng cưa và tách nhẹ dưới chân | `ShadowMap` dùng `GL_NEAREST` không bật `GL_COMPARE_REF_TO_TEXTURE`; frustum không vuông và không texel-snapped | **FIXED & VERIFIED** (`sampler2DShadow` + 5-tap bilinear PCF + texel-snap) |
| **BUG-04** | P0 | Mở `Gaming Workspace` báo ngay `Warnings: 1 overlap, 1 bounds` | `Desk Lamp` (`x=0.52`) đè `Computer Setup` (`x=-0.10`); `Indoor Plant` (`x=-1.45`) có OBJ thật rộng `0.89 m` vượt tường Tây `x=-2.0` | **FIXED & VERIFIED** (`0 overlaps, 0 bounds` trên 3/3 preset) |
| **BUG-05** | P1 | Toàn bộ 8/8 file `.mtl` Blender đều xuất `Ks 0.500 0.500 0.500` kể cả vải, thảm, đất | Mặc định Principled BSDF của Blender xuất specular 0.5 đồng loạt | **FIXED & VERIFIED** (phân tách vải/đất/gỗ/gốm/kim loại, bảo toàn `ComputerAluminum`) |
| **BUG-06** | P1 | `FurnitureCatalog` ghi `blender-indoor-plant` là `0.58 × 1.15 × 0.58 m` trong khi OBJ thật là `0.89 × 1.08 × 0.87 m` | Metadata không khớp bounding box chuẩn hóa thực tế (+53%) | **FIXED & VERIFIED** (đồng bộ cả 8 asset Blender, test sai số `≤ 0.03 m`) |
| **BUG-07** | P1 | `computeRoomCornerAO` áp dụng lên cả đồ nội thất và 2 mép sàn Cutaway trống | Không truyền mặt nạ tường hiển thị (`uVisibleWalls`) và cờ bề mặt phòng (`uIsRoomSurface`) | **FIXED & VERIFIED** |
| **BUG-08** | P1 | 4/6 thẻ C++ cắt chữ (`Procedural Bo...`, `Procedural De...`, `Procedural Ce...`, `Procedural Cu...`) | Tiền tố `"Procedural "` thừa dù đã có badge `[C++]`; `m_hierarchyRatio = 0.36` chật | **FIXED & VERIFIED** (tên gọn + giữ nguyên `assetId` + `ratio = 0.43`) |

---

## 2. Lộ trình Triển khai theo Cổng Chất lượng (Quality Gates)

| Giai đoạn | Phạm vi | Trạng thái |
|---|---|---|
| **RC2.0** | Bảo lưu RC1 (`dist/...rc1`, `docs/evidence/rc2/rc1_baseline/`), tái lập toàn bộ 8 lỗi & đo baseline | **DONE** |
| **RC2.1 (P0)** | `ShadowMap` hardware `GL_COMPARE_REF_TO_TEXTURE` + `GL_LINEAR`, frustum vuông texel-snapped, `GL_FRONT` culling riêng cho tường đặc + `glPolygonOffset`, 5-tap bilinear PCF + normal-offset bias | **DONE** |
| **RC2.2 (P0)** | Kiểm toán pipeline màu tuyến tính → ACES Filmic Tone Mapping trước `displayColor()` (chỉ 1 lần sRGB), cân chỉnh 4 lighting presets, ánh sáng bán cầu 2 tông, sửa `computeRoomCornerAO` | **DONE** |
| **RC2.3 (P0)** | `Room::collectVisibleCornerPoints()` + `CameraController::fitRoomView()` căn tâm NDC 3 vòng (50–56% silhouette, 0 cắt phòng), Studio radial gradient background, ẩn mặc định Grid/Axes, sửa `Gaming Workspace` & `FurnitureCatalog` | **DONE** |
| **GATE A** | Kiểm chứng 19/19 CTest + test P0 mới (`RC2-P0-PRESET`, `RC2-P0-CAM`, `BUG-06`), chụp `docs/evidence/rc2/p0_gate_a/`, kiểm tra 0 wall acne & 0.00% cháy sáng | **PASS** |
| **RC2.4 (P1)** | Chuẩn hóa 8 file `.mtl` + vật liệu Procedural, Energy-Aware Blinn-Phong + Schlick Fresnel, Anisotropic Filtering an toàn (Core `glGetStringi`), Soft Contact Grounding Shadows (OBB XZ chỉ tác động ambient) | **DONE** |
| **RC2.5 (P1)** | Baseboards theo 4 tường, Cửa sổ kiến trúc trang trí trên tường Bắc (`showWindow`, tương thích ngược JSON), sàn gỗ chống moiré `fwidth()`, nâng cấp `Procedural Desk` (cạnh vát + ngăn kéo + tay nắm đồng) & `Procedural Bookshelf` (plinth + sách & hộp procedural), cân chỉnh bố cục 3 phòng mẫu | **DONE** |
| **RC2.6 (P1)** | Tên thẻ gọn (giữ `assetId`), tối ưu thumbnail (`1.16f` + studio tone map), `m_hierarchyRatio = 0.43`, sửa cắt chữ `Validation Mode`, thêm Passive 3D Orientation Indicator góc trên-phải | **DONE** |
| **GATE B & RC2.7** | 19/19 CTest Debug & Release PASS (kèm toàn bộ test RC2 mới), Benchmark Intel UHD 300-frame, 18 ảnh RC2 + 4 bảng so sánh RC1 vs RC2, đóng gói `dist/RoomCraft3D-v1.0.0-rc2-win64.zip` & SHA-256 | **PASS (chờ Independent QA vòng 2 & User Acceptance)** |
