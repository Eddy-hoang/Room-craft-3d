# Hướng dẫn Đóng gói Portable (Packaging Guide — `v1.0.0-rc1`)

## 1. Quy trình Đóng gói Tái lập

1. Build bản **Release** (không ghi đè bản Debug):
   ```powershell
   $winlibsBin = (Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*" -Directory | Select-Object -First 1).FullName + "\mingw64\bin"
   $env:PATH = "$winlibsBin;$env:PATH"
   cmake --preset mingw-x64-release
   cmake --build --preset mingw-x64-release
   ctest --test-dir build\mingw-x64-release --output-on-failure
   ```
2. Chạy script đóng gói tự động [`scripts/package_portable.ps1`](../scripts/package_portable.ps1):
   ```powershell
   powershell -ExecutionPolicy Bypass -File scripts\package_portable.ps1
   ```
3. Đầu ra tạo tại:
   - Thư mục staged: `dist/RoomCraft3D-v1.0.0-rc1-win64/` (`48` tệp)
   - Gói nén phát hành: `dist/RoomCraft3D-v1.0.0-rc1-win64.zip` (`2,831,544` bytes ≈ `2.70 MB`)
   - SHA-256 ZIP: `23ef9d9596994e4fc0293ecf22c229a8c83c0a74e90b8bd041e18b680d496e79`

---

## 2. Cấu trúc Gói Portable

```text
RoomCraft3D-v1.0.0-rc1-win64/
├── RoomCraft3D.exe            (7,960,281 bytes — Release x64, liên kết tĩnh runtime MinGW + icon nhúng)
├── assets/
│   ├── shaders/               (basic, grid, mesh, shadow_depth GLSL 330 core)
│   ├── fonts/                 (Roboto-Medium.ttf — Apache 2.0)
│   ├── icons/                 (roomcraft.ico)
│   └── models/                (8 mẫu Blender .obj/.mtl + 11 .png textures; đã loại bỏ .blend nguồn)
├── examples/
│   ├── ModernBedroom.roomcraft
│   ├── GamingWorkspace.roomcraft
│   └── MinimalistLivingRoom.roomcraft
├── docs/
│   ├── EDITOR_USER_GUIDE.md
│   └── PROJECT_FORMAT.md
├── licenses/
│   ├── THIRD_PARTY_NOTICES.txt
│   └── ASSET_LICENSES.md
├── README.txt
├── CHANGELOG.md
└── SHA256SUMS.txt
```

- **Không bao gồm**: `.git/`, `build/`, tệp `.blend` authoring, `imgui.ini`, tệp cấu hình người dùng hay thư viện ngoài.

---

## 3. Kiểm chứng Phụ thuộc DLL & Chạy Cô lập

- **Phụ thuộc DLL (`ntldd build/mingw-x64-release/RoomCraft3D.exe`)**: Chỉ phụ thuộc **8 DLL hệ thống Windows có sẵn**:
  `ADVAPI32.dll`, `comdlg32.dll`, `GDI32.dll`, `KERNEL32.dll`, `OPENGL32.dll`, `SHELL32.dll`, `USER32.dll`, `WINMM.dll`.
- **Cơ chế phân giải tài nguyên & Preferences**:
  - `RoomCraft3D.exe` tìm thư mục chứa chính nó bằng Win32 `GetModuleFileNameW`, nạp shader/font/model/texture theo đường dẫn tương đối Unicode nên không phụ thuộc `D:\CG` hay thư mục làm việc hiện hành (`CWD`).
  - Tùy chọn bố cục giao diện lưu tại `%APPDATA%\RoomCraft3D\preferences.json` (thư mục luôn có quyền ghi ngay cả khi giải nén app vào vị trí chỉ đọc).
- **Kết quả kiểm thử cô lập thực tế** *(giải nén từ ZIP trên máy phát triển, không có WinLibs trong `PATH`, `CWD = %TEMP%`, đường dẫn chứa dấu tiếng Việt & khoảng trắng)*:
  - Đường dẫn thử nghiệm: `C:\Users\LENOVO\AppData\Local\Temp\Thử nghiệm RoomCraft 3D (Isolated)\RoomCraft3D-v1.0.0-rc1-win64\RoomCraft3D.exe`
  - `PATH = C:\Windows\System32;C:\Windows`
  - Kết quả: `--version` (Exit `0`), `--m8-smoke-test` (Exit `0`, đủ 14 asset & round-trip save/load), `--m9-smoke-test` (Exit `0`, Shadow Mapping + MSAA 4x, `OpenGL errors: NONE`).
  - *Ghi chú trung thực*: Kiểm thử trên là kiểm thử thư mục cô lập ngoài PATH trên máy phát triển, chưa thực hiện trên một máy tính Windows sạch thứ hai.
