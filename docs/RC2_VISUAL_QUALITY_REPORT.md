# Báo cáo Chất lượng Thị giác RC1 vs RC2 (`v1.0.0-rc2`)

Tài liệu này tổng hợp bằng chứng trực quan thực tế (kết xuất 100% từ GPU `Intel(R) UHD Graphics`, OpenGL 3.3 Core Profile, `1280 × 720`, MSAA 4×) đối chiếu giữa **RC1 (`v1.0.0-rc1`)** và **RC2 (`v1.0.0-rc2`)**.

> **Lưu ý khách quan**: Các chỉ số định lượng dưới đây là số đo thực tế từ pixel ảnh chụp GPU. Điểm thẩm mỹ độc lập (mục tiêu `≥ 8/10`) sẽ do **Independent QA vòng 2** và **Product Owner** trực tiếp quyết định.

---

## 1. Đối chiếu Định lượng Pixel: RC1 vs RC2

Định nghĩa phép đo (thống nhất trên vùng 3D Viewport `VBOX = (280, 105, 956, 655)`):
- **Silhouette Coverage (%)**: tỷ lệ pixel thuộc phòng/nội thất so với tổng diện tích vùng 3D Viewport.
- **Projected BBox Coverage (%)**: tỷ lệ diện tích hình chữ nhật bao khít hình chiếu căn phòng so với vùng 3D Viewport.
- **Clipped Pixels (`max(R,G,B) ≥ 248`)**: số lượng và tỷ lệ pixel gần/đạt ngưỡng bão hòa trắng trong vùng phòng.

| Kịch bản | RC1 Silhouette | RC2 Silhouette | RC2 Projected BBox | RC1 Clipped (`≥248`) | RC2 Clipped (`≥248`) | Kết quả |
|---|---:|---:|---:|---:|---:|---|
| **Modern Bedroom (Daylight)** | `26.5%` | **`55.4%`** | `89.2%` | `14` (`0.01%`) | **`0` (`0.00%`)** | Tăng gấp 2.1× độ phủ, 0 cắt phòng |
| **Gaming Workspace (Warm)** | `26.5%` | **`55.0%`** | `87.5%` | **`5,905` (`5.98%`)** | **`0` (`0.00%`)** | Hết cháy sáng, 0 lỗi va chạm/biên |
| **Minimalist Living Room** | `22.5%` | **`45.8%`** | `81.7%` | `8` (`0.01%`) | **`0` (`0.00%`)** | Bố cục đầy đặn, hài hòa hơn |
| **Daylight (Modern Bedroom)** | `24.2%` | **`49.3%`** | `84.3%` | `12` (`0.01%`) | **`0` (`0.00%`)** | Sạch shadow acne, tương phản khối rõ |
| **Warm Interior** | `24.2%` | **`49.1%`** | `84.3%` | **`7,620` (`8.47%`)** | **`0` (`0.00%`)** | Giảm 100% cháy sáng tường |
| **Neutral Studio** | `24.2%` | **`49.8%`** | `84.3%` | `12` (`0.01%`) | **`0` (`0.00%`)** | Trung tính sạch, không phẳng lì |
| **Evening** | `24.2%` | **`46.6%`** | `84.1%` | `144` (`0.16%`) | **`0` (`0.00%`)** | Bóng mềm, không răng cưa & peter-panning |

---

## 2. Xác minh Khắc phục từng Nhóm Kỹ thuật Đồ họa

### 2.1. Hệ thống Đổ bóng (`RC2.1` — `comparison_03_shadow_acne_rc1_vs_rc2.png`, `comparison_04_shadow_512_1024_2048.png`)
- **Wall Shadow Acne**: Ảnh khuếch đại hiệu 4× giữa `Shadows Off` và `Shadows 1024` cho thấy toàn bộ vân Moiré đồng tâm trên 2 tường ở RC1 đã **biến mất hoàn toàn** trên RC2 (`UpperWestWall ShadowAcneMeanDiff = 0.938 < 1.5` theo test `RC2-GPU-03`).
- **Cạnh bóng & Peter-Panning**: Hardware `sampler2DShadow` (`GL_COMPARE_REF_TO_TEXTURE` + `GL_LINEAR`) kết hợp 5-tap bilinear PCF và normal-offset bias theo kích thước texel thế giới cho cạnh bóng tủ quần áo và lá cây mượt mịn trên cả 3 mức `512 / 1024 / 2048`, bám khít chân đồ nội thất.

### 2.2. Ánh sáng & ACES Tone Mapping (`RC2.2` — `comparison_02_lighting_shadows_rc1_vs_rc2.png`)
- Đường ống màu tuyến tính được kiểm soát chặt: sRGB texture (`GL_SRGB8/SRGB8_ALPHA8`) + `linearColor(uColor)` → chiếu sáng tuyến tính HDR trong shader → **ACES Filmic Tone Mapping** (`exposure = 0.88`, chỉ bật cho cảnh 3D có chiếu sáng, không áp lên ImGui/gizmo/wireframe) → **1 lần chuyển `displayColor()` sang sRGB duy nhất**.
- `computeRoomCornerAO` nay chỉ chạy trên bề mặt phòng (`uIsRoomSurface == 1`) và tôn trọng mặt nạ tường hiển thị (`uVisibleWalls`), loại bỏ hoàn toàn tối sai ở đồ nội thất và mép sàn Cutaway trống.

### 2.3. Vật liệu & Bóng tiếp xúc (`RC2.4`)
- 8 file `.mtl` Blender được hiệu chỉnh lại từng chất liệu thay vì `Ks = 0.50` đồng loạt (thảm `Ks=0.02, Ns=8`, đất `Ks=0.015, Ns=6`, vải/linen `Ks=0.03–0.04, Ns=12–18`, gỗ sồi `Ks=0.11, Ns=42`, gốm `Ks=0.24, Ns=96`, đồng thau `Ks=0.62, Ns=220`, giữ nguyên `ComputerAluminum Ks=0.50, Ns=490`).
- Energy-aware Blinn-Phong + Schlick Fresnel nhẹ, Anisotropic Filtering tối đa `8×` theo truy vấn extension Core Profile, và **Soft Contact Grounding Shadows** (tối đa 16 OBB đứng sàn, chỉ tác động lên ambient và tự loại trừ thảm/vật đặt trên bàn) giúp toàn bộ nội thất bám chặt xuống mặt sàn.
- *Giới hạn tài liệu hóa*: Contact Grounding là xấp xỉ hình hộp OBB trên mặt sàn `Y = 0` (không thay thế ray-traced AO cho đồ vật đặt trên mặt bàn), và `computeRoomCornerAO` là xấp xỉ phân tích góc phòng (không phải Global Illumination vật lý).

### 2.4. Kiến trúc phòng, Mô hình Procedural & Trình bày Editor (`RC2.5` – `RC2.6`)
- **Kiến trúc**: Baseboards chạy khít chân 4 tường (tôn trọng Cutaway/Manual visibility), cửa sổ kiến trúc trang trí tường Bắc thích nghi màu trời theo 4 lighting presets (`Show Decorative Window`), sàn gỗ chống moiré bằng `fwidth()`.
- **Procedural Modeling**: `Desk` có mặt bàn vát cạnh 2 lớp, yếm bàn, ngăn kéo gỗ sồi kèm tay nắm đồng thau; `Bookshelf` có chân đế âm plinth, 15+ quyển sách đa màu Scandinavian và hộp vải linen sinh hoàn toàn bằng thuật toán C++ (giữ nguyên `assetId` và bounding box chuẩn).
- **Editor UI**: Bỏ tiền tố `"Procedural "` cắt chữ trên 6 thẻ C++ (`Desk`, `Dining Chair`, `Bookshelf`, `Desk Lamp`, `Ceramic Vase`, `Curved Chair` — vẫn giữ badge `[C++]` và đầy đủ metadata trong tooltip), mở rộng `Scene Hierarchy` (`0.43`), sửa nhãn `Validation Mode`, nâng cấp framing thumbnail (`1.16f`), thêm Studio Gradient Background và **Passive 3D Orientation Indicator** góc trên-phải.

---

## 3. Danh mục Bằng chứng Hình ảnh (`docs/evidence/rc2/`)

- **Ảnh đối chiếu Baseline RC1**: `docs/evidence/rc2/rc1_baseline/` (14 ảnh nguyên bản từ RC1).
- **18 ảnh chụp GPU thực tế RC2 (`1280 × 720`, `OpenGL errors: NONE`)**:
  - `01_modern_bedroom.png`, `02_gaming_workspace.png`, `03_minimalist_living_room.png`
  - `04_lighting_daylight.png`, `05_lighting_warm_interior.png`, `06_lighting_neutral_studio.png`, `07_lighting_evening.png`
  - `08_geometry_lab.png`, `09_asset_inspector.png`, `10_collision_warning.png`, `11_room_boundary_warning.png`
  - `12_shadows_off_comparison.png`, `13_shadow_512.png`, `14_shadow_1024.png`, `15_shadow_2048.png`
  - `16_isometric_view.png`, `17_room_settings_grid_on.png`, `18_desk_bookshelf_closeup.png`
- **4 bảng so sánh & phóng đại chẩn đoán**:
  - `comparison_01_rooms_rc1_vs_rc2.png`
  - `comparison_02_lighting_shadows_rc1_vs_rc2.png`
  - `comparison_03_shadow_acne_rc1_vs_rc2.png`
  - `comparison_04_shadow_512_1024_2048.png`
- **Hạng mục chưa chụp tương tác desktop OS**: Ảnh chụp cửa sổ tương tác trực tiếp bằng chuột/hộp thoại Win32 vẫn giữ trạng thái **`NOT VERIFIED`** do phiên dòng lệnh không có desktop compositor tương tác (cần người dùng tự kiểm 5 phút trên gói portable).
