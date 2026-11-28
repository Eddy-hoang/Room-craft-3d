# Kế hoạch kiểm thử — RoomCraft 3D (Test Plan)

## 1. Chiến lược kiểm thử

Dự án áp dụng mô hình kiểm thử 3 tầng với **13 bộ kiểm thử CTest**:
1. **Unit Testing thuần CPU (`6` suites)**:
   - `M1_UnitTests`, `M2_GeometryUnitTests`, `M3_AssetUnitTests`, `M4_SceneUnitTests`, `M5_InteractionUnitTests`, `M6_EditorUiUnitTests`.
   - Kiểm thử độc lập toán học, sinh hình học, Assimp import, quản lý cảnh, camera/picking/transform, bố cục đáp ứng `EditorLayout`, `NotificationSystem` và toán học 3-Axis Move Gizmo không cần OpenGL context.
2. **GPU Integration & Runtime Smoke Testing (`7` suites + `--m4-benchmark`)**:
   - `M0_SmokeTest`, `M1_IntegrationSmokeTest`, `M2_IntegrationSmokeTest`, `M3_IntegrationSmokeTest`, `M4_IntegrationSmokeTest`, `M5_IntegrationSmokeTest`, `M6_IntegrationSmokeTest`.
   - Chạy trên ngữ cảnh OpenGL 3.3 Core thật.
3. **Manual & Visual GUI Testing**:
   - Người dùng chạy trực tiếp `RoomCraft3D.exe`, kiểm tra trực quan bố cục, typography, thumbnail, kéo Move Gizmo, resize cửa sổ, DPI scale và xác nhận checklist.

---

## 2. Regression M0–M5 (`11/11` CTest)

Toàn bộ 11 bộ kiểm thử từ M0 đến M5 (`TC-M0-01`..`TC-M5-39`, `TC-M5-TRI`, `TC-M5-OCC`) phải tiếp tục đạt `PASS` không có suy thoái (đặc biệt giữ nguyên chọn vật thể, highlight cyan `TC-M5-38`, Ray–Triangle và Wall Occlusion).

---

## 3. Danh mục Test Case — Milestone M6 (Professional Editor UI/UX)

Phân loại rõ theo tầng kiểm chứng: **[Auto-CPU]**, **[Auto-GPU]**, **[Manual-Visual]**.

| Test ID | Loại | Hạng mục | Kịch bản kiểm thử | Kết quả mong đợi |
|---|---|---|---|---|
| **TC-M6-01** | Auto-All | Regression M0–M5 | Chạy lại toàn bộ `11/11` CTest từ M0 đến M5 | `11/11 PASS` |
| **TC-M6-02** | Manual-Visual | Product Menu Bar | Mở `RoomCraft3D.exe`, kiểm tra thanh trên cùng | Chỉ có `RoomCraft \| File \| Edit \| View \| Room \| Tools \| Help`, không có tab milestone `M1..M5` |
| **TC-M6-03** | Manual-Visual | No Fake Menu Items | Duyệt tất cả menu con của `File / Edit / View / Room / Tools / Help` | Mọi mục menu đều thực thi được; không có `Save`/`Open`/`Undo`/`Redo` giả |
| **TC-M6-04** | Manual-Visual | Modern Dark CAD Theme | Quan sát độ tương phản nền Viewport, Panel, Elevated card, nút Primary/Danger | Đúng bảng màu `EditorTheme`, phân cấp thị giác rõ, dễ đọc |
| **TC-M6-05** | Manual-Visual | Axis Color Coding | Quan sát ô `Position / Rotation / Scale` và Move Gizmo | Trục X Đỏ, Y Xanh lá, Z Xanh dương nhất quán |
| **TC-M6-06** | Auto-GPU | TrueType Font Loading | Chạy `--m6-smoke-test` (kể cả từ CWD khác `C:\`) | Nạp thành công `assets/fonts/Roboto-Medium.ttf` (`15 px`), có fallback an toàn |
| **TC-M6-07** | Manual-Visual | Vector Icons Sharpness | Quan sát toolbar, nút `Eye/Lock`, `Grid/List`, logo ở 100%/125%/150% | Không có ký tự tofu vỡ; nét vector sắc nét ở mọi mức scale |
| **TC-M6-08** | Auto-CPU | Layout 1280×720 | Tính `EditorLayout::compute(1280, 720, ...)` | Không chồng lấn panel; Viewport là vùng diện tích lớn nhất |
| **TC-M6-09** | Auto-CPU | Layout 1366×768 & 1600×900 | Tính `EditorLayout::compute` ở các độ phân giải laptop phổ biến | Không chồng lấn; Viewport ≥ 420 px và lớn nhất |
| **TC-M6-10** | Auto-CPU | Layout 1920×1080 | Tính `EditorLayout::compute(1920, 1080, ...)` | Bố cục cân đối, tỷ lệ panel hợp lý, Viewport rộng nhất |
| **TC-M6-11** | Auto-CPU | UI Scale 100/125/150% | Tính layout với `uiScale = 1.0 / 1.25 / 1.5` trên 1280×720 | Viewport vẫn khả dụng (`>= 420 px`), các vùng không giao cắt |
| **TC-M6-12** | Auto-CPU | Interactive Splitters & Reset | Kéo `leftWidth`, `rightWidth`, `rightSplitRatio` quá giới hạn rồi gọi `reset()` | Kẹp an toàn vào min/max và khôi phục mặc định sạch sẽ |
| **TC-M6-13** | Auto-CPU | Furniture Catalog 14 Items | Kiểm tra `FurnitureCatalog` trong `M6_EditorUiUnitTests` | Đủ 14 mẫu (`6 Procedural + 8 Blender`) |
| **TC-M6-14** | Auto-CPU | Search Filter | Lọc bằng từ khóa `"chair"`, `"lamp"`, `"sofa"` | Trả chính xác tập con khớp tên/ID |
| **TC-M6-15** | Auto-CPU | Source & Category Filter | Lọc `Procedural` / `Imported` kết hợp danh mục `Seating` / `Lighting` | Trả đúng danh sách mẫu theo cả 2 điều kiện |
| **TC-M6-16** | Auto-GPU | 14/14 GPU FBO Thumbnails | `ThumbnailRenderer` render cả 14 mẫu ra FBO `256×256` | `14/14` ảnh khác màu nền (`nonBlank = 14/14`), `cacheCount = 14` |
| **TC-M6-17** | Auto-GPU | Thumbnail Cache Reuse | Gọi lại `getOrRenderThumbnail()` cho asset đã cache | Không tăng bộ đếm `totalGenerations`, trả đúng cùng `GLuint` |
| **TC-M6-18** | Auto-GPU | Reload Asset Invalidation | Gọi `invalidate(id)` sau `reloadAsset(id)` | Thumbnail cũ bị giải phóng và render mới đúng 1 lần |
| **TC-M6-19** | Manual-Visual | Grid & List View Library | Chuyển đổi Grid 2 cột ↔ List View và double-click thêm đồ | Thêm đúng nội thất vào phòng, phát Toast thông báo |
| **TC-M6-20** | Manual-Visual | Viewport Overlays | Kiểm tra pill góc trên-trái, `Solid/Wire` góc trên-phải, badge trạng thái | Không che chắn mô hình chính, chuyển `Solid/Wire` tức thì |
| **TC-M6-21** | Manual-Visual | Hierarchy Eye / Lock Icons | Nhấn nút con mắt và ổ khóa trong Scene Hierarchy | Ẩn/hiện và khóa/mở khóa đồng bộ ngay với Inspector và Viewport |
| **TC-M6-22** | Manual-Visual | Hierarchy Context Menu | Chuột phải lên 1 đối tượng trong Hierarchy | Mở menu `Focus / Duplicate / Hide / Lock / Reset Transform / Delete` hoạt động đúng |
| **TC-M6-23** | Manual-Visual | Inspector Collapsible Sections | Mở/thu gọn `Object / Transform / Dimensions / Material / Geometry / Actions` | Bố cục gọn, nhập số `X/Y/Z` mượt, không bị khóa khi bật `Snap` |
| **TC-M6-24** | Manual-Visual | Clean Status Bar | Quan sát dòng trạng thái đáy cửa sổ và mở `View -> Performance Monitor` | Status Bar gọn (`Room | Objects | Grid | Mode | Scene ready`); FPS nằm trong Performance Monitor |
| **TC-M6-25** | Auto-CPU | NotificationSystem Lifecycle | Đẩy >4 thông báo, kiểm tra giới hạn hàng đợi, hết hạn TTL (`3.0s`) và `clear()` | Giới hạn đúng 4 mục, tự xóa đúng sau TTL |
| **TC-M6-26** | Manual-Visual | Confirmation Modals | Nhấn `New Room` / đổi Preset khi đã chỉnh sửa và nhấn `Delete` trên Inspector | Hiện hộp thoại xác nhận; `Cancel` giữ nguyên, `Confirm` mới thực thi |
| **TC-M6-27** | Auto-CPU | Picking & Gizmo after Resize | Đổi kích thước Viewport + 2× DPI rồi thử hit-test Move Gizmo | Tia chuột & hit-test trục X/Y/Z vẫn khớp chính xác |
| **TC-M6-28** | Auto-CPU | Gizmo Axis Drag, Esc & Locked | Kéo trục X trên Move Gizmo → `Esc` hủy → khóa object → thử kéo lại | Trục X di chuyển chỉ theo X; `Esc` khôi phục; `Locked` chặn kéo |
| **TC-M6-29** | Manual-Visual | Keyboard Shortcuts & Text Guard | Gõ chữ `"g"`, `"r"`, `"Delete"` trong ô tìm kiếm Library & Rename | Không kích hoạt tool hay xóa vật thể |
| **TC-M6-30** | Auto-GPU | Tools -> Geometry Lab Switch | Chuyển từ `RoomEditor` sang `GeometryLab` rồi quay lại | Cảnh phòng, danh sách vật thể, selection và camera giữ nguyên 100% |
| **TC-M6-31** | Auto-GPU | Tools -> Asset Inspector Switch | Chuyển sang `AssetInspector` rồi quay lại `RoomEditor` | Cảnh phòng và camera được bảo toàn nguyên vẹn |
| **TC-M6-32** | Auto-GPU | Tools -> Developer Tools Switch | Chuyển sang `GraphicsReference` rồi quay lại `RoomEditor` | Cảnh phòng và camera được bảo toàn nguyên vẹn |
| **TC-M6-33** | Manual-Visual | Help & System Dialogs | Mở `Keyboard Shortcuts`, `User Guide`, `System Information`, `About`, `OpenGL Diagnostics` | Hiển thị chính xác thông số GPU/OpenGL/Font, đóng mượt bằng `Close` |
| **TC-M6-34** | Auto-GPU | OpenGL State Restoration | Kiểm tra FBO binding, viewport, depth test, polygon mode sau khi vẽ Thumbnail & Gizmo | Toàn bộ trạng thái OpenGL khôi phục nguyên bản |
| **TC-M6-35** | Auto-GPU | GPU Cleanup on Shutdown | Gọi `ThumbnailRenderer::destroyAll()` & đóng ứng dụng | Giải phóng sạch FBO/Texture trước khi hủy GL context, không rò rỉ |
| **TC-M6-36** | Auto-GPU | Zero GL Errors & Cross-CWD | Chạy `--m6-smoke-test` ngoài PATH WinLibs và từ `C:\` | `OpenGL errors: NONE`, thoát mã `0`, `ntldd` chỉ link 7 DLL Windows |
| **TC-M6-GIZMO** | Auto-CPU + GPU | 3-Axis Move Gizmo (P1) | Hit-test 3 trục X/Y/Z ưu tiên trước vật phía sau + render vào FBO | Trúng đúng trục X/Y/Z, vẽ nổi bật trên FBO không gây lỗi OpenGL |
| **TC-M6-PERF** | Auto-GPU | M6 30-Object & Thumbnail Perf | Đo 30 vật thể off-screen `1280×720` + thời gian sinh 14 thumbnail | Không suy giảm hiệu năng; thumbnail chỉ sinh 1 lần (`< 5 ms` tổng) |
| **TC-M6-MANUAL** | Manual-Visual | End-to-End Visual & UX Review | Người dùng chạy trực tiếp `RoomCraft3D.exe` và xác nhận checklist thị giác + chụp ảnh | Chờ người dùng trực tiếp xác nhận |
