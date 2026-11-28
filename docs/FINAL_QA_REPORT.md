# Báo cáo Kiểm định Chất lượng Tổng thể (Final QA Report — M0–M9 · `v1.0.0-rc1`)

## 1. Phân tách Mức độ Xác minh

- **Automated Verification (CPU + GPU)**: **`19/19` CTest PASS (100%)** trên cả `mingw-x64-debug` (`4.82 s`) và `mingw-x64-release` (`3.56 s`), `OpenGL errors: NONE`.
- **Visual Evidence Capture (GPU FBO thật)**: **`12/12` ảnh PNG `1280×720`** tại `docs/evidence/m9/`, đã kiểm tra trực quan từng ảnh.
- **Isolated Runtime Test**: Đã kiểm chứng bản portable giải nén tại `C:\Users\LENOVO\AppData\Local\Temp\Thử nghiệm RoomCraft 3D (Isolated)\RoomCraft3D-v1.0.0-rc1-win64\` với `PATH=C:\Windows\System32;C:\Windows` và `CWD=%TEMP%` (`--version`, `--m8-smoke-test`, `--m9-smoke-test` đều thoát mã `0`). *(Lưu ý: đây là kiểm thử thư mục cô lập trên máy phát triển, không thay thế kiểm thử trên một máy tính Windows sạch khác.)*
- **Interactive Manual Acceptance (M6–M9)**: **`NOT VERIFIED`** — giữ nguyên trạng thái trung thực cho tới khi người dùng chạy thử tương tác và duyệt.

---

## 2. Kết quả Kiểm toán An toàn Dữ liệu & Độ tin cậy (M9.1)

| Miền kiểm toán | Lỗi / Rủi ro phát hiện trong M9.1 | Biện pháp xử lý | Kiểm thử xác minh |
|---|---|---|---|
| **JSON Parser** (`Json.cpp`) | Thiếu kiểm tra UTF-8 đa byte, không từ chối lone low surrogate `\uDC00`, ghi đè khóa trùng, `std::stod` phụ thuộc locale, thiếu giới hạn số node, thiếu `outError` khi file rỗng | Kiểm tra UTF-8 nghiêm ngặt, từ chối lone surrogate & duplicate key, dùng `std::from_chars(..., double)` + `std::locale::classic()`, giới hạn `250,000` node, sửa `fail(..., outError)` | `M9-JSON-01..08` (kèm 1.500 ca fuzzing) — **PASS** |
| **Project Persistence** (`ProjectManager.cpp`, `ProjectSerializer.cpp`) | Cần bảo đảm tương thích ngược M8 khi thêm shadow fields và kiểm tra khóa tệp đích trên Windows | Giữ các trường shadow tùy chọn với giá trị mặc định an toàn; thêm test tiêm lỗi khóa tệp độc quyền (`CreateFileW(..., 0)`) xác nhận giữ nguyên byte cũ & dọn tệp tạm | `M9-PERS-01..03`, `M8-T10..27` — **PASS** |
| **Dirty-State & Undo/Redo** (`ProjectManager.cpp`, `RoomEditor.cpp`) | `undo()/redo()` trước đây luôn gọi `markDirty()`, không trả về sạch khi undo về bản đã lưu; một số nút toggle chưa sync dirty | Chữ ký trạng thái chuẩn hóa `captureSavedBaseline()` + `syncDirtyWithState()`; sync dirty cho mọi cờ render/editor được lưu | `M9-DIRTY-01..02` — **PASS** |
| **Collision System** (`CollisionSystem.cpp`, `RoomEditor.cpp`) | `Prevent Invalid` có thể khóa kẹt vật đã vi phạm sẵn; `isFloorCovering` quá rộng với vật thu nhỏ dẹt | Thêm `isPreventInvalidTransitionAllowed()` cho phép di chuyển giảm vi phạm; giới hạn `isFloorCovering()` cho asset thảm | `M9-COLL-01..02`, `M8-T02..09` — **PASS** |
| **Unicode & Portable UX** (`Window.cpp`, `Application.cpp`, `EditorTheme.cpp`, `Texture.cpp`, `ModelLoader.cpp`) | Vỡ dấu `—` trên thanh tiêu đề Windows (`â€”`); `argv[0]`, `AddFontFromFileTTF`, `stbi_load`, `Assimp` lỗi khi đặt ở đường dẫn Unicode; rác `imgui.ini` | `SetWindowTextW`, `GetModuleFileNameW`, nạp font/texture qua `std::ifstream(path)` vào bộ nhớ, Assimp dùng `u8string()`, tắt `io.IniFilename` | Isolated Unicode test + `M9-PREF-01` — **PASS** |

---

## 3. Ma trận Truy vết Tổng hợp M0–M9 (`19/19` CTest)

| Milestone | Bộ kiểm thử CTest | Debug | Release | Trạng thái tổng |
|---|---|---:|---:|---|
| **M0** Foundation | `M0_SmokeTest` | PASS (`0.23 s`) | PASS (`0.18 s`) | AUTO PASS |
| **M1** Core Engine | `M1_UnitTests`, `M1_IntegrationSmokeTest` | PASS (`0.02 s`, `0.21 s`) | PASS (`0.02 s`, `0.23 s`) | AUTO PASS |
| **M2** Procedural Geometry | `M2_GeometryUnitTests`, `M2_IntegrationSmokeTest` | PASS (`0.03 s`, `0.25 s`) | PASS (`0.02 s`, `0.22 s`) | AUTO PASS |
| **M3** Blender Asset Pipeline | `M3_AssetUnitTests`, `M3_IntegrationSmokeTest` | PASS (`0.19 s`, `0.67 s`) | PASS (`0.08 s`, `0.34 s`) | AUTO PASS |
| **M4** Scene & Room Editor | `M4_SceneUnitTests`, `M4_IntegrationSmokeTest` | PASS (`0.03 s`, `0.48 s`) | PASS (`0.02 s`, `0.38 s`) | AUTO PASS |
| **M5** Camera, Picking & Interaction | `M5_InteractionUnitTests`, `M5_IntegrationSmokeTest` | PASS (`0.03 s`, `0.37 s`) | PASS (`0.02 s`, `0.32 s`) | AUTO PASS |
| **M6** Professional UI/UX & Gizmo | `M6_EditorUiUnitTests`, `M6_IntegrationSmokeTest` | PASS (`0.03 s`, `0.50 s`) | PASS (`0.02 s`, `0.40 s`) | CONDITIONAL PASS *(chờ manual)* |
| **M7** Lighting, Materials & MSAA | `M7_LightingGpu` | PASS (`0.67 s`) | PASS (`0.49 s`) | IMPLEMENTED / CONDITIONAL PASS |
| **M8** Collision & Persistence | `M8_CollisionUnitTests`, `M8_PersistenceUnitTests`, `M8_IntegrationSmokeTest` | PASS (`0.15 s`, `0.06 s`, `0.40 s`) | PASS (`0.03 s`, `0.03 s`, `0.33 s`) | IMPLEMENTED / CONDITIONAL PASS |
| **M9** Reliability, Shadows, Polish & RC | `M9_ReliabilityUnitTests`, `M9_IntegrationSmokeTest` | PASS (`0.07 s`, `0.40 s`) | PASS (`0.05 s`, `0.35 s`) | **RELEASE CANDIDATE (`v1.0.0-rc1`)** |

---

## 4. Chỉ mục Bằng chứng Hình ảnh GPU Thật (`docs/evidence/m9/`)

Toàn bộ 12 ảnh PNG `1280×720` được chụp trực tiếp bằng `build/mingw-x64-release/RoomCraft3D.exe --m9-capture-evidence docs/evidence/m9` qua off-screen FBO thực tế trên `Intel(R) UHD Graphics` (`OpenGL errors: NONE`):

| Tệp ảnh | Nội dung xác minh trực quan |
|---|---|
| `01_modern_bedroom.png` | Preset **Modern Bedroom** với Directional Shadows 1024 + MSAA 4x + vân ván gỗ + Corner AO |
| `02_gaming_workspace.png` | Preset **Gaming Workspace** (`Warm Interior`, phát hiện trực quan cảnh báo góc chậu cây/đèn bàn sẵn có của preset M4) |
| `03_minimalist_living_room.png` | Preset **Minimalist Living Room** (`6×7×3 m`) với đổ bóng mềm trên sàn gỗ |
| `04_lighting_daylight.png` | Preset ánh sáng **Daylight** + Shadows 1024 |
| `05_lighting_warm_interior.png` | Preset ánh sáng **Warm Interior** (nguồn điểm ấm + chỉ báo dirty `*` ở góc trên-phải) |
| `06_lighting_neutral_studio.png` | Preset ánh sáng **Neutral Studio** |
| `07_lighting_evening.png` | Preset ánh sáng **Evening** (ánh trăng nghiêng tạo bóng dài + đèn nội thất ấm) |
| `08_geometry_lab.png` | **Geometry Lab** — Ghế cong Bicubic Bézier Surface (`508` tris, `0.547 ms`) |
| `09_asset_inspector.png` | **Asset Inspector** — Blender Modern Sofa (`15` submeshes, `836` tris, `6` materials, `2` textures) |
| `10_collision_warning.png` | **Collision Warning**: Sofa chồng Bed → khung hổ phách trên Bed + giữ khung cyan & Gizmo trên Sofa + Status Bar `1 overlap` |
| `11_room_boundary_warning.png` | **Room Boundary Warning**: Sofa vượt tường Đông → cảnh báo biên phòng trong Status Bar (`1 overlap, 1 bounds`) |
| `12_shadows_off_comparison.png` | **Đối chứng Shadows Off** trên Modern Bedroom để so sánh trực tiếp với `01_modern_bedroom.png` |

---

## 5. Vòng đời Tài nguyên GPU (Resource Lifetime Verification)

- Mọi tài nguyên OpenGL (`ShadowMap`, `Framebuffer`, `ThumbnailRenderer`, `SceneRenderer`, `AssetManager`, `TextureManager`, `Shader`, `Mesh`, `VAO/VBO`) đều giải phóng tường minh trong `Application::shutdownGraphics()` **trước khi** OpenGL context bị hủy.
- Kiểm chứng ổn định **240 khung hình liên tiếp** (`--m9-benchmark`) và lặp **5 chu kỳ Save/Load + Asset Reload**: **`OpenGL errors: NONE`**, GPU asset cache giữ ổn định ở `14 assets, 11 textures` (không rò rỉ đối tượng GL).
