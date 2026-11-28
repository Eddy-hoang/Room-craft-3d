# Báo cáo Hiệu năng & Tối ưu hóa (Performance Report — M9 · `v1.0.0-rc2`)

## 1. Phương pháp Đo & Điều kiện Thực nghiệm

- **Phần cứng & Hệ điều hành**: Windows 11 x64 (`10.0.26200`), GPU tích hợp **`Intel(R) UHD Graphics`**, Driver `32.0.101.7088`, OpenGL `3.3.0 Core Profile`.
- **Trình biên dịch**: WinLibs MinGW-w64 x64 UCRT `g++ 16.2.0` (`Debug`: `-g`, `Release`: `-O3 -DNDEBUG`).
- **Độ phân giải đo**: **`1280 × 720`** (off-screen FBO & Complete Editor Frame gồm Dear ImGui).
- **Cô lập tiến trình**: Chỉ chạy **1 phiên bản `RoomCraft3D.exe` duy nhất** tại một thời điểm; đo trên off-screen FBO không bị chặn bởi `swapBuffers` VSync để tách bạch năng lực xử lý thực của CPU và GPU.
- **Lệnh tái lập**:
  - Release: `.\build\mingw-x64-release\RoomCraft3D.exe --m9-benchmark`
  - Debug: `.\build\mingw-x64-debug\RoomCraft3D.exe --m9-benchmark`
- **Phương pháp lấy mẫu**: `8–15` khung hình warm-up + `60–300` khung hình đo (**`300` khung hình** cho các cấu hình mặc định tiêu chuẩn 30 vật thể; `100` khung hình cho Complete Editor Frame gồm Dear ImGui), đo tách biệt CPU submit, `GL_TIME_ELAPSED` GPU, và Wall-Clock (`avg / med / p95 / p99`).

---

## 2. Ma trận Benchmark Thực tế RC2 (`v1.0.0-rc2`): Release vs Debug (`1280 × 720`)

| Kịch bản | Draws | Tris | Release CPU / GPU / Wall avg (`p95`) | Release FPS | Debug CPU / GPU / Wall avg (`p95`) | Debug FPS |
|---|---:|---:|---|---:|---|---:|
| **30 obj · MSAA Off · Shadows Off** | 387 | 18,692 | `0.56 / 4.40 / 6.37 ms` (`8.52`) | **157.0** | `4.93 / 4.17 / 10.01 ms` (`13.79`) | 99.9 |
| **30 obj · MSAA Off · Shadows 512** | 387 | 18,692 | `0.97 / 4.96 / 7.11 ms` (`8.85`) | **140.7** | `8.08 / 5.39 / 15.00 ms` (`26.52`) | 66.7 |
| **30 obj · MSAA Off · Shadows 1024 (300f)** | 387 | 18,692 | `1.15 / 5.34 / 7.90 ms` (`10.36`) | **126.6** | `6.01 / 5.96 / 13.01 ms` (`16.54`) | 76.9 |
| **30 obj · MSAA Off · Shadows 2048** | 387 | 18,692 | `1.23 / 6.32 / 8.64 ms` (`11.34`) | **115.7** | `7.62 / 4.88 / 13.71 ms` (`23.39`) | 72.9 |
| **30 obj · MSAA 2× · Shadows 1024** | 387 | 18,692 | `1.16 / 7.18 / 9.54 ms` (`12.50`) | **104.9** | `4.71 / 4.01 / 9.50 ms` (`12.31`) | 105.3 |
| **30 obj · MSAA 4× · Shadows 1024 (300f)** | 387 | 18,692 | `1.41 / 7.33 / 10.05 ms` (`13.11`) | **99.5** | `6.34 / 4.32 / 11.61 ms` (`15.86`) | 86.1 |
| **100 obj · MSAA Off · Shadows 1024** | 1,276 | 63,712 | `2.79 / 8.85 / 14.39 ms` (`16.90`) | **69.5** | `21.07 / 6.43 / 28.63 ms` (`34.94`) | 34.9 |
| **250 obj · Stress · Shadows 1024** | 3,329 | 161,572 | `6.46 / 14.03 / 25.95 ms` (`32.99`) | **38.5** | `34.68 / 8.14 / 43.65 ms` (`50.41`) | 22.9 |
| **30 obj · Camera Orbit/Zoom** | 387 | 18,692 | `1.40 / 6.31 / 9.11 ms` (`12.75`) | **109.7** | `4.56 / 3.37 / 8.72 ms` (`11.05`) | 114.7 |
| **30 obj · Interactive Gizmo Drag** | 393 | 18,908 | `1.35 / 6.12 / 8.82 ms` (`12.40`) | **113.4** | `4.76 / 3.60 / 9.07 ms` (`10.80`) | 110.2 |
| **Complete Editor Frame (ImGui + 30 obj + 4× + 1024)** | — | — | Wall avg **`8.93 ms`** / p95 **`11.78 ms`** | **112.0** | Wall avg `12.60 ms` / p95 `15.99 ms` | 79.3 |

### Tác vụ I/O & Tái nạp (RC2 Release vs Debug)
- **Asset Reload (`blender-modern-sofa`)**: Release **`12.42 ms`** | Debug `18.90 ms` (**PASS**).
- **Save / Load `.roomcraft` (30 vật thể, trung bình 5 chu kỳ)**: Release Save **`4.71 ms`** / Load **`15.55 ms`** | Debug Save `5.67 ms` / Load `17.13 ms` (**PASS**).

---

## 3. So sánh Hiệu năng RC1 vs RC2 & Đánh đổi Chất lượng

1. **Mục tiêu 60 FPS ở cấu hình mặc định (30 vật thể) ĐẠT DƯ DẢ trên Intel UHD**:
   - **30 obj + Shadows 1024 + MSAA 4×**: `10.05 ms` avg / `13.11 ms` p95 (**`99.5 FPS`**).
   - **Complete Editor Frame (toàn bộ Dear ImGui + 3D Viewport MSAA 4× + Shadows 1024)**: **`8.93 ms` avg / `11.78 ms` p95 (`112.0 FPS`)** — nằm gọn trong ngân sách `16.67 ms` (60 Hz) ngay cả ở phân vị `p95`.
2. **Giải thích trung thực chi phí tăng thêm từ RC1 → RC2**:
   - Do **Camera Framing RC2** tăng diện tích pixel màn hình thực tế của căn phòng lên **>2.1×** (`24% → 55%` viewport), cộng thêm **5-tap hardware bilinear PCF (`sampler2DShadow`)**, **ACES Filmic Tone Mapping + Energy-Aware Fresnel**, **Soft Floor Contact Grounding (16 OBB)**, **Anisotropic Filtering** và các chi tiết **Baseboards + Window + Bookshelf books** (`333 → 387` draw calls ở 30 obj; `2,896 → 3,329` ở 250 obj):
     - Cảnh chuẩn 30 obj + Shadows 1024 + MSAA 4× tăng từ `3.42 ms` (RC1) lên `10.05 ms` (RC2) — vẫn đạt **`99.5 FPS`** (và **`112.0 FPS`** Full Editor Frame).
     - Cảnh stress `250` vật thể giảm từ `110.6 FPS` (RC1) xuống **`38.5 FPS`** (RC2); khuyến nghị chuyển `Shadow Quality` sang `512 (Fast)` hoặc tắt bóng nếu bố trí cảnh stress trên `150+` vật thể.
   - Chi phí giữa `Shadow 512 / 1024 / 2048` rất hợp lý: `7.11 ms (140.7 FPS)` → `7.90 ms (126.6 FPS)` → `8.64 ms (115.7 FPS)`, xác nhận **`1024 (Balanced)`** là cấu hình mặc định tối ưu.
