# RoomCraft 3D

RoomCraft 3D là ứng dụng Windows x64 để bố trí nội thất trong phòng 3D. Bạn có thể dùng phòng mẫu, điều chỉnh ánh sáng, kiểm tra va chạm và lưu bố cục dưới dạng tệp `.roomcraft`.

## Chạy ứng dụng

Nếu đã có gói portable, giải nén cả thư mục và mở `RoomCraft3D.exe`. Để build từ mã nguồn, cần CMake, Ninja và WinLibs MinGW-w64 x64 UCRT.

```powershell
$winlibsBin = (Get-ChildItem "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\BrechtSanders.WinLibs*" -Directory | Select-Object -First 1).FullName + "\mingw64\bin"
$env:PATH = "$winlibsBin;$env:PATH"

cmake --preset mingw-x64-release
cmake --build --preset mingw-x64-release
.\build\mingw-x64-release\RoomCraft3D.exe
```

Trong editor, chọn **Room Presets → Modern Bedroom** để bắt đầu. Thêm nội thất từ **Furniture Library**, chỉnh vị trí trong **Properties**, rồi dùng **File → Save As** để lưu. Thư mục `examples/` có các dự án mẫu có thể mở từ **File → Open Project**.

## Hướng dẫn

- [Sử dụng RoomCraft 3D Editor](docs/EDITOR_USER_GUIDE.md)
- [Cài đặt công cụ và build trên Windows](docs/BUILD_GUIDE.md)
- [Chuẩn bị nội thất trong Blender](docs/BLENDER_GUIDE.md)
- [Đóng gói bản portable](docs/PACKAGING_GUIDE.md)
- [Định dạng tệp .roomcraft](docs/PROJECT_FORMAT.md)

## Phát triển

Mã nguồn dùng C++17, OpenGL 3.3 Core, FreeGLUT và Dear ImGui. Xem quy ước phát triển trong [AGENTS.md](AGENTS.md). Sau khi build, chạy CTest từ thư mục gốc:

```powershell
ctest --test-dir build\mingw-x64-release --output-on-failure
```

## Giấy phép

[LICENSE](LICENSE) · [Giấy phép tài nguyên](docs/ASSET_LICENSES.md) · [Thông báo thư viện bên thứ ba](licenses/THIRD_PARTY_NOTICES.txt)
