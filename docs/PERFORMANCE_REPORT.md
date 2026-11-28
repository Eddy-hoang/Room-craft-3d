# Báo cáo Hiệu năng & Tối ưu hóa (Performance Report — M9.2 · `v1.0.0-rc1`)

## 1. Phương pháp Đo & Điều kiện Thực nghiệm

- **Phần cứng & Hệ điều hành**: Windows 11 x64 (`10.0.26200`), GPU tích hợp **`Intel(R) UHD Graphics`**, Driver `32.0.101.7088`, OpenGL `3.3.0 Core Profile`.
- **Trình biên dịch**: WinLibs MinGW-w64 x64 UCRT `g++ 16.2.0` (`Debug`: `-g`, `Release`: `-O3 -DNDEBUG`).
- **Độ phân giải đo**: **`1280 × 720`** (off-screen FBO).
- **VSync & Cô lập tiến trình**: Chỉ chạy **1 phiên bản `RoomCraft3D.exe` duy nhất** tại một thời điểm; đo trên off-screen FBO không bị chặn bởi `swapBuffers` VSync để tách bạch năng lực xử lý thực của CPU và GPU.
- **Lệnh tái lập**:
  - Debug: `.\build\mingw-x64-debug\RoomCraft3D.exe --m9-benchmark`
  - Release: `.\build\mingw-x64-release\RoomCraft3D.exe --m9-benchmark`
- **Phương pháp lấy mẫu**: `10` khung hình warm-up + `50` khung hình đo (hoặc `40`/`25`/`240` tùy kịch bản), đo tách biệt:
  - **CPU Submit Time**: thời gian gọi lệnh vẽ trên CPU trước `glFinish()`.
  - **GPU Execution Time**: đo trực tiếp trên phần cứng bằng OpenGL 3.3 Core Timer Query (`GL_TIME_ELAPSED`).
  - **Wall-Clock Frame Time**: tổng thời gian hoàn tất khung hình (`avg / median / p95 / p99`).

---

## 2. Ma trận Benchmark Thực tế: Debug vs Release (`1280 × 720`)

| Kịch bản | Draw Calls | Tris | Debug CPU / GPU / Wall avg (p95) | Debug FPS | Release CPU / GPU / Wall avg (p95) | Release FPS | Coll (Rel) |
|---|---:|---:|---|---:|---|---:|---:|
| **30 obj · MSAA Off · Shadows Off** | 333 | 17,960 | `1.33 / 0.76 / 2.66 ms` (`4.62`) | 376.2 | `0.24 / 0.84 / 2.12 ms` (`4.10`) | **471.9** | *(cold load)* |
| **30 obj · MSAA Off · Shadows 1024** | 333 | 17,960 | `2.64 / 1.42 / 4.61 ms` (`6.76`) | 216.9 | `0.26 / 1.45 / 2.73 ms` (`3.56`) | **366.5** | `0.023 ms` |
| **30 obj · MSAA 2× · Shadows 1024** | 333 | 17,960 | `2.09 / 1.79 / 4.44 ms` (`5.94`) | 225.3 | `0.33 / 1.91 / 3.39 ms` (`4.34`) | **294.8** | `0.024 ms` |
| **30 obj · MSAA 4× · Shadows 1024** | 333 | 17,960 | `2.08 / 1.85 / 4.47 ms` (`6.16`) | 223.8 | `0.34 / 2.04 / 3.42 ms` (`4.40`) | **292.2** | `0.023 ms` |
| **100 obj · MSAA Off · Shadows 1024** | 1,102 | 61,540 | `7.48 / 2.46 / 10.51 ms` (`12.95`) | 95.2 | `0.62 / 2.45 / 4.60 ms` (`5.64`) | **217.5** | `0.059 ms` |
| **250 obj · Stress · Shadows 1024** | 2,896 | 156,292 | `22.02 / 4.74 / 27.39 ms` (`30.91`) | 36.5 | `1.58 / 4.61 / 9.04 ms` (`11.08`) | **110.6** | `0.353 ms` |
| **30 obj · Camera Orbit/Zoom** | 333 | 17,960 | `2.48 / 1.48 / 4.44 ms` (`5.23`) | 225.0 | `0.27 / 1.39 / 2.72 ms` (`3.75`) | **367.6** | `0.298 ms` |
| **30 obj · Interactive Gizmo Drag** | 339 | 18,176 | `3.59 / 1.52 / 5.72 ms` (`7.53`) | 174.8 | `0.35 / 1.37 / 2.74 ms` (`3.43`) | **365.5** | `0.023 ms` |
| **30 obj · Stability (240 frames)** | 333 | 17,960 | `3.96 / 1.26 / 5.95 ms` (`7.73`) | 168.1 | `0.29 / 1.20 / 2.52 ms` (`3.31`) | **396.8** | `0.024 ms` |

### Tác vụ I/O & Tái nạp (Debug vs Release)
- **Asset Reload (`blender-modern-sofa`)**: Debug `19.57 ms` → Release **`9.32 ms`** (**PASS**).
- **Save / Load `.roomcraft` (30 đối tượng, trung bình 5 chu kỳ)**:
  - Debug: Save `4.35 ms` | Load `14.41 ms`.
  - Release: Save **`2.72 ms`** | Load **`18.95 ms`** *(gồm kiểm tra toàn vẹn + dựng Scene ứng viên + làm mới va chạm)*.

---

## 3. Phân tích Kết quả & Giải thích Biến thiên M7

1. **Nguyên nhân biến thiên M7 và tác động của bản build Release**:
   - Tại M7, tất cả số đo trước đây đều chạy trên bản **Debug** (không bật tối ưu hóa, GLM không inline, chi phí CPU submit rất cao ở cảnh đông vật thể) và từng có nhiễu đa cửa sổ.
   - Khi chuyển sang **Release (`-O3`)** với một tiến trình cô lập duy nhất, **CPU submit time giảm 5× đến 13.9×** (`22.02 ms → 1.58 ms` ở cảnh 250 vật thể), đưa `p95` ở mọi kịch bản (kể cả stress **250 vật thể + Shadow Map 1024**) xuống **`11.08 ms` (< 16.67 ms của 60 Hz)**.
2. **Chi phí thực tế của Directional Shadow Mapping (1024×1024 PCF 3×3)**:
   - Trên cảnh tiêu chuẩn 30 vật thể (Release): GPU tăng từ `0.84 ms` → `1.45 ms` (`+0.61 ms` tổng wall-clock, vẫn đạt **`366.5 FPS`**).
   - Kết hợp **MSAA 4× + Shadows 1024**: `3.42 ms` avg / `4.40 ms` p95 (**`292.2 FPS`**) — dư dả hơn **4.8×** ngân sách 60 Hz trên GPU tích hợp Intel UHD. Do đó, giữ bật **Directional Shadow Mapping (`1024`)** làm tính năng chính thức trong `v1.0.0-rc1` (có công tắc bật/tắt & chỉnh `512/1024/2048` trong `Room Settings`).
3. **Giới hạn đã ghi nhận trung thực**:
   - Benchmark đo trên off-screen FBO không bao gồm thời gian vẽ UI ImGui & đồng bộ DWM VSync thực tế (khi chạy cửa sổ tương tác có VSync 60 Hz, FPS sẽ khóa ở 60 FPS theo màn hình).
   - Ở cảnh 250 vật thể, số draw call đạt `2,896` (do vẽ từng submesh chưa instancing); nếu tương lai vượt `500+` vật thể mới cần cân nhắc instanced rendering.
