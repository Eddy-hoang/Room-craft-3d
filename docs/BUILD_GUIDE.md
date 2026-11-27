# Hướng dẫn cài đặt và Build trên Windows x64 — RoomCraft 3D

## 1. Môi trường đã kiểm chứng (Milestone M0)

- **Hệ điều hành**: Windows 11 Home Single Language 64-bit (`10.0.26200`)
- **GPU**: Intel(R) UHD Graphics (Driver `32.0.101.7088`, hỗ trợ OpenGL 3.3 Core Profile & GLSL 3.30)
- **Compiler**: WinLibs MinGW-w64 x64 UCRT (`x86_64-w64-mingw32-g++ 16.2.0`)
- **Build Tools**: CMake `4.4.4`, Ninja `1.13.2`, Git `2.47.1`, Python `3.13.5` (dùng cho GLAD generator ở chế độ FetchContent)

## 2. Cách 1: Build bằng WinLibs MinGW-w64 x64 + Ninja (Không cần quyền Admin)

Đây là cấu hình đã được biên dịch và kiểm chứng thực tế trên máy phát triển hiện tại.

### Bước 1: Cài đặt WinLibs x64 (tích hợp sẵn GCC x64, CMake và Ninja)
```powershell
winget install --id BrechtSanders.WinLibs.POSIX.UCRT --exact --accept-package-agreements --accept-source-agreements
```

### Bước 2: Đảm bảo WinLibs `mingw64\bin` đứng trước các bản MinGW cũ trong PATH
> **Lưu ý quan trọng**: Nếu máy có sẵn `C:\MinGW\bin` (bản MinGW.org 32-bit cũ trong System PATH), hãy đặt thư mục `mingw64\bin` của WinLibs lên đầu `PATH` của phiên PowerShell hiện tại:

```powershell
$winlibsBin = (Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*" -Directory | Select-Object -First 1).FullName + "\mingw64\bin"
$env:PATH = "$winlibsBin;$env:PATH"
```

### Bước 3: Configure và Build (Debug)
```powershell
cd d:\CG
cmake --preset mingw-x64-debug
cmake --build --preset mingw-x64-debug
```

CMake sẽ tự động tải đúng các phiên bản đã khóa commit SHA (`FreeGLUT v3.8.0`, `GLAD v0.1.36`, `Dear ImGui v1.93.0-docking`), sinh GLAD cho OpenGL 3.3 Core Profile, biên dịch và sao chép thư mục `assets/` sang cạnh `RoomCraft3D.exe`.

### Bước 4: Chạy ứng dụng và kiểm thử
```powershell
# Chạy chế độ cửa sổ tương tác bình thường (không cần đặt PATH WinLibs vì runtime đã liên kết tĩnh)
.\build\mingw-x64-debug\RoomCraft3D.exe

# Chạy kiểm thử tự động M0 qua CTest
ctest --test-dir build\mingw-x64-debug --output-on-failure -V
```

## 3. Cách 2: Build bằng Visual Studio 2022 (MSVC x64) + vcpkg (Tùy chọn khuyến nghị)

Dự án đã chuẩn bị sẵn `vcpkg.json` (khóa `builtin-baseline`) và preset `msvc-x64-vcpkg-debug` để sử dụng khi máy đã cài Visual Studio 2022 / Build Tools và vcpkg.

1. Cài **Visual Studio 2022 Community** hoặc **Visual Studio 2022 Build Tools** với workload **Desktop development with C++** (cần quyền Administrator).
2. Cài đặt và bootstrap `vcpkg`, đặt biến môi trường `VCPKG_ROOT` trỏ tới thư mục `vcpkg`.
3. Chạy lệnh trong *Developer PowerShell for VS 2022*:
```powershell
cd d:\CG
cmake --preset msvc-x64-vcpkg-debug
cmake --build --preset msvc-x64-vcpkg-debug
.\build\msvc-x64-vcpkg\Debug\RoomCraft3D.exe
```

> *Lưu ý*: Preset `msvc-x64-vcpkg-debug` chưa được chạy kiểm chứng thực tế trên máy hiện tại do máy chưa cài MSVC Build Tools và vcpkg (xem [`docs/KNOWN_ISSUES.md`](KNOWN_ISSUES.md)).

## 4. Xử lý sự cố thường gặp

- **Lỗi chọn nhầm `g++ 6.3.0` 32-bit**: Preset `mingw-x64-debug` đã chỉ định rõ `x86_64-w64-mingw32-gcc` và `x86_64-w64-mingw32-g++`, đảm bảo không dùng nhầm `C:\MinGW\bin\g++.exe`.
- **Không tìm thấy shader khi chạy**: Ứng dụng tự dò `assets/shaders/basic.vert` và `basic.frag` theo thư mục chứa file thực thi trước rồi mới tới thư mục làm việc hiện tại (`cwd`). Nếu sao chép `.exe` sang nơi khác, hãy copy kèm thư mục `assets/`.
