# Hướng dẫn cài đặt và Build trên Windows x64 — RoomCraft 3D

## 1. Môi trường chính đã kiểm chứng (Milestone M0, M1, M2)

- **Hệ điều hành**: Windows 11 Home Single Language 64-bit (`10.0.26200`)
- **GPU**: `Intel(R) UHD Graphics` (Driver `32.0.101.7088`, hỗ trợ OpenGL 3.3 Core Profile & GLSL 3.30)
- **Compiler chính**: WinLibs MinGW-w64 x64 UCRT (`x86_64-w64-mingw32-g++ 16.2.0`)
- **Build Tools**: CMake `4.4.4`, Ninja `1.13.2`, Git `2.47.1`, Python `3.13.5`
- **Dependency khóa commit (FetchContent)**:
  - `FreeGLUT v3.8.0` (`3db1649ce1f5e42f1338b51e3fa14849be547d5d`)
  - `GLAD v0.1.36` (`1ecd45775d96f35170458e6b148eb0708967e402`)
  - `Dear ImGui v1.93.0-docking` (`b916186e66aa3c02b72c6ca17455516512ed8049`)
  - `GLM 1.0.1` (`0af55ccecd98d4e5a8d1fad7de25ba429d60e863`)

## 2. Build và chạy bằng WinLibs MinGW-w64 x64 + Ninja

```powershell
# 1. Đưa WinLibs mingw64\bin lên đầu PATH của phiên PowerShell hiện tại
$winlibsBin = (Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*" -Directory | Select-Object -First 1).FullName + "\mingw64\bin"
$env:PATH = "$winlibsBin;$env:PATH"

# 2. Configure & Build Debug
cd d:\CG
cmake --preset mingw-x64-debug
cmake --build --preset mingw-x64-debug

# 3. Chạy toàn bộ 5 bộ kiểm thử tự động (M0/M1 Regression + M2 Unit & GPU Integration)
ctest --test-dir build\mingw-x64-debug --output-on-failure -V

# 4. Chạy ứng dụng tương tác M2 Geometry Playground (chạy độc lập, không cần PATH WinLibs)
.\build\mingw-x64-debug\RoomCraft3D.exe
```

Sản phẩm biên dịch tại `build\mingw-x64-debug\`:
- `RoomCraft3D.exe` (Interactive M2 Geometry Playground; hỗ trợ `--smoke-test`, `--m1-smoke-test`, `--m2-smoke-test`)
- `RoomCraft3D_UnitTests.exe` (M1 CPU unit tests: `Transform`, `Camera`, `MeshValidation`)
- `RoomCraft3D_GeometryUnitTests.exe` (M2 CPU unit tests: Primitives, Bézier Curve/Surface, Revolution Surface, BoundingBox, 6 Furniture models)
- `assets\shaders\` (`basic.vert/frag`, `mesh.vert/frag`, `grid.vert/frag`)

## 3. Phương án thay thế: Visual Studio 2022 (MSVC x64) + vcpkg

```powershell
cmake --preset msvc-x64-vcpkg-debug
cmake --build --preset msvc-x64-vcpkg-debug
```
> *Lưu ý*: Preset MSVC + vcpkg chưa được chạy kiểm chứng thực tế trên máy phát triển hiện tại (`NOT RUN`).
