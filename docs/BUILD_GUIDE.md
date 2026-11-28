# Hướng dẫn cài đặt và Build trên Windows x64 — RoomCraft 3D

## 1. Môi trường chính đã kiểm chứng (Milestone M0–M6)

- **Hệ điều hành**: Windows 11 Home Single Language 64-bit (`10.0.26200`)
- **GPU**: `Intel(R) UHD Graphics` (Driver `32.0.101.7088`, hỗ trợ OpenGL 3.3 Core Profile & GLSL 3.30)
- **Compiler chính**: WinLibs MinGW-w64 x64 UCRT (`x86_64-w64-mingw32-g++ 16.2.0`)
- **Build Tools**: CMake `4.4.4`, Ninja `1.13.2`, Git `2.47.1`, Python `3.13.5`
- **Dependency khóa phiên bản & hash (FetchContent)**:
  - `FreeGLUT v3.8.0` (`3db1649ce1f5e42f1338b51e3fa14849be547d5d`)
  - `GLAD v0.1.36` (`1ecd45775d96f35170458e6b148eb0708967e402`)
  - `Dear ImGui v1.93.0-docking` (`b916186e66aa3c02b72c6ca17455516512ed8049`)
  - `GLM 1.0.1` (`0af55ccecd98d4e5a8d1fad7de25ba429d60e863`)
  - `Assimp v6.0.6` (`a79ff6bbce54d3e01186a918dc6374ed55317426`, `URL_HASH SHA256=5f7ed412b76575621df0525a762b31da5874afff352b3a890c6308db4ac7204c`, static OBJ-only)
  - `stb` (`stb_image.h v2.34`, `6b5799bae8a1fe3792691ef4b72ae26434c3e799`, `URL_HASH SHA256=87e9ecd2d2e0cb8fa35bf5a93ea6ca3999595ea4f0a4738ed1aac0a34cbd4da9`)
- **UI Font bundled**: `assets/fonts/Roboto-Medium.ttf` (Apache License 2.0, tự động đồng bộ cạnh file thực thi qua CMake `POST_BUILD`).

## 2. Build và chạy bằng WinLibs MinGW-w64 x64 + Ninja

```powershell
# 1. Đưa WinLibs mingw64\bin lên đầu PATH của phiên PowerShell hiện tại
$winlibsBin = (Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*" -Directory | Select-Object -First 1).FullName + "\mingw64\bin"
$env:PATH = "$winlibsBin;$env:PATH"

# 2. Configure & Build Debug
cd d:\CG
cmake --preset mingw-x64-debug
cmake --build --preset mingw-x64-debug

# 3. Chạy toàn bộ 14 bộ kiểm thử tự động (M0–M7)
ctest --test-dir build\mingw-x64-debug --output-on-failure -V

# 4. Chạy ứng dụng tương tác RoomCraft 3D (chạy độc lập, không cần PATH WinLibs)
.\build\mingw-x64-debug\RoomCraft3D.exe

# 5. Chạy riêng smoke test M6 hoặc benchmark M4
.\build\mingw-x64-debug\RoomCraft3D.exe --m6-smoke-test
.\build\mingw-x64-debug\RoomCraft3D.exe --m4-benchmark
```

Sản phẩm biên dịch tại `build\mingw-x64-debug\`:
- `RoomCraft3D.exe`: Ứng dụng chính (mặc định mở **RoomCraft 3D — Room Editor** với Product Menu Bar; hỗ trợ `--smoke-test`, `--m1-smoke-test`, `--m2-smoke-test`, `--m3-smoke-test`, `--m4-smoke-test`, `--m4-benchmark`, `--m5-smoke-test`, `--m6-smoke-test`). Có thể chạy từ bất kỳ working directory nào nhờ tìm tài nguyên tương đối theo thư mục file thực thi.
- `RoomCraft3D_UnitTests.exe` (M1), `RoomCraft3D_GeometryUnitTests.exe` (M2), `RoomCraft3D_AssetUnitTests.exe` (M3), `RoomCraft3D_SceneUnitTests.exe` (M4), `RoomCraft3D_InteractionUnitTests.exe` (M5), `RoomCraft3D_EditorUiUnitTests.exe` (M6).
- `assets\shaders\`, `assets\models\`, `assets\fonts\`: Tự động đồng bộ bởi `POST_BUILD`.

## 3. Phương án thay thế: Visual Studio 2022 (MSVC x64) + vcpkg

```powershell
cmake --preset msvc-x64-vcpkg-debug
cmake --build --preset msvc-x64-vcpkg-debug
```
> *Lưu ý*: Preset MSVC + vcpkg chưa được chạy kiểm chứng thực tế trên máy phát triển hiện tại (`NOT RUN`).

Lượt Final QA đã gỡ override CMAKE_RUNTIME_OUTPUT_DIRECTORY sau khi đóng M6. Bản Debug chuẩn dùng `build/mingw-x64-debug/RoomCraft3D.exe`.
