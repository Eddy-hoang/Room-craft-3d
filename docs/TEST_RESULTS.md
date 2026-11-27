# Kết quả kiểm thử thực tế — RoomCraft 3D (Test Results)

Cập nhật lần cuối: **2026-11-27**

## 1. Cấu hình máy kiểm thử

- **OS**: Microsoft Windows 11 Home Single Language 64-bit (`10.0.26200`)
- **GPU**: `Intel(R) UHD Graphics` (Vendor: `Intel`, Driver: `32.0.101.7088`)
- **OpenGL Runtime**: `3.3.0 - Build 32.0.101.7088` (`Core Profile`), GLSL `3.30 - Build 32.0.101.7088`
- **Toolchain**: `x86_64-w64-mingw32-g++ 16.2.0` (WinLibs UCRT), `CMake 4.4.4`, `Ninja 1.13.2`, `GLM 1.0.1`
- **CTest tổng hợp**: `ctest --test-dir build/mingw-x64-debug --output-on-failure -V` → **`3/3 Passed (1.37 sec)`**

---

## 2. Kết quả kiểm thử Milestone M0 (Project Foundation)

| Test ID | Module | Kết quả mong đợi | Kết quả thực tế | Trạng thái |
|---|---|---|---|---|
| **TC-M0-01** | Foundation | CMake Configure không lỗi | `-- Configuring done`, `-- Generating done`, exit code `0` | **PASS** |
| **TC-M0-02** | Foundation | Build Debug tạo `RoomCraft3D.exe` | Build 0 lỗi, 0 cảnh báo trên code project; liên kết tĩnh runtime | **PASS** |
| **TC-M0-03** | Foundation | Khởi tạo OpenGL 3.3 Core + thông tin GPU | `Intel(R) UHD Graphics`, `OpenGL 3.3.0 (Core Profile)`, `GLSL 3.30` | **PASS** |
| **TC-M0-04** | Renderer (M0) | Render tam giác bằng VAO/VBO/GLSL 330 | `Center RGB=(140,147,160)`, `OpenGL errors: NONE` | **PASS** |
| **TC-M0-05** | Window | Resize cửa sổ `960x540` | `[SMOKE] Resize verified: YES (960x540)` | **PASS** |
| **TC-M0-06** | Dear ImGui | Tích hợp `imgui_impl_glut` + `imgui_impl_opengl3` | Khởi tạo và render không lỗi GL | **PASS** |
| **TC-M0-07** | Lifecycle | Giải phóng GPU/ImGui trước khi hủy context | `GPU & Dear ImGui resources released cleanly`, exit code `0` | **PASS** |
| **TC-M0-MSVC** | Build MSVC | `cmake --preset msvc-x64-vcpkg-debug` | Máy chưa cài MSVC Build Tools và `vcpkg` | **NOT RUN** |
| **TC-M0-MANUAL** | Manual GUI | Người dùng kiểm thử thủ công 8 mục M0 | Chưa nhận bằng chứng xác nhận thủ công từ người dùng | **NOT VERIFIED** |

---

## 3. Kết quả kiểm thử Milestone M1 (Core Graphics Engine)

| Test ID | Module | Kết quả mong đợi | Kết quả thực tế & Bằng chứng | Trạng thái |
|---|---|---|---|---|
| **TC-M1-01** | Regression M0 | `M0_SmokeTest` vẫn PASS sau refactor | `Test #1: M0_SmokeTest Passed (0.82 sec)` — `Center RGB=140,147,160`, `Resize 960x540`, `OpenGL errors: NONE` | **PASS** |
| **TC-M1-02** | Shader | Compile & link `mesh.*` và `grid.*` thành công | `[TC-M1-02] Valid 3D & Grid shaders compiled and linked: PASS` | **PASS** |
| **TC-M1-03** | Shader | Shader sai cú pháp báo lỗi rõ ràng | Driver Intel trả về `ERROR: 0:2: 'INVALID_SYNTAX_TOKEN' : undeclared identifier`; `loadFromSource` trả về `false`, handle `0` | **PASS** |
| **TC-M1-04** | Mesh | Upload và vẽ indexed cube (24 verts, 36 indices) | `[TC-M1-04] Indexed cube mesh uploaded (24 vertices, 36 indices): PASS` | **PASS** |
| **TC-M1-05** | Mesh Validation | Từ chối chỉ mục vượt biên, buffer rỗng, sai topology, NaN | Unit test (`MeshValidationTests`, 6/6 PASS) + Integration (`index=99, vertexCount=3` bị từ chối sạch) | **PASS** |
| **TC-M1-06** | Transform | Translation chính xác | `M1_UnitTests`: `[Transform] TC-M1-06 Translation: PASS` | **PASS** |
| **TC-M1-07** | Transform | Rotation +90° quanh X, Y, Z chính xác | `M1_UnitTests`: cả 3 trục X, Y, Z đều `PASS` (`epsilon = 1e-4`) | **PASS** |
| **TC-M1-08** | Transform | Uniform/Non-uniform Scaling, `M=T*R*S`, từ chối scale lỗi | `M1_UnitTests`: Uniform 2x, Non-uniform `(2,0.5,3)`, Combined `T*R*S`, Reset, Invalid scale (0/âm/NaN) đều `PASS` | **PASS** |
| **TC-M1-09** | Camera | Perspective projection đúng + từ chối input lỗi | `M1_UnitTests`: foreshortening `PASS`; từ chối aspect ≤ 0/NaN, clip planes lỗi, `pos==target`, collinear/zero up, FOV ngoài biên `PASS` | **PASS** |
| **TC-M1-10** | Camera | Orthographic projection đúng + presets hợp lệ | `M1_UnitTests`: `w=1`, tọa độ gần/xa trùng khớp `0.5`, Top/Front/Reset presets `PASS` | **PASS** |
| **TC-M1-11** | Depth Testing | Che khuất đúng độc lập thứ tự draw + đối chứng Depth OFF | `Far->Near (Depth ON) RGB=255,0,0`; `Near->Far (Depth ON) RGB=255,0,0`; `Near->Far (Depth OFF) RGB=0,0,255` | **PASS** |
| **TC-M1-12** | Renderer / Scene | Hiển thị nhiều cube 3D + Grid/Axes trong 10 frames | `Frames rendered: 10`, `drawCalls=7 (vertices=130, indices=166)`; `CubeA=(213,82,82)`, `CubeB=(88,141,221)`, `CubeC=(75,196,131)` | **PASS** |
| **TC-M1-13** | Framebuffer | FBO hoàn thiện (`GL_FRAMEBUFFER_COMPLETE`) | `[TC-M1-13] Off-screen Framebuffer complete with valid color texture: PASS` | **PASS** |
| **TC-M1-14** | Framebuffer | Resize đúng, no-op caching, xử lý an toàn `0x0` | `[TC-M1-14] Framebuffer resize, no-op caching, and 0x0 safety: PASS` | **PASS** |
| **TC-M1-15** | Lifecycle (RAII) | Move ownership sạch, chống double-delete, giải phóng trước khi hủy context | `[TC-M1-15] RAII move ownership & safe resource destruction: PASS`; `GPU & Dear ImGui resources released cleanly` | **PASS** |
| **TC-M1-16** | OpenGL Errors | Không phát sinh lỗi OpenGL ngoài dự kiến | `[M1-SMOKE] OpenGL errors: NONE`, `M1_EXIT=0`, `UNIT_EXIT=0` (chạy cả trong CTest và độc lập ngoài PATH WinLibs) | **PASS** |
| **TC-M1-MANUAL** | Manual GUI | Người dùng chạy `RoomCraft3D.exe`, kiểm tra trực quan 3 cube 3D trong ImGui Viewport, tắt/bật Depth Test, đổi Perspective/Orthographic, kéo Transform | Chưa được người dùng chạy kiểm chứng trực quan | **NOT VERIFIED** |
