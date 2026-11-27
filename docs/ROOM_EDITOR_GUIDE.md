# Hướng dẫn sử dụng Room Editor — RoomCraft 3D (Milestone M4)

## 1. Khởi chạy ứng dụng

Từ thư mục gốc `D:\CG` (chạy độc lập, không cần thiết lập PATH WinLibs):

```powershell
.\build\mingw-x64-debug\RoomCraft3D.exe
```

Mặc định chương trình mở trực tiếp **Room Editor (M4)** với căn phòng mẫu **Modern Bedroom (`5.0 × 6.0 × 3.0 m`)**. Trên thanh menu trên cùng, bạn có thể chuyển qua lại bất kỳ lúc nào giữa:
- **`Room Editor (M4)`**
- **`Asset Browser (M3)`**
- **`Geometry Playground (M2)`**
- **`M1 Reference Scene`**

---

## 2. Bố cục giao diện Room Editor

1. **Main Toolbar** (ngay dưới menu):
   - **Tạo nhanh cảnh**: `New Room` (phòng trống 5×6×3 m), `Bedroom` (Modern Bedroom), `Gaming` (Gaming Workspace), `Living` (Minimalist Living Room). Nếu bạn đã chỉnh sửa cảnh hiện tại, chương trình sẽ mở hộp thoại xác nhận trước khi thay thế.
   - **Thao tác nhanh**: `Duplicate`, `Delete` cho vật thể đang chọn.
   - **Góc nhìn Camera**: `3D` (Perspective), `Iso` (Isometric Orthographic), `Top 2D` (mặt bằng từ trên xuống), `Front` (mặt đứng), `Fit Room`, `Fit Sel` (căn khung theo vật thể đang chọn).
   - **Tùy chọn nhanh**: `Wire` (Wireframe), `BBox` (khung bao đối tượng chọn), `Snap` (bật Grid Snapping theo bước lưới sàn).
2. **Cột trái — Furniture Library & Scene Hierarchy**:
   - **Furniture Library**: Tìm kiếm theo tên, lọc theo nguồn (`All` / `Procedural` / `Blender`) và danh mục (`Seating`, `Tables`, `Storage`, `Lighting`, `Beds`, `Electronics`, `Decor`), nhấn **`+ Add`** để thêm vào phòng.
   - **Scene Hierarchy**: Hiển thị cấu trúc phòng (`Room Structure`) và danh sách toàn bộ `Furniture Objects` với checkbox ẩn/hiện, chọn đối tượng, nút **`D`** (nhân bản) và **`X`** (xóa).
3. **Vùng trung tâm — 3D Room Viewport**:
   - Hiển thị căn phòng 3D render qua Off-screen Framebuffer (tự động resize theo khung cửa sổ).
4. **Cột phải — Properties Inspector & Room Settings**:
   - **Tab `Properties Inspector`**: Xem `Object ID`, đổi tên (`Rename`), bật/tắt `Visible`, chỉnh sửa `Position (X, Y, Z)`, `Rotation (X, Y, Z)`, `Scale (X, Y, Z)` (hỗ trợ `Uniform Scale`), `Reset Transform`, `Fit Object`, xem kích thước Asset/World BBox và cảnh báo nếu vật thể vượt ra ngoài phạm vi phòng.
   - **Tab `Room Settings`**: Chỉnh sửa `Width`, `Length`, `Height`, `Wall Thickness`, `Grid Step`, màu sàn/tường/trần, chế độ ẩn tường (`All Walls`, `Hide Ceiling`, `Cutaway View`, `Manual` bật/tắt từng bức tường Bắc/Nam/Tây/Đông và trần), nhấn **`Apply Changes`** hoặc **`Reset Room`**. Hiển thị diện tích sàn ($m^2$), thể tích ($m^3$) và giới hạn bên trong phòng.
5. **Status Bar** (dưới cùng): Hiển thị FPS, frame time, số vật thể hiển thị/tổng, số Unique GPU Assets đang tái sử dụng, số draw calls, số tam giác và thông báo trạng thái gần nhất.

---

## 3. Đặt vật thể lên mặt bàn / kệ (Transform Y)

RoomCraft 3D không ép mọi vật thể ở `Y = 0`:
- Ví dụ `Procedural Desk` có chiều cao mặt bàn mặc định `0.75 m`.
- Chọn `Computer Setup` hoặc `Desk Lamp` trong **Scene Hierarchy**, đặt `Position Y = 0.750` trong **Properties Inspector** để đặt khít lên mặt bàn.
- Với thảm (`Minimalist Carpet`), mặc định `Position Y = 0.003` (3 mm) để tránh hiện tượng z-fighting với mặt sàn `Y = 0`.
