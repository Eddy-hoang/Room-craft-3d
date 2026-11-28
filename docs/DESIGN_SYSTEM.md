# Hệ thống Thiết kế Giao diện — RoomCraft 3D (Design System — M6)

Tài liệu này định nghĩa bộ **Design Tokens**, **Typography**, **Vector Icon System** và **quy chuẩn UI components** áp dụng thống nhất trên toàn bộ giao diện **RoomCraft 3D** từ **Milestone M6 (`v0.6.0`)**.

Mọi hằng số màu, bo góc và kích thước được tập trung tại [`EditorTheme.hpp`](../include/roomcraft/editor/EditorTheme.hpp) và [`EditorTheme.cpp`](../src/editor/EditorTheme.cpp), tránh rải màu hardcode ở từng panel.

---

## 1. Bảng màu (Modern Dark CAD Theme Tokens)

Triết lý màu sắc lấy cảm hứng từ các phần mềm CAD / 3D hiện đại (Blender, Unreal Editor, Figma Dark): nền tối độ tương phản vừa phải để mô hình 3D và vật liệu nội thất nổi bật, không gây mỏi mắt và giữ phân cấp rõ ràng giữa nền Viewport, bề mặt Panel và bề mặt Elevated/Input.

| Token (`editor::tokens`) | Mã HEX | Giá trị RGBA | Vai trò |
|---|---|---|---|
| `kViewportBg` | `#171A20` | `(0.090, 0.102, 0.125, 1.00)` | Nền ứng dụng & vùng Viewport 3D |
| `kPanelSurface` | `#20242B` | `(0.125, 0.141, 0.169, 1.00)` | Nền các panel chính (Library, Hierarchy, Inspector, Toolbar) |
| `kElevatedSurface` | `#292E38` | `(0.161, 0.180, 0.220, 1.00)` | Thẻ nội thất, ô nhập liệu, popup, header nổi |
| `kHeaderSurface` | `#1C2027` | `(0.110, 0.125, 0.153, 1.00)` | Thanh menu chính, thanh trạng thái |
| `kPrimary` | `#799CE3` | `(0.475, 0.612, 0.890, 1.00)` | Màu nhấn chính (nút `+ Add Furniture`, công cụ đang bật, chọn mục) |
| `kPrimaryHover` | `#8EB0F2` | `(0.557, 0.690, 0.949, 1.00)` | Trạng thái hover của phần tử Primary |
| `kPrimaryActive` | `#5F84D1` | `(0.373, 0.518, 0.820, 1.00)` | Trạng thái nhấn giữ của phần tử Primary |
| `kSelectionBg` | `#40577C` | `(0.251, 0.341, 0.486, 0.72)` | Nền mục đang chọn trong Hierarchy / Library |
| `kTextPrimary` | `#E6EAF2` | `(0.902, 0.918, 0.949, 1.00)` | Văn bản chính, tiêu đề panel, giá trị số |
| `kTextSecondary` | `#929FB2` | `(0.573, 0.624, 0.698, 1.00)` | Nhãn phụ, metadata, placeholder, đơn vị |
| `kTextMuted` | `#667285` | `(0.400, 0.447, 0.522, 1.00)` | Trạng thái disabled, phần tử ẩn/khóa |
| `kBorder` | `#3A424F` | `(0.227, 0.259, 0.310, 1.00)` | Đường viền tinh tế 1px giữa panel, card và nút |
| `kSuccess` | `#79B69A` | `(0.475, 0.714, 0.604, 1.00)` | Thông báo thành công, đèn trạng thái sẵn sàng |
| `kWarning` | `#E6AE5D` | `(0.902, 0.682, 0.365, 1.00)` | Cảnh báo (vật thể khóa, vượt biên phòng, thiếu texture) |
| `kDanger` | `#DF7878` | `(0.875, 0.471, 0.471, 1.00)` | Hành động hủy/xóa (`Delete`, `Confirm Replace`) |

### Màu trục tọa độ 3D (Axis Color Coding — Inspector & Move Gizmo)

| Trục | Token | Mã HEX | Ứng dụng |
|---|---|---|---|
| **X** | `kAxisX` | `#E35B5B` | Nhãn `X` trên ô Transform + Tay cầm trục X của Move Gizmo + Trục thế giới +X |
| **Y** | `kAxisY` | `#5ECA7A` | Nhãn `Y` trên ô Transform + Tay cầm trục Y của Move Gizmo + Trục thế giới +Y |
| **Z** | `kAxisZ` | `#5B9CEB` | Nhãn `Z` trên ô Transform + Tay cầm trục Z của Move Gizmo + Trục thế giới +Z |
| **Selection BBox** | Cyan Highlight | `#33E0FF` | Đường viền hộp bao đối tượng đang chọn trong 3D Viewport |

---

## 2. Typography & Scaling

- **Font chính**: **`Roboto-Medium.ttf`** (`assets/fonts/Roboto-Medium.ttf`, 162,588 bytes) — Google Fonts, giấy phép **Apache License 2.0** (đã khai báo tại [`docs/ASSET_LICENSES.md`](ASSET_LICENSES.md)).
- **Kích thước chuẩn**: Base UI `15 px` (cân bằng giữa độ rõ nét khi xem trên 1280×720 và mật độ thông tin của ứng dụng CAD), nạp kèm dải glyph **Latin + Vietnamese** (`GetGlyphRangesVietnamese()`) để hiển thị sạch cả tiếng Anh và tiếng Việt có dấu.
- **Cơ chế Fallback an toàn**: Nếu vì lý do nào đó file `assets/fonts/Roboto-Medium.ttf` không tồn tại hoặc lỗi đọc, `EditorTheme::loadTypography()` tự động rơi về font mặc định của Dear ImGui (`Default (Fallback)`) và ghi log cảnh báo — tuyệt đối không crash ứng dụng.
- **UI Scaling (`View -> UI Scale`)**: Hỗ trợ chuyển trực tiếp **`100%`**, **`125%`** và **`150%`** tại runtime thông qua `ImGuiIO::FontGlobalScale` kết hợp đồng bộ lại kích thước style (`ImGuiStyle::ScaleAllSizes`), giúp chữ và control dễ đọc trên màn hình HiDPI / laptop Full HD.

---

## 3. Hệ thống Biểu tượng Vector (`EditorIcons`)

Thay vì dùng ký tự emoji Unicode (thường hiển thị thành ô vuông vỡ trên Windows khi font thiếu glyph) hoặc ghép thêm font icon bên thứ ba có rủi ro giấy phép/font-atlas, RoomCraft 3D sử dụng bộ **biểu tượng vector thuần túy** vẽ trực tiếp qua `ImDrawList` tại [`EditorIcons.hpp`](../include/roomcraft/editor/EditorIcons.hpp) và [`EditorIcons.cpp`](../src/editor/EditorIcons.cpp):

- **Sắc nét độc lập DPI**: Mọi nét vẽ được tính theo tọa độ tâm `(center.x, center.y)` và kích thước `size`, nên phóng to 125%/150% không bị răng cưa hay nhòe pixel.
- **Giấy phép**: Mã nguồn gốc của dự án **RoomCraft 3D** (**MIT**).
- **Danh mục 21 biểu tượng đang dùng**:
  - `AppLogo`, `NewFile`, `Select`, `Move`, `Rotate`, `Scale`, `Snap`, `Fit`, `TopView`, `View3D`, `Grid`, `Wireframe`, `Gizmo`, `Plus`, `Duplicate`, `Trash`, `EyeOpen`, `EyeClosed`, `LockClosed`, `LockOpen`, `Search`, `GridView`, `ListView`, `Warning`, `Check`, `Info`, `ChevronDown`.

---

## 4. Spacing, Bo góc & Phân cấp Component

### Hệ lưới khoảng cách (Spacing Scale)
- `kSpaceXs = 4 px` — khoảng cách nội bộ giữa biểu tượng và chữ hoặc giữa 3 ô trục X/Y/Z.
- `kSpaceSm = 8 px` — khoảng cách giữa các nút công cụ, padding khung nhập liệu.
- `kSpaceMd = 12 px` — padding tiêu chuẩn bên trong panel và thẻ nội thất.
- `kSpaceLg = 16 px` — khoảng phân tách giữa các section lớn trong Inspector.
- `kSpaceXl = 24 px` — khoảng thở lớn trong các hộp thoại modal (`About`, `Shortcuts`, `Confirm`).

### Bo góc & Viền
- `WindowRounding = 0 px` (các dock/split panel dính liền thành bố cục thống nhất), `PopupRounding = 6 px`, `FrameRounding = 5 px`, `GrabRounding = 4 px`, `TabRounding = 5 px`.
- `WindowBorderSize = 1 px`, `FrameBorderSize = 1 px` dùng màu `kBorder` để tách nhẹ các bề mặt mà không tạo cảm giác chật chội.

### Quy tắc Thành phần Giao diện
1. **Primary Action** (`+ Add Furniture`, nút xác nhận): Nền `kPrimary` (`#799CE3`), chữ tối tương phản cao.
2. **Secondary Action** (`Duplicate`, `Reset Transform`, nút toolbar): Nền `kElevatedSurface` (`#292E38`), viền `kBorder`, nổi bật bằng `kPrimary` khi đang kích hoạt (ví dụ `Move`, `Snap`, `Grid`).
3. **Destructive Action** (`Delete`, `Confirm Replace`): Viền/nhãn sắc `kDanger` (`#DF7878`) và luôn đi kèm hộp thoại xác nhận khi có nguy cơ mất dữ liệu hoặc xóa đối tượng.
4. **Transform Inputs**: Ba ô độc lập màu đỏ (`X`), xanh lá (`Y`), xanh dương (`Z`), căn thẳng hàng, nhập số chính xác hoặc kéo giá trị trực tiếp.
