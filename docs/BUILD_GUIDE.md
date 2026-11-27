# Hướng dẫn cài đặt và Build trên Windows x64 — RoomCraft 3D

## 1. Môi trường chính đã kiểm chứng (Milestone M0–M3)

- **Hệ điều hành**: Windows 11 Home Single Language 64-bit (`10.0.26200`)
- **GPU**: `Intel(R) UHD Graphics` (Driver `32.0.101.7088`, hỗ trợ OpenGL 3.3 Core Profile & GLSL 3.30)
- **Compiler chính**: WinLibs MinGW-w64 x64 UCRT (`x86_64-w64-mingw32-g++ 16.2.0`)
- **Build Tools**: CMake `4.4.4`, Ninja `1.13.2`, Git `2.47.1`, Python `3.13.5`
- **Công cụ thiết kế 3D (sinh asset M3)**: `Blender 5.2.3 LTS` (`e13b728c6880`, bản giải nén portable tại `D:\blender\Blender-5.2.3\Blender Foundation\Blender 5.2\blender.exe`)
- **Dependency khóa phiên bản (FetchContent)**:
  - `FreeGLUT v3.8.0` (`3db1649ce1f5e42f1338b51e3fa14849be547d5d`)
  - `GLAD v0.1.36` (`1ecd45775d96f35170458e6b148eb0708967e402`)
  - `Dear ImGui v1.93.0-docking` (`b916186e66aa3c02b72c6ca17455516512ed8049`)
  - `GLM 1.0.1` (`0af55ccecd98d4e5a8d1fad7de25ba429d60e863`)
  - `Assimp v6.0.6` (`a79ff6bbce54d3e792691ef4b72ae26434c3e799`, release archive `.tar.gz`, static, chỉ bật OBJ importer, tắt exporters/tests/tools)
  - `stb` (`stb_image.h`, commit `6b5799bae8a1fe3792691ef4b72ae26434c3e799`, release archive `.tar.gz`)

> **Lưu ý TLS trên WinLibs**: `CMakeLists.txt` tự động phát hiện và thiết lập `CMAKE_TLS_CAINFO` tới `C:/Program Files/Git/mingw64/etc/ssl/certs/ca-bundle.crt` khi có Git for Windows, giúp `FetchContent` tải archive HTTPS của Assimp và `stb` với xác thực chứng chỉ đầy đủ.

## 2. Build và chạy bằng WinLibs MinGW-w64 x64 + Ninja

```powershell
# 1. Đưa WinLibs mingw64\bin lên đầu PATH của phiên PowerShell hiện tại
$winlibsBin = (Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*" -Directory | Select-Object -First 1).FullName + "\mingw64\bin"
$env:PATH = "$winlibsBin;$env:PATH"

# 2. Configure & Build Debug
cd d:\CG
cmake --preset mingw-x64-debug
cmake --build --preset mingw-x64-debug

# 3. Chạy toàn bộ 7 bộ kiểm thử tự động (M0/M1/M2 Regression + M3 Unit & GPU Integration)
ctest --test-dir build\mingw-x64-debug --output-on-failure -V

# 4. Chạy ứng dụng tương tác M3 Furniture Asset Browser (chạy độc lập, không cần PATH WinLibs)
.\build\mingw-x64-debug\RoomCraft3D.exe
```

Sản phẩm biên dịch tại `build\mingw-x64-debug\`:
- `RoomCraft3D.exe`: Ứng dụng chính (mặc định mở **M3 Furniture Asset Browser**, hỗ trợ chuyển qua M2/M1 và các cờ kiểm thử `--smoke-test`, `--m1-smoke-test`, `--m2-smoke-test`, `--m3-smoke-test`).
- `RoomCraft3D_UnitTests.exe`: M1 CPU unit tests (`Transform`, `Camera` + `fitBoundingBox`, `MeshValidation`).
- `RoomCraft3D_GeometryUnitTests.exe`: M2 CPU unit tests (Primitives, Bézier Curve/Surface, Revolution Surface, BoundingBox, 6 Procedural Furniture models).
- `RoomCraft3D_AssetUnitTests.exe`: M3 CPU unit tests (`ModelLoader`, `AssetNormalizer`, `FurnitureCatalog`, 8 Blender assets + test fixtures).
- `assets\shaders\` (`basic.vert/frag`, `mesh.vert/frag`, `grid.vert/frag`) và `assets\models\` (8 thư mục mô hình Blender `.obj`/`.mtl`/`.png`), tự động copy bởi `POST_BUILD`.

## 3. Tái sinh lại toàn bộ 8 mô hình Blender (tùy chọn)

Các file `.blend`, `.obj`, `.mtl` và `.png` đã có sẵn trong repository. Nếu muốn tái sinh từ script Python `bpy`:

```powershell
& "D:\blender\Blender-5.2.3\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup --python d:\CG\blender\scripts\generate_all.py
```

Xem thêm hướng dẫn chi tiết tại [`docs/BLENDER_GUIDE.md`](BLENDER_GUIDE.md) và [`blender/README.md`](../blender/README.md).

## 4. Phương án thay thế: Visual Studio 2022 (MSVC x64) + vcpkg

```powershell
cmake --preset msvc-x64-vcpkg-debug
cmake --build --preset msvc-x64-vcpkg-debug
```
> *Lưu ý*: `vcpkg.json` đã khai báo đủ `freeglut`, `glad`, `glm`, `imgui`, `assimp`, `stb`, nhưng preset MSVC + vcpkg chưa được chạy kiểm chứng thực tế trên máy phát triển hiện tại (`NOT RUN`).
