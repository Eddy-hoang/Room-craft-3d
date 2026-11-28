# Hướng dẫn Sử dụng RoomCraft 3D Editor (Editor User Guide — M6–M8)

Tài liệu này hướng dẫn đầy đủ cách sử dụng **RoomCraft 3D** từ phiên bản **`v0.6.0` (Milestone M6)** đến **`v0.8.0` (Milestone M8)** với giao diện sản phẩm hoàn chỉnh, ánh sáng Blinn–Phong, kiểm tra va chạm 3D và lưu/mở dự án `.roomcraft`.

---

## 1. Khởi chạy & Tổng quan Giao diện

Chạy trực tiếp file thực thi (không cần thêm MinGW vào PATH):

```powershell
.\build\mingw-x64-debug\RoomCraft3D.exe
```

Khi khởi động, ứng dụng mở trực tiếp không gian làm việc **Room Editor** với căn phòng mẫu **Modern Bedroom (`5.0 × 6.0 × 3.0 m`)**. Giao diện gồm 6 vùng chính:

1. **Main Menu Bar** (trên cùng): `RoomCraft` · `File` · `Edit` · `View` · `Room` · `Tools` · `Help`, bên phải hiển thị tên căn phòng đang thiết kế.
2. **Main Toolbar**: Các nút thao tác nhanh có biểu tượng vector sắc nét, tooltip kèm phím tắt và menu chọn nhanh **Room Presets**.
3. **Furniture Library** (cột trái): Duyệt **14 mẫu nội thất** (6 Procedural C++ + 8 Blender) dưới dạng **Grid ảnh thu nhỏ 256×256 thật** hoặc **List View**, tìm kiếm, lọc theo nguồn/danh mục và nhấn **`+ Add Furniture`** (hoặc nhấp đúp thẻ).
4. **3D Room Viewport** (vùng trung tâm lớn nhất): Không gian thiết kế 3D thời gian thực, góc trên-trái hiển thị chế độ góc nhìn, góc trên-phải chuyển nhanh `Solid / Wireframe`, góc dưới-trái hiển thị chế độ tương tác, góc dưới-phải hiển thị thông báo Toast.
5. **Scene Hierarchy & Properties Inspector** (cột phải):
   - **Trên**: Cây cấu trúc phòng (`Floor & Walls` + danh sách toàn bộ đồ nội thất có nút biểu tượng **Ẩn/Hiện** và **Khóa/Mở khóa**, hỗ trợ **chuột phải Context Menu**).
   - **Dưới**: Hai tab **`Properties`** (chỉnh tên, Transform X/Y/Z mã màu, kích thước, vật liệu, hình học, thao tác) và **`Room Settings`** (kích thước phòng, độ dày tường, lưới, màu sắc, chế độ ẩn tường).
6. **Status Bar** (dưới cùng): Kích thước phòng, số đối tượng, bước lưới, chế độ hiện tại, tọa độ thế giới con trỏ và trạng thái cảnh (`Scene ready`).

> **Mẹo bố cục**: Bạn có thể kéo các thanh chia dọc/ngang giữa các panel để mở rộng Viewport hoặc danh sách nội thất, và dùng **`View -> Reset Workspace Layout`** để đưa về mặc định bất kỳ lúc nào.

---

## 2. Thanh Menu Chính (Main Menu Bar)

| Menu | Chức năng chính |
|---|---|
| **File** | `New Project (Ctrl+N)`, `Open Project... (Ctrl+O)`, `Save (Ctrl+S)`, `Save As... (Ctrl+Shift+S)`, `Recent Projects`, `Room Presets` (`Modern Bedroom`, `Gaming Workspace`, `Minimalist Living Room`), `Exit` (hỏi `Save / Discard / Cancel` nếu có thay đổi chưa lưu). |
| **Edit** | `Undo (Ctrl+Z)`, `Redo (Ctrl+Y)`, chuyển công cụ (`Select Q`, `Move G`, `Rotate R`, `Scale S`), `Duplicate Selected (Ctrl+D)`, `Delete Selected (Del)`, `Reset Transform`. |
| **View** | Góc nhìn (`Perspective 2`, `Top 2D 1`, `Isometric 3`, `Front`), `Fit Room (Home)`, `Fit Selected (F)`, bật/tắt `Grid`, `Axes`, `Wireframe`, `Selection Bounding Box`, `3-Axis Move Gizmo`, `UI Scale (100% / 125% / 150%)`, `Reset Workspace Layout`, `Performance Monitor`. |
| **Room** | Mở nhanh `Room Settings...`, đổi nhanh `Wall Visibility` (`All Walls`, `Hide Ceiling`, `Cutaway View`, `Manual`), `Toggle Ceiling`, `Room Presets`. |
| **Tools** | Quay về `Room Editor (Main Workspace)`, mở **`Geometry Lab`** (công cụ sinh hình học Procedural/Bézier/Revolution), mở **`Asset Inspector`** (kiểm tra mesh/material/texture và `Reload Asset`), hoặc mở **`Developer Tools`** (`Graphics Reference Scene`, `OpenGL Diagnostics`, `Performance Monitor`). |
| **Help** | `Keyboard Shortcuts`, `User Guide`, `System Information`, `About RoomCraft`. |

> **Bảo toàn cảnh làm việc**: Khi bạn mở `Tools -> Geometry Lab` hoặc `Tools -> Asset Inspector` để kiểm tra hình học/tài nguyên, toàn bộ căn phòng, đồ nội thất, camera và đối tượng đang chọn trong **Room Editor** vẫn được giữ nguyên. Bấm nút **`Return to Room Editor`** ở góc phải thanh menu (hoặc `Tools -> Room Editor`) để quay lại ngay.

---

## 3. Thao tác Cơ bản trong Room Editor

### Thêm và duyệt nội thất
- Gõ từ khóa vào ô **`Search furniture...`**, chọn bộ lọc nguồn (`All` / `C++` / `OBJ`) hoặc danh mục (`Seating`, `Tables`, `Storage`, `Lighting`, `Beds`, `Electronics`, `Decor`).
- Chuyển đổi giữa **Grid View** (ảnh thu nhỏ 2 cột) và **List View** bằng hai nút biểu tượng ở góc trên-phải panel.
- Nhấp để chọn trước một mẫu rồi bấm **`+ Add Furniture`** (hoặc **nhấp đúp** trực tiếp vào thẻ) để thêm vào căn phòng.

### Chọn và di chuyển bằng 3-Axis Move Gizmo
- **Click chuột trái** lên bất kỳ món nội thất nào trong 3D Viewport để chọn (nhấp vùng trống để bỏ chọn).
- Khi đối tượng được chọn (ở chế độ `Select` hoặc `Move` và đang bật `Gizmo`), **3-Axis Move Gizmo** xuất hiện tại tâm đáy vật thể:
  - Kéo mũi tên **Đỏ (X)** để trượt dọc trục X.
  - Kéo mũi tên **Xanh lá (Y)** để nâng/hạ cao độ (ví dụ đặt đèn hoặc máy tính lên mặt bàn `Y = 0.75 m`).
  - Kéo mũi tên **Xanh dương (Z)** để trượt dọc trục Z.
- Bạn vẫn có thể kéo tự do trên mặt phẳng ngang bằng cách kéo trực tiếp vào thân vật thể ở chế độ `Move (G)` — cao độ `Y` hiện tại luôn được giữ nguyên.
- Nhấn **`Esc`** trong lúc đang kéo/xoay/scale để hủy thao tác và trả vật thể về vị trí ban đầu.

### Quản lý bằng Scene Hierarchy & Context Menu
- Click vào một dòng để chọn nhanh; bấm biểu tượng **Con mắt** để ẩn/hiện; bấm biểu tượng **Ổ khóa** để khóa/mở khóa (vật thể khóa không bị dịch chuyển/xóa nhầm).
- **Nhấp chuột phải** lên bất kỳ mục nào trong Hierarchy để mở Context Menu: `Focus Camera (F)`, `Duplicate (Ctrl+D)`, `Hide / Show`, `Lock / Unlock`, `Reset Transform`, `Delete (Del)`.

---

## 4. Bảng Phím tắt Nhanh

| Phím tắt | Chức năng |
|---|---|
| **Chuột phải kéo / Chuột giữa kéo / Con lăn** | Orbit Camera / Pan Camera / Zoom Camera |
| **`Q` / `G` / `R` / `S`** | `Select` / `Move` / `Rotate` / `Scale` |
| **`X` / `Y` / `Z`** | Bật/tắt giới hạn trục tương ứng khi đang biến đổi |
| **`1` / `2` / `3`** | `Top View (2D Ortho)` / `Perspective 3D` / `Isometric` |
| **`F`** / **`Home`** | `Focus Selected` / `Fit Room` |
| **`Ctrl + D`** / **`Delete`** | Nhân bản / Xóa đối tượng đang chọn |
| **`Ctrl + N`** / **`Ctrl + O`** | Tạo dự án mới (`New Project`) / Mở dự án `.roomcraft` (`Open Project...`) |
| **`Ctrl + S`** / **`Ctrl + Shift + S`** | Lưu dự án (`Save`) / Lưu thành tệp mới (`Save As...`) |
| **`Ctrl + Z`** / **`Ctrl + Y`** | Hoàn tác (`Undo`) / Làm lại (`Redo`) (tối đa 64 bước) |
| **`Esc`** | Hủy thao tác đang chạy & khôi phục Transform ban đầu |

---

## 5. Ánh sáng và vật liệu (M7)

Trong cột phải, chọn **Room Settings → Lighting Settings**. Chọn Daylight, Warm Interior, Neutral Studio hoặc Evening. Các thanh chỉnh có tác dụng từ khung kế tiếp. Direction chỉ hướng tia sáng đi tới phòng. Position dùng mét trong hệ tọa độ phòng. Constant, Linear và Quadratic điều chỉnh độ suy giảm đèn điểm. Khi sửa một tham số ánh sáng, tên preset chuyển thành Custom. Reset lighting khôi phục Daylight.

Chọn Off, 2x hoặc 4x ở MSAA. Active samples báo số mẫu thật của driver. Off giảm chi phí bộ nhớ và resolve. Mở **Properties → Material** của vật đang chọn để xem nguồn màu diffuse, hệ số phản xạ Ks và số mũ độ bóng Ns từng vật liệu. Thanh **Shininess scale** nhân Ns của từng phần trên riêng vật đang chọn. Đặt lại 1,00x để trở về vật liệu gốc. Vật bị khóa cũng khóa thanh chỉnh này. Khi so màu sàn, dùng nút Grid của viewport để bật/tắt lưới.

---

## 6. Kiểm tra Va chạm & Lưu/Mở Dự án (M8)

### Kiểm tra va chạm (OBB SAT) và biên phòng (Room Boundary)
- Mở **Room Settings → Collision & Placement** để chọn chế độ kiểm tra:
  - **`Off`**: Tắt kiểm tra va chạm/biên phòng.
  - **`Warning Only`** *(mặc định)*: Cho phép đặt vật thể tự do nhưng tô khung cảnh báo trong 3D Viewport (**vàng cam** khi xuyên nhau giữa 2 nội thất, **đỏ san hô** khi vượt biên phòng) trong khi vẫn giữ khung **xanh ngọc (cyan)** của đối tượng đang chọn.
  - **`Prevent Invalid`**: Ngăn hoàn tất thao tác di chuyển/xoay/scale nếu kết quả cuối cùng xuyên vật thể khác hoặc vượt biên phòng — hệ thống tự hoàn nguyên về Transform hợp lệ trước đó và hiện Toast giải thích lý do.
- Có thể bật/tắt riêng **`Allow Rug / Carpet Overlap`** (mặc định bật để cho phép ghế, bàn, giường đặt trên thảm mà không báo lỗi giả) và **`Show Collision Highlights`**.
- Trong **Properties → Dimensions & Placement**, trạng thái đặt của vật thể đang chọn hiển thị trực tiếp (`Placement: Valid`, danh sách vật thể đang xuyên kèm độ xuyên theo mét `m`, hoặc vượt biên phòng); **Status Bar** ở cạnh dưới luôn tổng hợp số cảnh báo toàn cảnh.

### Lưu, mở dự án `.roomcraft` và Undo/Redo
- Dùng menu **`File`** hoặc phím tắt **`Ctrl+N`** (New), **`Ctrl+O`** (Open), **`Ctrl+S`** (Save), **`Ctrl+Shift+S`** (Save As) để mở hộp thoại tệp gốc của Windows hỗ trợ đường dẫn Unicode và dấu cách.
- Khi dự án có thay đổi chưa lưu, dấu **`*`** xuất hiện cạnh tên dự án trên thanh tiêu đề cửa sổ và thanh menu. Nếu bạn thực hiện `New`, `Open`, `Load Preset` hoặc đóng cửa sổ (`File -> Exit` hay nút **`X`**), hộp thoại **`Unsaved Changes`** sẽ hỏi **`Save`** / **`Discard`** / **`Cancel`** để tránh mất dữ liệu.
- Danh sách tối đa 8 dự án gần nhất nằm tại **`File -> Recent Projects`**.
- Lưu dự án dùng cơ chế **ghi tệp tạm cùng thư mục rồi thay thế nguyên tử (`MoveFileExW`)**; mở dự án dùng cơ chế **transactional** — nếu tệp hỏng hoặc sai schema, dự án hiện đang mở vẫn được giữ nguyên vẹn.
- **`Edit -> Undo (Ctrl+Z)`** và **`Edit -> Redo (Ctrl+Y)`** hỗ trợ hoàn tác/làm lại cho các thao tác thêm, xóa, nhân bản, biến đổi (1 lần kéo = 1 bước Undo), đổi tên, ẩn/hiện, khóa/mở khóa, đổi độ bóng vật liệu và đổi thiết lập phòng.
