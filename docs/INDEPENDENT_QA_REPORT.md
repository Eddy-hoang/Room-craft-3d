# RoomCraft 3D (`v1.0.0-rc1`) — Báo cáo Kiểm toán Chất lượng Độc lập & Thẩm định Thiết kế Thị giác (Independent QA & Visual Design Audit)

- **Sản phẩm**: RoomCraft 3D (`v1.0.0-rc1`)
- **Workspace**: `D:\CG`
- **Gói phát hành kiểm toán**: `dist/RoomCraft3D-v1.0.0-rc1-win64/RoomCraft3D.exe` (và cặp build `build/mingw-x64-debug`, `build/mingw-x64-release`)
- **Nền tảng kiểm thử**: Windows 11 x64 (`10.0.26200`), GPU tích hợp `Intel(R) UHD Graphics` (Driver `32.0.101.7088`, OpenGL 3.3.0 Core Profile), WinLibs MinGW-w64 x64 UCRT `g++ 16.2.0`, CMake 4.4.4, Ninja 1.13.2
- **Vai trò kiểm toán**: Independent QA Engineer · Senior Computer Graphics Engineer · Technical Artist · UI/UX Designer · Software Architect
- **Phạm vi tuân thủ**: Kiểm toán độc lập 100% — **không chỉnh sửa mã nguồn**, không đổi kiến trúc, không cài thêm dependency, không commit/tag/push/reset Git, không phát hành `v1.0.0`, không tạo ảnh chụp hoặc số liệu giả.

---

## 1. Executive Summary

### Kết luận tổng quan: **KHÔNG ĐẠT ĐIỀU KIỆN PHÁT HÀNH `v1.0.0` (NO-GO — Giữ ở `v1.0.0-rc1`)**

Về mặt **kỹ thuật phần mềm và kiến trúc**, RoomCraft 3D `v1.0.0-rc1` có nền tảng vững chắc: mã nguồn C++17 / OpenGL 3.3 Core sạch, tách biệt rõ ràng các thư viện lõi (`roomcraft_scene`, `roomcraft_engine`, `roomcraft_app`), toàn bộ **19/19 bộ kiểm thử CTest đều PASS độc lập trên cả Debug và Release**, ứng dụng portable khởi chạy sạch trên Windows x64, xử lý Unicode tiêu đề cửa sổ chính xác và hiệu năng Release rất dồi dào trên Intel UHD Graphics (`292–366 FPS` off-screen với Shadows 1024 + MSAA 4×).

Tuy nhiên, **cảm nhận không hài lòng của người dùng về chất lượng hình ảnh và thẩm mỹ là hoàn toàn có cơ sở kỹ thuật**. Báo cáo kiểm toán phát hiện **một khoảng cách rất lớn giữa "đạt kiểm thử tự động" và "chất lượng thẩm mỹ sản phẩm thiết kế nội thất 3D"**:

1. **Bóng đổ có lỗi đồ họa thật (Confirmed P0 Graphics Bugs)**: Shadow map hiện tại gây ra **shadow acne dạng vân gợn sóng Moiré đồng tâm trên cả hai bức tường Bắc và Tây**, cạnh bóng răng cưa rõ rệt và hiện tượng **peter-panning / thiếu bóng tiếp xúc (contact shadow)** ở chân tủ/bàn/ghế trong chế độ `Evening` và `Daylight`.
2. **Ánh sáng cháy sáng (Blown-out Overexposure) do thiếu Tone Mapping**: Preset `Warm Interior` có **`7,620` pixel gần bão hòa trắng (`8.42%` diện tích căn phòng, `max(R,G,B) ≥ 248`)** và `Gaming Workspace` có **`5,905` pixel (`5.95%`)**, tạo vệt sáng vàng trắng lóa gắt trên tường phía trên tủ.
3. **Khung hình 3D lãng phí ~75% diện tích**: Trong cả 3 phòng mẫu mặc định, toàn bộ căn phòng chỉ chiếm **`22.5% – 26.7%`** diện tích viewport 3D — gần **3/4 màn hình là khoảng trống tối phẳng `#171A20`**, khiến cảnh 3D trông như mô hình đồ chơi nhỏ lơ lửng giữa hư không thay vì một studio nội thất.
4. **Vật liệu "nhựa hóa" đồng loạt (Universal Plastic Specular)**: Toàn bộ **8/8 file `.mtl` xuất từ Blender** đều mang `Ks 0.500 0.500 0.500` kể cả vải sofa, chăn ga, thảm dệt, chụp đèn vải và đất trồng cây; kết hợp Blinn-Phong không bảo toàn năng lượng khiến bề mặt bị bóng nhựa giả tạo và thiếu tương phản chất liệu.
5. **Preset mặc định `Gaming Workspace` xuất xưởng kèm lỗi va chạm & vượt biên**: Ngay khi mở preset này, hệ thống báo **`Warnings: 1 overlap, 1 bounds`** (chậu cây `Indoor Plant` chọc ra ngoài tường Tây, `Desk Lamp` đè lên `Computer Setup`) và tô khung dây đỏ/cam lỗi trực tiếp lên cảnh mặc định.
6. **Không gian kiến trúc trống trải & chi tiết hiển thị đậm chất "CAD debug"**: Tường phẳng trơn cao 3m không có len chân tường (baseboard), không cửa sổ hay khung tranh; trục tọa độ RGB debug mặc định đâm xuyên qua giữa giường và thảm; lưới sàn bật sẵn gây rối; tên 4/6 card nội thất C++ bị cắt chữ (`Procedural Bo...`, `Procedural De...`, `Procedural Ce...`, `Procedural Cu...`).

**Điểm chất lượng thị giác tổng thể (Overall Visual Quality)**: **`5.1 / 10`** (chưa đạt chuẩn sản phẩm phát hành `v1.0.0`). Điều rất đáng mừng là GPU Intel UHD hiện mới chỉ dùng `~1.4–2.0 ms/frame` trong tổng ngân sách `16.67 ms` (60 Hz) — tức dự án **còn thừa tới ~12–13 ms GPU mỗi khung hình** để triển khai toàn bộ gói nâng cấp thị giác P0 + P1 mà không cần đổi OpenGL 3.3 hay thêm dependency mới.

---

## 2. Independent Automated Test Results

### 2.1. Kết quả tái chạy độc lập

| Cấu hình | Lệnh thực thi độc lập | Exit Code | Kết quả | Thời gian |
|---|---|---:|---|---:|
| **Debug** | `ctest --test-dir 'd:\CG\build\mingw-x64-debug' --output-on-failure` | `0` | **19/19 PASS** | `7.89 s` |
| **Release** | `ctest --test-dir 'd:\CG\build\mingw-x64-release' --output-on-failure` | `0` | **19/19 PASS** | `5.81 s` |
| **Portable GPU Capture** | `d:\CG\dist\RoomCraft3D-v1.0.0-rc1-win64\RoomCraft3D.exe --m9-capture-evidence d:\CG\docs\evidence\independent-qa` | `0` | **12/12 ảnh PASS · `OpenGL errors: NONE`** | `~3.8 s` |

Danh sách 19 bộ kiểm thử đều đạt (`100%`) trên cả Debug và Release:
`M1_CameraUnitTests`, `M1_SmokeTest`, `M2_GeometryUnitTests`, `M2_GpuSmokeTest`, `M3_AssetUnitTests`, `M3_AssetGpuSmokeTest`, `M4_SceneUnitTests`, `M4_RoomGpuSmokeTest`, `M5_InteractionUnitTests`, `M5_InteractionGpuSmokeTest`, `M6_UiUxUnitTests`, `M6_UiUxGpuSmokeTest`, `M7_LightingUnitTests`, `M7_LightingGpuSmokeTest`, `M8_CollisionUnitTests`, `M8_PersistenceUnitTests`, `M8_IntegrationSmokeTest`, `M9_ReliabilityUnitTests`, `M9_IntegrationSmokeTest`.

### 2.2. Phản biện chất lượng bộ kiểm thử tự động (Test Blind Spots)

Mặc dù **19/19 CTest đều PASS**, việc kiểm tra mã nguồn test cho thấy nhiều điểm mù nghiêm trọng khiến các lỗi thật lọt qua QA trước đó:

1. **Test bóng đổ quá sơ sài (`M9_IntegrationSmokeTest`)**: Bài test chỉ kiểm tra độ sáng trung bình toàn ảnh giảm nhẹ khi bật shadow (`AvgLuma`: `55.74 → 54.98`) và không có lỗi `glGetError()`. Bài test **hoàn toàn không kiểm tra** shadow acne trên tường, độ răng cưa cạnh bóng, peter-panning hay cháy sáng điểm nhấn.
2. **Preset mặc định không được chạy qua `CollisionSystem` trong unit test**: `M4_SceneUnitTests` xác minh 3 preset nạp thành công về mặt số lượng object, nhưng **không kiểm tra `CollisionSystem::validateScene()` bằng 0 lỗi**. Kết quả là `GamingWorkspace` lọt qua kiểm thử dù có sẵn `1 overlap + 1 bounds` ngay khi mở!
3. **Shader lighting GPU test bị tách khỏi đường ống render thực tế (`M7_LightingGpu`)**: Test M7 chạy với `uShadowEnabled = 0` và vật thể đơn lẻ, nên chưa bao giờ kiểm tra độ bão hòa/clipping (`rgb > 1.0`) của `Warm Interior` trên tường thật hoặc tương tác giữa `computeRoomCornerAO` với đồ nội thất.
4. **Thiếu kiểm thử tương tác GUI thật ở tầng OS**: Các smoke test M5/M6/M8/M9 gọi trực tiếp các phương thức C++ nội bộ (`applyPresetImmediateForAutomation`, `requestSave`, ...) thay vì bấm chuột thật, kéo gizmo qua FreeGLUT callback, mở hộp thoại Win32 `GetOpenFileNameW`/`GetSaveFileNameW` hay nhấn nút `X` cửa sổ thật.
5. **Thiếu kiểm tra đồng bộ kích thước Catalog vs OBJ thực tế**: `FurnitureCatalog` khai báo `blender-indoor-plant` là `0.58 × 1.15 × 0.58 m`, nhưng file OBJ thực tế có bounding box `0.89 × 1.08 × 0.87 m` (rộng hơn 53% so với khai báo catalog) — không có test nào bắt sai lệch này.

---

## 3. Desktop Functional Testing

### 3.1. Năng lực chạy thực tế & giới hạn môi trường kiểm toán

- **Khởi chạy bản đóng gói thật (`dist\RoomCraft3D-v1.0.0-rc1-win64\RoomCraft3D.exe`)**: Đã kiểm chứng thành công bằng tiến trình Win32 thực qua `ctypes`: cửa sổ tạo sạch kích thước vùng client `1280 × 720` trên `Intel(R) UHD Graphics`, tự động nạp font `Roboto-Medium.ttf` từ `assets/fonts/` ngay trong thư mục portable, tiêu đề cửa sổ đọc trực tiếp từ `GetWindowTextW` là **`RoomCraft 3D — Modern Bedroom`** (UTF-8 em-dash chính xác 100%, **không còn lỗi mojibake `â€”`**).
- **Giới hạn OS Desktop Capture (`BLOCKED`)**: Môi trường chạy dòng lệnh hiện tại là phiên Windows không có bề mặt desktop tương tác (non-interactive session), nên `PIL.ImageGrab.grab()` trả về `OSError: screen grab failed` và Win32 `PrintWindow(PW_RENDERFULLCONTENT)` trả về ảnh trắng rỗng (`MeanLuma = 255.0`, đã xóa ngay). Thư viện `pyautogui` / `pywinauto` không có sẵn và không được phép cài theo quy tắc kiểm toán.
- Do đó, các thao tác GUI cần chuột/bàn phím/hộp thoại Win32 sống được ghi nhận trung thực là **`BLOCKED / NOT VERIFIED (OS Desktop)`** kèm hướng dẫn tự kiểm nhanh cho người dùng, **tuyệt đối không đánh dấu PASS giả**.

### 3.2. Ma trận kiểm thử chức năng ứng dụng Desktop (21 hạng mục)

| # | Kịch bản kiểm thử | Kết quả kỳ vọng | Kết quả thực tế | Trạng thái | Bằng chứng |
|---|---|---|---|---|---|
| 1 | Khởi chạy `dist/.../RoomCraft3D.exe` | Mở cửa sổ chính sạch, không lỗi DLL/asset | Cửa sổ Win32 `1280×720` mở ổn định, nạp font & asset từ `dist/.../assets`, exit sạch | **PASS (Native Launch)** | Win32 probe log |
| 2 | Hiển thị tiếng Anh / Unicode title bar | Hiển thị `RoomCraft 3D — Modern Bedroom` không vỡ ký tự | `GetWindowTextW` = `RoomCraft 3D — Modern Bedroom` (`em-dash OK`) | **PASS (Native)** | Win32 probe log |
| 3 | Chuyển Preset `Modern Bedroom` | Cảnh tải đủ 7 vật thể, 0 lỗi va chạm/biên | Đủ 7 vật thể, `Warnings: 0`, không lỗi GL | **PASS (GPU Capture)** | `01_modern_bedroom.png` |
| 4 | Chuyển Preset `Gaming Workspace` | Cảnh tải sạch, 0 lỗi va chạm/biên | **LỖI**: Báo ngay `Warnings: 1 overlap, 1 bounds` + khung đỏ/cam | **FAIL (BUG-04)** | `02_gaming_workspace.png` |
| 5 | Chuyển Preset `Minimalist Living Room` | Cảnh tải đủ 7 vật thể, bố cục hài hòa | Đủ 7 vật thể, 0 lỗi va chạm nhưng phòng quá thưa (`22.5%` viewport) | **PASS (Func) / POOR (Visual)** | `03_minimalist_living_room.png` |
| 6 | Orbit Camera bằng chuột | Xoay mượt quanh target, không giật bóng | Tự động M5/M9 PASS; tương tác chuột sống chưa kiểm chứng | **NOT VERIFIED (Manual)** | — |
| 7 | Pan Camera bằng chuột | Tịnh tiến camera mượt trong viewport | Tự động M5 PASS; tương tác chuột sống chưa kiểm chứng | **NOT VERIFIED (Manual)** | — |
| 8 | Zoom Camera bằng cuộn chuột | Zoom mượt cả Perspective & Orthographic | Tự động M5 PASS; tương tác chuột sống chưa kiểm chứng | **NOT VERIFIED (Manual)** | — |
| 9 | Chọn vật thể 3D (Picking) | Click chính xác (AABB + Ray–Triangle + Wall Occlusion) | Tự động M5 PASS; click chuột sống trên UI chưa kiểm chứng | **NOT VERIFIED (Manual)** | — |
| 10 | Move / Rotate / Scale bằng Gizmo & phím `G/R/S` | Kéo mượt, đúng trục X/Y/Z, cập nhật Inspector | Tự động M5/M9 PASS; kéo chuột sống chưa kiểm chứng | **NOT VERIFIED (Manual)** | — |
| 11 | Grid Snapping | Khớp đúng bước lưới `0.50 m` / `15°` | Tự động M5 PASS; kéo chuột sống chưa kiểm chứng | **NOT VERIFIED (Manual)** | — |
| 12 | Thêm vật thể từ Furniture Library | Click `+ Add Furniture` thêm vật thể giữa phòng, cảnh báo nếu đè | Tự động M4/M6 PASS; thao tác click sống chưa kiểm chứng | **NOT VERIFIED (Manual)** | — |
| 13 | Duplicate / Delete vật thể | Nhân bản có offset, xóa có xác nhận/Undo | Tự động M6/M8 PASS; thao tác UI sống chưa kiểm chứng | **NOT VERIFIED (Manual)** | — |
| 14 | Undo / Redo (`Ctrl+Z` / `Ctrl+Y`) | Hoàn tác/làm lại chính xác không mất trạng thái | Tự động M8 PASS; phím tắt sống chưa kiểm chứng | **NOT VERIFIED (Manual)** | — |
| 15 | Đổi kích thước phòng & chế độ tường (Cutaway/All/Floor) | Tường/sàn tái tạo đúng, va chạm cập nhật | Tự động M4/M8 PASS; thao tác UI sống chưa kiểm chứng | **NOT VERIFIED (Manual)** | — |
| 16 | Chuyển 4 Lighting Preset | Ánh sáng thay đổi rõ rệt, không cháy sáng | Đổi được 4 preset nhưng `Warm Interior` cháy sáng `8.42%` tường & làm bật dấu `*` dirty | **FAIL (Visual / BUG-02)** | `04`–`07_lighting_*.png` |
| 17 | Bật/tắt Shadow Mapping & đổi `512/1024/2048` | Bóng sạch, sắc nét, không acne | Bật/tắt chạy nhưng xuất hiện **wall shadow acne** và **răng cưa/peter-panning**; `512/2048` chưa chụp được ảnh UI | **FAIL (Visual / BUG-01,03)** | `analysis_shadow_wall_comparison.png` |
| 18 | Kiểm tra cảnh báo Collision & Boundary | Tô màu cam (overlap) / đỏ (bounds) và ghi status bar | Hiển thị đúng khung dây & thông điệp trên status bar | **PASS (GPU Capture)** | `10_collision_warning.png`, `11_room_boundary_warning.png` |
| 19 | Save / Save As / Open `.roomcraft` (Win32 File Dialog) | Mở hộp thoại Windows native, lưu/nạp toàn vẹn | Unit/Integration C++ PASS; hộp thoại Win32 thực tế chưa kiểm chứng | **BLOCKED / NOT VERIFIED** | — |
| 20 | Recent Projects & Preferences persistence | Nhớ danh sách file gần đây, UI Scale, layout qua lần chạy sau | `M9_ReliabilityUnitTests` PASS; xác minh 2 phiên mở tay chưa kiểm chứng | **NOT VERIFIED (Manual)** | — |
| 21 | Close app khi dirty (`WM_CLOSE` prompt) | Hiện modal `Save / Discard / Cancel` khi bấm nút `X` | Hook `WM_CLOSE` đã kiểm chứng qua code + unit; bấm nút `X` thật chưa kiểm chứng | **BLOCKED / NOT VERIFIED** | — |

### 3.3. Hướng dẫn tự kiểm thủ công ngắn gọn (5 phút cho người dùng)
1. Mở `dist\RoomCraft3D-v1.0.0-rc1-win64\RoomCraft3D.exe`.
2. Kiểm tra Orbit (kéo chuột phải/giữa), Pan (`Shift + chuột giữa`), Zoom (cuộn chuột), chuyển `3D / Top / Iso`.
3. Chuyển `Room Presets → Gaming Workspace` để tự xác nhận lỗi `Warnings: 1 overlap, 1 bounds` ở góc chậu cây và bàn máy tính.
4. Mở tab `Room Settings`, chuyển qua lại `Shadows: Off / 512 / 1024 / 2048` và 4 `Lighting Presets` khi đang xoay camera để quan sát acne trên tường và hiện tượng lấp lánh cạnh bóng (shimmering) nếu có.
5. Thử `File → Save As` lưu 1 file `.roomcraft` có ký tự tiếng Việt, sửa vị trí giường, nhấn `X` đóng cửa sổ để xác nhận hộp thoại `Save / Discard / Cancel`, mở lại app và kiểm tra `File → Open Recent`.

---

## 4. Visual Quality Assessment

### Tại sao người dùng nhận xét *"nó không đẹp lên mấy"* dù M7–M9 đã thêm Blinn-Phong, Gamma, MSAA và Shadow Mapping?

Từ góc nhìn **Technical Artist & Computer Graphics Engineer**, có **6 nguyên nhân thị giác cộng hưởng** khiến ảnh chụp hiện tại vẫn tạo cảm giác *"một công cụ học thuật / CAD debug"* thay vì một ứng dụng thiết kế nội thất hiện đại (như Planner 5D, Sweet Home 3D bản nâng cấp hay HomeByMe):

1. **Tỷ lệ chiếm khung hình quá nhỏ (`22.5% – 26.7%`) trên nền đen chết phẳng**:
   - Phép đo pixel chính xác trên vùng viewport `VBOX = (280, 105, 956, 655)` cho thấy:
     - `Modern Bedroom`: **`24.3% – 26.6%`** diện tích viewport.
     - `Gaming Workspace`: **`26.7%`** diện tích viewport.
     - `Minimalist Living Room`: **`22.5%`** diện tích viewport.
   - Hơn **73% – 77% viewport là màu tối phẳng đơn điệu `#171A20`**, không có gradient studio nhẹ. Căn phòng bị đẩy quá xa (`Camera::fitBoundingBox` tính khoảng cách theo ngoại biên cả tường đã ẩn + `paddingFactor = 1.22` + cộng thêm `projHalfZ` tại `src/graphics/Camera.cpp:159-161`). Mắt người xem lập tức thấy sự trống rỗng trước khi kịp chú ý chi tiết đồ nội thất.

2. **Đường nét debug đè lên không gian sống**:
   - Trục tọa độ `X/Y/Z` (đỏ/xanh lá/xanh dương) mặc định đang **BẬT** và đâm thẳng xuyên qua tâm giường/thảm (`01_modern_bedroom.png`, `03_minimalist_living_room.png`).
   - Lưới sàn `Grid: On` mặc định cắt nhỏ mặt gỗ sồi thành các ô vuông kỹ thuật, triệt tiêu cảm giác ấm cúng của sàn gỗ.

3. **Ánh sáng không có điểm nghỉ tương phản và thiếu Tone Mapping**:
   - Ở `Daylight` và `Neutral Studio`, mức `ambient = 0.42 – 0.50` quá cao nên mọi bề mặt đều sáng đều, thiếu độ sâu khối (flat lighting).
   - Ngược lại, ở `Warm Interior`, không có filmic tone mapping nên nguồn đèn điểm tạo một vùng **cháy trắng vàng bệt trên tường** (`8.42%` diện tích phòng bị kẹp ngưỡng gần 255).

4. **Bóng đổ có lỗi kỹ thuật dễ thấy bằng mắt thường**:
   - Tường Bắc và Tây bị **vân sọc Moiré đồng tâm (shadow acne)** do thiếu front-face culling / normal-offset bias khi vẽ shadow pass.
   - Cạnh bóng bị **răng cưa bậc thang** (`GL_NEAREST` trên shadow map 1024) và đồ nội thất thiếu **bóng tiếp xúc mềm dưới đáy (contact shadow / AO grounding)**, khiến giường, ghế, bàn và chậu cây có cảm giác "lơ lửng nhẹ" trên sàn.

5. **Chất liệu đồng nhất "nhựa mờ" (No PBR / Uniform `Ks = 0.50`)**:
   - Gỗ, vải nỉ sofa, linen chăn gối, thảm dệt và đất trồng cây đều dùng cùng một phản xạ `Ks = 0.50` từ mặc định Blender MTL — không có bản đồ nhám (roughness), không có normal/bump map, và không phân biệt được vật liệu mờ (matte) với kim loại/gốm.

6. **Kiến trúc căn phòng quá sơ khai ("hộp 2 tường trơn")**:
   - Hai bức tường phẳng hoàn toàn từ sàn `y = 0` lên `y = 3.0 m`, không có **len chân tường (baseboard)**, không có **cửa sổ (window frame + kính phản xạ nhẹ)** hay **tranh treo tường** như hình mockup mà người dùng từng gửi làm chuẩn tham chiếu ở M6.
   - Các mẫu Procedural C++ (bàn học, kệ sách) là hình hộp vuông sắc cạnh 90° đơn sắc, đặt cạnh mẫu Blender có texture vải/gỗ khiến tổng thể bị lệch phong cách rõ rệt.

---

## 5. Lighting Audit

Kiểm toán trực tiếp trên `include/roomcraft/graphics/Lighting.hpp:8-51`, `assets/shaders/mesh.frag:37-122` và 4 ảnh chụp thực tế (`04_lighting_daylight.png` – `07_lighting_evening.png`):

| Preset | Tham số thực tế (`Lighting.hpp`) | Mean Luma vùng phòng | Pixel cháy sáng (`max≥248`) | Đánh giá thị giác |
|---|---|---|---|---|
| **Daylight** *(Mặc định)* | `ambient=0.42`, `sun=0.72` `(1.0,0.95,0.86)`, `dir=(-0.6,-1.0,-0.45)`, `point=Off` | `176.2` | `0` (`0.00%`) | Sáng sạch nhưng **hơi phẳng**; độ tương phản vùng sáng/tối thấp; có vân shadow acne nhẹ trên tường Tây & Bắc. |
| **Warm Interior** | `ambient=0.32`, `sun=0.36`, `point=On`, `pos=(-1.0,2.3,0.7)`, `intensity=2.0`, `color=(1.0,0.68,0.42)` | `177.9` | **`7,620` (`8.42%`)** | **LỖI NGHIÊM TRỌNG (BUG-02)**: Quầng sáng điểm gắt làm cháy bệt tường phía trên tủ quần áo thành mảng vàng trắng mất chi tiết; không tự nhiên vì vị trí đèn ở `(-1, 2.3, 0.7)` không gắn với bất kỳ nguồn sáng vật lý nào trong phòng. |
| **Neutral Studio** | `ambient=0.50`, `sun=0.62` `(1.0,1.0,1.0)`, `point=Off` | `179.9` | `0` (`0.00%`) | Trắng trung tính nhưng **`ambient=0.50` quá cao**, xóa gần hết chiều sâu khối và làm bóng đổ mờ nhạt (shadow contrast rất yếu). |
| **Evening** | `ambient=0.17`, `sun=0.35` `(0.48,0.62,1.0)`, `dir=(-1.0,-0.28,0.4)`, `point=On (1.3)` | `135.4` | `0` (`0.00%`) | Có tâm trạng tốt nhất trong 4 preset (tương phản ấm–lạnh rõ), nhưng góc mặt trời rất thấp `(-1.0, -0.28, 0.4)` phơi bày nặng nề **bóng đổ răng cưa & peter-panning** của tủ lên tường Tây. |

### Phân tích thuật toán chiếu sáng (`mesh.frag`)
- **Thiếu Tone Mapping (`mesh.frag:122`)**: Dòng `rgb = displayColor(max(rgb, vec3(0)));` chuyển thẳng giá trị tuyến tính không giới hạn sang sRGB. Bất kỳ pixel nào có tổng radiance `> 1.0` đều bị cắt cứng (hard clipping) và lệch sắc độ (hue shift sang vàng/trắng).
- **Blinn-Phong không bảo toàn năng lượng (`mesh.frag:37-42`)**: Công thức `albedo * diffuse + uSpecular * highlight` cộng trực tiếp specular lên diffuse mà không có hệ số chuẩn hóa `(shininess + 8) / (8π)` và không xét Fresnel góc nghiêng (`Schlick Fresnel`).
- **Suy giảm Point Light yếu (`attenuation = (1.0, 0.18, 0.12)`)**: Ở khoảng cách `d = 1.2–1.6 m` từ đèn tới tường Tây, mẫu số chỉ khoảng `1.4–1.6`, khiến `pointIntensity = 2.0` đổ vào tường trắng `(0.90, 0.89, 0.86)` tạo giá trị tuyến tính `> 1.35` → cháy trắng ngay lập tức.
- **Lỗi phạm vi `computeRoomCornerAO` (`mesh.frag:65-84`)**:
  1. Áp dụng cho **tất cả hình học trong cảnh** (kể cả mặt bàn, ghế, tủ đặt gần tường/sàn) vì shader chỉ kiểm tra `uRoomHalfSize.z > 0.0` chứ không phân biệt tường/sàn phòng với đồ nội thất.
  2. Tính khoảng cách `dEast`, `dSouth` kể cả khi tường Đông và tường Nam **đang bị ẩn ở chế độ CutawayView**, gây tối giả tạo dọc hai mép sàn trống phía trước!

---

## 6. Shadow Mapping Audit

Kiểm toán trực tiếp trên `src/graphics/ShadowMap.cpp`, `src/graphics/SceneRenderer.cpp:62-115`, `assets/shaders/mesh.frag:45-62` và 2 ảnh phân tích phóng đại (`analysis_shadow_wall_comparison.png`, `analysis_wall_acne_zoom_2x.png`):

### 6.1. Các lỗi bóng đổ đã xác nhận bằng bằng chứng pixel
1. **Wall Shadow Acne (Vân Moiré đồng tâm trên tường Bắc & Tây — BUG-01)**:
   - Khi lấy hiệu đại số khuếch đại 4× giữa `12_shadows_off_comparison.png` và `04_lighting_daylight.png` (`analysis_shadow_wall_comparison.png`, góc dưới trái), hiện rõ **nhiều dải vân gợn sóng Moiré đồng tâm chạy khắp mặt trong tường Tây và tường Bắc**; phóng đại 2× (`analysis_wall_acne_zoom_2x.png`, góc trên phải) thấy các vệt lốm đốm chéo làm bẩn bề mặt tường.
   - **Nguyên nhân gốc**:
     - `SceneRenderer::renderShadowPass()` (`src/graphics/SceneRenderer.cpp:82-95`) vẽ 4 tường phòng **không bật `glEnable(GL_CULL_FACE); glCullFace(GL_FRONT);`** và **không có `glPolygonOffset`**.
     - `mesh.frag:51-52` chỉ dùng depth bias đơn giản (`bias = max(0.0022 * (1 - ndotl) * 2.2, 0.00099)`) mà **không có Normal-Offset Bias** (`worldPos + normal * normalBias`) trước khi chiếu sang light space. Với góc chiếu xiên trên tường, độ phân giải depth texel lệch theo độ dốc tạo ra tự đổ bóng (self-shadowing) theo chu kỳ.
2. **Cạnh bóng răng cưa & Peter-Panning / thiếu Contact Grounding (BUG-03)**:
   - Trong `analysis_wall_acne_zoom_2x.png` (góc dưới phải — `Evening`), bóng của tủ quần áo hắt lên tường Tây bị **răng cưa bậc thang rõ rệt** và xuất hiện khoảng hở tách bóng dưới chân tủ.
   - **Nguyên nhân gốc**:
     - `ShadowMap.cpp:36-37` đặt `GL_TEXTURE_MIN_FILTER = GL_NEAREST`, `GL_TEXTURE_MAG_FILTER = GL_NEAREST` và không dùng `sampler2DShadow` + `GL_COMPARE_REF_TO_TEXTURE` (hardware bilinear PCF). Bộ lọc PCF 3×3 thủ công trong `mesh.frag:55-61` chỉ lấy 9 mẫu nhị phân (0 hoặc 1) trên lưới rời rạc nên vẫn lộ cạnh bậc thang.
     - Vùng chiếu `ShadowMap::computeDirectionalLightSpaceMatrix` bao trùm toàn bộ **ngoại biên căn phòng** (cả chiều cao tường 3m + bề dày tường), làm lãng phí mật độ texel trên sàn và đồ nội thất.
     - Chân bàn/ghế mỏng (`4–5 cm`) nhỏ hơn `1.5–2` shadow texel cộng với depth bias nên không tạo được vệt bóng tiếp xúc xuống sàn.
3. **Chưa kiểm chứng trực quan độ phân giải `512` và `2048`**:
   - CLI capture hiện tại chỉ xuất cấu hình mặc định `1024` và `Off`. Ảnh chụp `512` và `2048` ở trạng thái **`NOT CAPTURED / BLOCKED`** (xem Mục 14).

---

## 7. Material Audit

Kiểm toán trực tiếp trên **toàn bộ 8 file `.mtl`** (`assets/models/*/*.mtl`), `src/graphics/Texture.cpp`, `src/graphics/SceneRenderer.cpp` và `assets/shaders/mesh.frag`:

### 7.1. Phát hiện cốt lõi: Toàn bộ 8/8 tài sản Blender đều bị "nhựa hóa" (`Ks = 0.50`)
Kiểm tra trực tiếp toàn bộ 8 file `.mtl` cho thấy lỗi hệ thống từ script xuất Blender:

| File `.mtl` | Vật liệu đại diện | `Kd` (Diffuse) | `Ks` (Specular) | `Ns` (Shininess) | Texture `map_Kd` | Nhận xét thẩm mỹ |
|---|---|---|---|---|---|---|
| `modern_sofa.mtl` | `SofaFabricBeige`, `SofaCushionSage` | Vải be / xám xanh | **`0.500 0.500 0.500`** | `225`–`250` | Có (`512²`) | **Sai vật lý**: Vải dệt không thể có specular 0.5 và shininess 250 như nhựa cứng! |
| `modern_bed.mtl` | `BedDuvetLinen`, `BedPillowWhite`, `BedWoodFrame` | Vải linen / gỗ | **`0.500 0.500 0.500`** | `225`–`250` | Có | Chăn ga gối bị ánh lóa bóng khi xoay góc đèn. |
| `minimalist_carpet.mtl` | `ScandiRugWeave` | Thảm dệt | **`0.500 0.500 0.500`** | `225` | Có | Thảm bị bóng dầu ở góc xiên, mất độ xù mềm của sợi len. |
| `wardrobe.mtl` | `WardrobeMatteWhite`, `WardrobeOak` | Trắng mờ / gỗ sồi | **`0.500 0.500 0.500`** | `225` | Có | Tên là `MatteWhite` nhưng `Ks = 0.50`, cộng hưởng làm cháy sáng trên tường/tủ. |
| `indoor_plant.mtl` | `PlantSoilDark`, `PlantPotCeramic`, `PlantLeaf` | Đất / gốm / lá | **`0.500 0.500 0.500`** | `225` | Có | **Đất trồng cây** có cùng độ bóng gương với chậu gốm! |
| `floor_lamp.mtl` | `FloorLampLinenShade`, `FloorLampWood` | Chụp vải / gỗ | **`0.500 0.500 0.500`** | `225` | Có | Chụp đèn vải phản xạ như nhựa; không có phát sáng nhẹ (emissive) khi bật đèn buổi tối. |
| `computer_setup.mtl` | `MonitorScreen`, `DeskAluminum` | Màn hình / kim loại | **`0.500 0.500 0.500`** | `225` | Có | Thiếu phân tách độ bóng kính màn hình vs vỏ nhựa nhám. |
| `gaming_chair.mtl` | `ChairLeatherDark`, `ChairAccentBlue` | Da PU / nhựa | **`0.500 0.500 0.500`** | `225` | Có | Tạm chấp nhận nhất vì da PU có bóng, nhưng vẫn đồng đều quá mức. |

**Nguyên nhân gốc**: Khi Blender xuất OBJ/MTL từ `Principled BSDF`, tham số mặc định `Specular IOR Level = 0.5` được ghi thẳng thành `Ks 0.500 0.500 0.500` và `Roughness = 0.5` thành `Ns 225.0` bất kể bản chất vật liệu.

### 7.2. Đánh giá theo từng nhóm vật liệu
- **Gỗ (Sàn & Nội thất)**: Sàn gỗ procedural (`mesh.frag:90-106`) có vân sồi theo tọa độ mét không giãn khi đổi kích thước phòng (điểm cộng), nhưng mạch ghép `seamZ` độ rộng `0.008 m` tạo **sọc ngang/moiré nhẹ ở khoảng cách xa** (`analysis_shadow_wall_comparison.png`); bàn/kệ procedural C++ chỉ là màu phẳng không vân gỗ.
- **Vải (Sofa, Giường, Thảm, Chụp đèn)**: Có texture diffuse nhưng bị phá hỏng bởi `Ks = 0.50` và thiếu hiệu ứng sheen/wrap diffuse mềm của vải.
- **Kim loại & Gốm**: Không phân biệt được kim loại (tay nắm tủ, khung màn hình, trục đèn) với nhựa vì Blinn-Phong hiện tại không có Fresnel và không tint specular theo albedo kim loại (metallic workflow).
- **Lọc Texture (`Texture.cpp:171-180`)**: Đã upload đúng chuẩn `GL_SRGB8` / `GL_SRGB8_ALPHA8` (không bị double-gamma) và bật mipmap trilinear, nhưng **chưa bật Anisotropic Filtering** (`GL_TEXTURE_MAX_ANISOTROPY_EXT`), khiến texture thảm và gỗ hơi mờ ở góc nhìn nghiêng.

---

## 8. Furniture Modeling Audit

Kiểm toán toàn bộ **14 tài sản nội thất** (6 Procedural C++ tại `src/geometry/FurnitureGenerator.cpp` + 8 Blender OBJ tại `assets/models/`):

| # | Asset ID | Nguồn | Kích thước khai báo (Catalog) | Chất lượng hình khối & Silhouette | Cạnh sắc / Bevel | Texture / Vật liệu | Mức nhất quán |
|---|---|---|---|---|---|---|---|
| 1 | `procedural-desk` | C++ (M2) | `1.40 × 0.75 × 0.70 m` | Rất đơn giản (5 hộp ghép: mặt bàn + 4 chân vuông) | **Cạnh 90° sắc hoàn toàn**, không bevel | Màu phẳng đơn sắc, không vân gỗ | **Thấp** — lộ rõ khối hộp cạnh ghế Blender |
| 2 | `procedural-chair` | C++ (M2) | `0.46 × 0.89 × 0.46 m` | Hình khối cơ bản (chân + mặt ngồi + tựa phẳng) | Cạnh 90° sắc | Màu phẳng | **Thấp** |
| 3 | `procedural-bookshelf` | C++ (M2) | `0.90 × 1.80 × 0.32 m` | Khối hộp có ngăn rỗng, **trống trơn không có sách/đồ trang trí** | Cạnh 90° sắc | Màu nâu phẳng đồng nhất | **Thấp** — trông như hộp các-tông khi đặt cạnh tường |
| 4 | `procedural-desk-lamp` | C++ (M2) | `0.27 × 0.51 × 0.27 m` | Hình tròn xoay mượt (`RevolutionSurface`), silhouette khá tốt | Bo tròn tốt | Màu phẳng, không emissive | **Trung bình – Khá** |
| 5 | `procedural-vase` | C++ (M2) | `0.20 × 0.40 × 0.20 m` | Cong Bézier tròn xoay mượt, thẩm mỹ tốt | Mượt | Màu gốm đơn sắc | **Khá** |
| 6 | `procedural-curved-chair` | C++ (M2) | `0.50 × 0.91 × 0.58 m` | Mặt cong Bézier rõ giá trị học thuật CG | Mặt cong mềm, chân sắc | Màu đơn sắc | **Trung bình – Khá** |
| 7 | `blender-modern-sofa` | Blender (M3) | `2.10 × 0.85 × 0.92 m` | Silhouette Scandinavian tốt, có đệm + gối nhấn | Bo góc nhẹ | Có texture, nhưng `Ks=0.50` bóng nhựa | **Khá** |
| 8 | `blender-modern-bed` | Blender (M3) | `1.75 × 1.05 × 2.15 m` | Hình khối tốt: đầu giường, chăn gấp, 2 gối rõ ràng | Bo góc vừa | Có texture, nhưng `Ks=0.50` bóng nhựa | **Khá – Tốt** |
| 9 | `blender-gaming-chair` | Blender (M3) | `0.68 × 1.26 × 0.68 m` | Chi tiết tốt (chân sao 5 nhánh, tay vịn, tựa lưng ôm) | Tốt | Có texture | **Tốt** |
| 10 | `blender-computer-setup` | Blender (M3) | `0.92 × 0.48 × 0.48 m` | Rõ màn hình, bàn phím, chuột, thùng PC | Khá | Có texture | **Khá – Tốt** |
| 11 | `blender-indoor-plant` | Blender (M3) | Catalog `0.58×1.15×0.58 m` (**OBJ thật `0.89×1.08×0.87 m`**) | Tán lá tự nhiên, tạo điểm xanh tốt | Tốt | Có texture; đất bị `Ks=0.50` | **Khá (nhưng lệch kích thước catalog +53%!)** |
| 12 | `blender-floor-lamp` | Blender (M3) | `0.48 × 1.62 × 0.48 m` | Kiểu tripod thanh mảnh, silhouette hiện đại | Khá | Chụp vải bị `Ks=0.50` | **Khá** |
| 13 | `blender-wardrobe` | Blender (M3) | `1.40 × 2.05 × 0.60 m` | 3 cánh phối màu gỗ + trắng + tay nắm mảnh, rất gọn | Khá | Bị phản sáng mạnh ở `Warm Interior` | **Khá – Tốt** |
| 14 | `blender-carpet` | Blender (M3) | `2.20 × 0.02 × 1.50 m` | Tấm chữ nhật mỏng, đường viền hơi cứng | Cạnh thẳng | Texture dệt rõ nhưng bị `Ks=0.50` | **Trung bình – Khá** |

### Nhận xét tổng thể về Modeling
- **Độ chênh lệch phong cách giữa Procedural và Blender**: Khi `procedural-desk` (Study Desk / Main Gaming Desk / Coffee Table) và `procedural-bookshelf` xuất hiện trong cả 3 phòng mẫu ngay cạnh `Modern Bed`, `Gaming Chair`, `Modern Sofa`, sự thiếu bevel + thiếu chi tiết phụ (ngăn kéo, viền bàn, vài quyển sách đơn giản trên kệ) + màu phẳng không vân làm giảm mạnh đẳng cấp tổng thể của căn phòng.
- **Lệch kích thước nghiêm trọng (`BUG-06`)**: `blender-indoor-plant` có kích thước thực trong OBJ (`0.89 × 1.08 × 0.87 m`) lớn hơn đáng kể so với `defaultDimensions` trong `FurnitureCatalog.cpp:200` (`0.58 × 1.15 × 0.58 m`), trực tiếp góp phần gây ra lỗi vượt tường trong `Gaming Workspace`.

---

## 9. Room Composition Audit

Đánh giá kiến trúc & bố cục nội thất trên 3 phòng mẫu thực tế (`01_modern_bedroom.png`, `02_gaming_workspace.png`, `03_minimalist_living_room.png`):

1. **`Modern Bedroom` (`5.0 × 6.0 × 3.0 m`, 7 vật thể — `01_modern_bedroom.png`)**:
   - **Điểm đạt**: Phân khu hợp lý (khu ngủ + khu học tập + tủ quần áo + cây xanh), không va chạm (`0 warnings`), tỷ lệ người–nội thất chuẩn mét.
   - **Điểm yếu**: Khoảng trống phía Đông-Nam và trung tâm còn loãng; tủ quần áo đứng trơ trên tường Tây dài 6 m; **thiếu hoàn toàn các chi tiết kiến trúc làm phòng "thật"** như len chân tường (baseboard), cửa sổ trên tường Bắc/Tây, tranh treo tường hoặc tab đầu giường.

2. **`Gaming Workspace` (`4.0 × 5.0 × 3.0 m`, 7 vật thể — `02_gaming_workspace.png`)**:
   - **Lỗi xuất xưởng (BUG-04)**: Ngay khi vừa chọn preset, ứng dụng hiện **`Warnings: 1 overlap, 1 bounds`**:
     - `Indoor Plant` đặt tại `(-1.45, 0, -1.85)` nhưng bán kính lá thực tế là `~0.45 m` (do OBJ rộng `0.89 m`) → `x_min ≈ -1.90` đến `-2.05 m`, đụng/chọc ra ngoài tường Tây (`x = -2.0 m`, khung dây đỏ báo lỗi biên).
     - `Desk Lamp` tại `(0.52, 0.75, -2.10)` (bán kính `0.135 m`, `x_min = 0.385 m`) đè vào `Computer Setup` tại `(-0.10, 0.75, -2.02)` (rộng `0.92 m`, `x_max = 0.36 m` + sai số OBB/vỏ thùng PC) → khung dây cam báo va chạm!
   - **Thẩm mỹ**: Cháy sáng tường nặng (kế thừa đèn điểm) và kệ sách trống trơn trên tường Tây.

3. **`Minimalist Living Room` (`6.0 × 7.0 × 3.0 m = 42 m²`, 7 vật thể — `03_minimalist_living_room.png`)**:
   - **Thẩm mỹ yếu nhất trong 3 preset**: Diện tích `42 m²` quá rộng cho chỉ 7 vật thể kích thước trung bình; nửa phía Nam của căn phòng (`z ∈ [0.5, 3.5]`) gần như là **sàn trống hoàn toàn**; bàn trà dùng lại `procedural-desk` thu nhỏ tỷ lệ `(0.85, 0.60, 0.80)` trông cứng nhắc; toàn phòng chỉ chiếm **`22.5%`** diện tích viewport.

---

## 10. UI/UX Audit

Đánh giá khách quan giao diện Dear ImGui hiện tại (tham chiếu ảnh `01_modern_bedroom.png`, `06_lighting_neutral_studio.png`, `09_asset_inspector.png` và so với `docs/DESIGN_SYSTEM.md`):

### Điểm làm tốt (KHÔNG cần thiết kế lại toàn bộ UI)
- Bố cục 3 cột chuyên nghiệp (Furniture Library bên trái · 3D Viewport ở giữa · Scene Hierarchy & Properties/Room Settings bên phải) sạch sẽ, rõ ràng, đã loại bỏ triệt để các tab phát triển `M1–M4` ra khỏi giao diện chính (chuyển vào menu `Tools`).
- Bảng màu tối Slate Dark (`#181A20` / `#22252E` / `#4F7DF3`) hiện đại, độ tương phản chữ dễ đọc, dùng font `Roboto-Medium` sắc nét, thanh công cụ và thanh trạng thái sắp xếp mạch lạc.
- Thumbnail 3D trong Furniture Library có ảnh thật từ FBO với nhãn nguồn `[C++]` / `[OBJ]` gọn gàng.

### Các lỗi hoàn thiện cục bộ cần sửa (Localized Polish Fixes)
1. **Cắt cụt tên 4/6 thẻ nội thất Procedural (`01_modern_bedroom.png`)**:
   - Trong cột `FURNITURE LIBRARY`, tên bị cắt thành `Procedural Bo...`, `Procedural De...`, `Procedural Ce...`, `Procedural Cu...` vì `FurnitureCatalog.cpp` gắn thừa tiền tố `"Procedural "` trong khi mỗi thẻ **đã có badge `[C++]` góc trên phải**. Rút gọn tên hiển thị thành `Desk`, `Chair`, `Bookshelf`, `Desk Lamp`, `Ceramic Vase`, `Curved Chair` sẽ hết cắt chữ ngay lập tức.
2. **Khu vực `SCENE HIERARCHY` quá chật (`m_hierarchyRatio = 0.36`)**:
   - Dù cảnh chỉ có 7 đồ vật, khung Hierarchy chỉ hiển thị được 4–5 dòng và xuất hiện thanh cuộn, trong khi phần `PROPERTIES` ngay phía dưới gần như trống rỗng khi chưa chọn vật thể (`01_modern_bedroom.png`).
3. **Trạng thái mặc định trong Viewport làm xấu trải nghiệm mở app lần đầu**:
   - `showAxes = true` (trục RGB đâm giữa giường) và `Grid: On` (lưới đè lên sàn gỗ) đều bật mặc định. Nên đặt mặc định `showAxes = false` (chỉ bật trong menu View hoặc khi vào Geometry Lab) và `Grid: Off` mặc định (tự động hiện nhẹ khi bật `Snap` hoặc kéo vật thể).
4. **Đổi Lighting Preset để xem trước lại đánh dấu dự án bị sửa (`*` dirty)**:
   - Quan sát thanh tiêu đề góc phải của `06_lighting_neutral_studio.png` hiển thị `Modern Bedroom *` chỉ sau khi đổi lighting preset. Nếu người dùng chỉ bấm xem thử các preset ánh sáng khi chưa sửa nội thất, việc báo dirty ngay có thể gây hộp thoại hỏi lưu gây phiền khi đóng ứng dụng (cần cân nhắc kỹ hành vi mong muốn).
5. **HUD overlay ở góc viewport (`Move (G) | XZ Plane`)**: Gọn nhưng nằm khá sát mép dưới trái và nên hiển thị thêm gợi ý tương tác ngắn gọn khi chưa có object được chọn.

---

## 11. Performance Assessment

Đối chiếu số liệu benchmark Release & Debug (`docs/PERFORMANCE_REPORT.md`) trên GPU tích hợp `Intel(R) UHD Graphics` (`1280 × 720` off-screen FBO):

| Kịch bản (Release `-O3`) | Draw Calls | Tam giác | CPU Submit | GPU Time | Wall Avg (`p95`) | FPS (off-screen) | Nhận xét |
|---|---:|---:|---:|---:|---:|---:|---|
| 30 obj · MSAA Off · Shadows Off | 333 | 17,960 | `0.24 ms` | `0.84 ms` | `2.12 ms` (`4.10`) | `471.9` | Rất nhẹ |
| 30 obj · MSAA Off · Shadows 1024 | 333 | 17,960 | `0.26 ms` | `1.45 ms` | `2.73 ms` (`3.56`) | `366.5` | Chi phí shadow chỉ `+0.61 ms` |
| **30 obj · MSAA 4× · Shadows 1024** | 333 | 17,960 | `0.34 ms` | `2.04 ms` | **`3.42 ms`** (`4.40`) | **`292.2`** | **Dư >12 ms cho nâng cấp đồ họa!** |
| 100 obj · MSAA Off · Shadows 1024 | 1,102 | 61,540 | `0.62 ms` | `2.45 ms` | `4.60 ms` (`5.64`) | `217.5` | Tốt |
| 250 obj · Stress · Shadows 1024 | 2,896 | 156,292 | `1.58 ms` | `4.61 ms` | `9.04 ms` (`11.08`) | `110.6` | `p95` vẫn < `16.67 ms` (60 Hz) |

### Đánh giá kiến trúc hiệu năng
- **Rất mạnh ở phần mềm hiện tại**: Thời gian GPU thực tế ở cảnh điển hình (`7–30` vật thể + MSAA 4× + Shadow 1024) mới chỉ khoảng **`2.04 ms`**, tức mới tiêu tốn **~12% ngân sách 60 Hz (`16.67 ms`)** của Intel UHD Graphics.
- **Ý nghĩa chiến lược cho nâng cấp thẩm mỹ**: Dự án **KHÔNG bị nghẽn hiệu năng**. Ta hoàn toàn đủ ngân sách GPU để bổ sung:
  - ACES Filmic Tone Mapping + Roughness/Fresnel-aware lighting (`~0.15 ms`),
  - Hardware bilinear PCF / 16-tap Poisson PCF + Shadow 2048 mặc định tùy chọn (`~0.5–1.0 ms`),
  - Contact shadow mềm dưới đáy nội thất (`~0.2 ms`),
  - Gradient nền studio cho viewport (`~0.05 ms`),
  - Baseboard + khung cửa sổ + bevel nhẹ cho nội thất procedural (`~0.1 ms`),
  mà **vẫn duy trì dễ dàng > 180–200 FPS off-screen (khóa cứng 60 FPS ổn định có VSync)** trên Intel UHD Graphics.
- **Lưu ý trung thực**: Các con số `292–471 FPS` là đo trên **off-screen FBO không gồm ImGui & DWM VSync**. Cần bổ sung 1 bài đo full-frame (gồm ImGui pass) sau khi nâng cấp thị giác.

---

## 12. Confirmed Bugs

| Bug ID | Mức độ | Mô tả lỗi đã xác nhận | Vị trí mã nguồn | Bằng chứng thực tế |
|---|---|---|---|---|
| **BUG-01** | **P0 (Critical Visual)** | **Wall Shadow Acne (Vân Moiré đồng tâm trên tường Bắc & Tây)**: Tự đổ bóng theo chu kỳ xuất hiện rõ trên cả hai bức tường khi bật Directional Shadow Mapping do thiếu `GL_CULL_FACE(GL_FRONT)` / `glPolygonOffset` trong shadow pass và thiếu normal-offset bias trong `mesh.frag`. | `src/graphics/SceneRenderer.cpp:82-95`, `assets/shaders/mesh.frag:45-62` | `docs/evidence/independent-qa/analysis_shadow_wall_comparison.png`, `analysis_wall_acne_zoom_2x.png` |
| **BUG-02** | **P0 (Critical Visual)** | **Cháy sáng bệt tường (Overexposure Clipping) ở `Warm Interior` & `Gaming Workspace`**: **`7,620` pixel (`8.42%` phòng)** ở `Warm Interior` và **`5,905` pixel (`5.95%`)** ở `Gaming Workspace` chạm ngưỡng bão hòa `≥ 248/255` do `pointIntensity = 2.0` đặt sát tường và shader hoàn toàn không có tone mapping. | `include/roomcraft/graphics/Lighting.hpp:15-34`, `assets/shaders/mesh.frag:117-122` | `docs/evidence/independent-qa/05_lighting_warm_interior.png`, `02_gaming_workspace.png`, `analysis_wall_acne_zoom_2x.png` |
| **BUG-03** | **P0 (High Visual)** | **Cạnh bóng răng cưa bậc thang & Peter-Panning / mất bóng tiếp xúc chân nội thất**: `ShadowMap` dùng `GL_NEAREST` không bật hardware shadow comparison, PCF 3×3 rời rạc và frustum bóng bao cả tường ngoại biên gây mất chi tiết texel quanh chân đồ nội thất. | `src/graphics/ShadowMap.cpp:36-37`, `src/graphics/ShadowMap.cpp:62-95`, `assets/shaders/mesh.frag:51-61` | `docs/evidence/independent-qa/07_lighting_evening.png`, `analysis_wall_acne_zoom_2x.png` |
| **BUG-04** | **P0 (Functional / Preset)** | **Preset mặc định `Gaming Workspace` xuất xưởng kèm lỗi `1 overlap, 1 bounds`**: Chậu cây `Indoor Plant` vượt biên tường Tây (`x=-1.45` với OBJ rộng `0.89 m`) và `Desk Lamp` (`x=0.52`) va chạm với `Computer Setup` (`x=-0.10`), làm hiện khung dây lỗi đỏ/cam ngay khi mở preset. | `src/scene/ScenePresets.cpp:84-98` | `docs/evidence/independent-qa/02_gaming_workspace.png` |
| **BUG-05** | **P1 (Material)** | **Toàn bộ 8/8 tài sản Blender xuất `Ks 0.500 0.500 0.500` đồng loạt**: Vải sofa, chăn ga, thảm dệt, chụp đèn vải và cả đất trồng cây đều mang specular 0.5 / shininess 225–250 như nhựa bóng. | `assets/models/*/*.mtl` (8 file) & `blender/scripts/create_*.py` | Kiểm tra trực tiếp 8 file `.mtl` + ảnh `01`–`07` |
| **BUG-06** | **P1 (Data Integrity)** | **Lệch kích thước Catalog vs OBJ thực tế ở `blender-indoor-plant`**: Catalog khai báo `0.58 × 1.15 × 0.58 m`, nhưng OBJ thực tế là `0.89 × 1.08 × 0.87 m` (+53% bề rộng), gây tính sai khoảng cách an toàn khi đặt preset. | `src/assets/FurnitureCatalog.cpp:193-206`, `assets/models/plant/indoor_plant.obj` | `02_gaming_workspace.png` + đọc trực tiếp mã nguồn |
| **BUG-07** | **P1 (Shader Logic)** | **`computeRoomCornerAO` làm tối sai đồ nội thất & mép sàn Cutaway trống**: Shader áp dụng room corner AO cho mọi mesh trong phòng (không loại trừ đồ nội thất) và tối hóa cả 2 mép sàn phía Nam/Đông dù tường đã bị ẩn ở chế độ CutawayView. | `assets/shaders/mesh.frag:65-84,112` | Đọc kiểm toán trực tiếp `mesh.frag` |
| **BUG-08** | **P1 (UI Polish)** | **Tên 4/6 card nội thất C++ bị cắt chữ trong Furniture Library**: `Procedural Bo...`, `Procedural De...`, `Procedural Ce...`, `Procedural Cu...` do thừa tiền tố `"Procedural "` dù đã có badge `[C++]`. | `src/assets/FurnitureCatalog.cpp:31-125` | `docs/evidence/independent-qa/01_modern_bedroom.png` |

---

## 13. Suspected Issues (Cần xác minh trên phiên Desktop tương tác thật)

1. **Shadow Shimmering / Swimming khi xoay Camera hoặc kéo vật thể**: Chưa quan sát được video/tương tác trực tiếp; mặc dù light-space ortho đang cố định theo room bounds (nên không swim khi xoay camera — đây là điểm tốt), khi di chuyển đồ nội thất nhỏ với `GL_NEAREST`, cạnh bóng có khả năng nhảy bậc theo từng texel (`~1 cm`).
2. **Độ sắc nét & tốc độ thực tế của `Shadow 512` vs `Shadow 2048`**: Chưa chụp được 2 mức này trên UI vì giới hạn OS desktop capture.
3. **Cắt chữ / tràn bố cục ở `UI Scale 125%` và `150%` trên màn hình `1366×768` hoặc `1280×720`**: Cột trái (`265 px`) và phải (`300 px`) hiện đã khá sát ở 100%; cần kiểm thử thực tế khi phóng 150%.
4. **Hộp thoại Win32 Native File Dialog & Đường dẫn Unicode có dấu tiếng Việt**: Đã có unit test C++ cho `std::filesystem::path`, nhưng cần kiểm chứng trực tiếp hộp thoại `GetOpenFileNameW` / `GetSaveFileNameW` trong bản portable.
5. **Khả năng chạy trên máy Windows sạch không cài MinGW/vcpkg**: Chưa kiểm chứng trên máy thứ 2 (mới chạy trên máy phát triển có sẵn môi trường hệ thống).

---

## 14. Screenshot Evidence

Toàn bộ ảnh chụp thật được kết xuất trực tiếp bằng GPU OpenGL 3.3 từ gói phát hành **`dist/RoomCraft3D-v1.0.0-rc1-win64/RoomCraft3D.exe`** và lưu trong `docs/evidence/independent-qa/`. Không có bất kỳ ảnh minh họa hoặc ảnh giả nào.

| # | Hạng mục yêu cầu kiểm toán | Trạng thái | Tệp bằng chứng thực tế (`docs/evidence/independent-qa/`) | Ghi chú kiểm toán |
|---|---|---|---|---|
| 1 | Modern Bedroom | **CAPTURED** | `01_modern_bedroom.png` | `1280×720` GPU FBO · Viewport coverage `24.3%` · Trục RGB & Grid bật mặc định |
| 2 | Gaming Workspace | **CAPTURED** | `02_gaming_workspace.png` | `1280×720` GPU FBO · Xác nhận **BUG-04** (`1 overlap, 1 bounds`) & cháy tường (`5.95%`) |
| 3 | Minimalist Living Room | **CAPTURED** | `03_minimalist_living_room.png` | `1280×720` GPU FBO · Viewport coverage chỉ `22.5%`, phòng quá thưa |
| 4 | Daylight | **CAPTURED** | `04_lighting_daylight.png` | `1280×720` GPU FBO · Sáng phẳng, có wall shadow acne nhẹ |
| 5 | Warm Interior | **CAPTURED** | `05_lighting_warm_interior.png` | `1280×720` GPU FBO · Xác nhận **BUG-02** (`7,620 px = 8.42%` cháy sáng tường) |
| 6 | Neutral Studio | **CAPTURED** | `06_lighting_neutral_studio.png` | `1280×720` GPU FBO · Ambient quá cao (`0.50`), bật dấu `*` dirty |
| 7 | Evening | **CAPTURED** | `07_lighting_evening.png` | `1280×720` GPU FBO · Xác nhận **BUG-03** (bóng tủ răng cưa & peter-panning) |
| 8 | Shadow Off | **CAPTURED** | `12_shadows_off_comparison.png` | `1280×720` GPU FBO · Dùng làm baseline so sánh shadow |
| 9 | Shadow 512 | **NOT CAPTURED (BLOCKED)** | *(không có — OS capture blocked, cấm sửa code thêm CLI)* | Cần chụp tay hoặc bổ sung tùy chọn sau khi được duyệt |
| 10 | Shadow 1024 | **CAPTURED** | `04_lighting_daylight.png` + `analysis_shadow_wall_comparison.png` | Kèm ảnh khuếch đại hiệu 4× & zoom 2× (`analysis_wall_acne_zoom_2x.png`) chứng minh **BUG-01** |
| 11 | Shadow 2048 | **NOT CAPTURED (BLOCKED)** | *(không có — OS capture blocked)* | Cần chụp tay hoặc kiểm chứng sau nâng cấp |
| 12 | Perspective View | **CAPTURED** | `01_modern_bedroom.png` | Góc nhìn mặc định quá xa (`~75%` nền trống) |
| 13 | Isometric View | **NOT CAPTURED (BLOCKED)** | *(không có — OS capture blocked)* | Cần kiểm chứng trực quan bằng tay |
| 14 | Grid On & Grid Off | **CAPTURED (Grid On only)** | Grid On: `01_modern_bedroom.png` · Grid Off: `NOT CAPTURED (BLOCKED)` | Grid On mặc định làm rối sàn gỗ |
| 15 | Room Settings Panel | **NOT CAPTURED (BLOCKED)** | *(không có — OS capture blocked)* | Cần chụp tay để kiểm tra tràn chữ ở 125%–150% |
| 16 | Furniture Library (Grid) & Asset Inspector / Warnings | **CAPTURED** | Library Grid: `01_modern_bedroom.png` · Inspector: `09_asset_inspector.png` · Warnings: `10_collision_warning.png`, `11_room_boundary_warning.png` · Geometry Lab: `08_geometry_lab.png` | Xác nhận cắt chữ 4 card C++ (`BUG-08`) và hiển thị warning |

---

## 15. Design Quality Scorecard

Thang điểm khách quan **0–10** (không chấm cao chỉ vì pass test; chuẩn tham chiếu `8.0/10` = đạt mức sản phẩm thương mại/portfolio chuyên nghiệp cho ứng dụng OpenGL 3.3 Core):

| # | Hạng mục đánh giá | Điểm (0–10) | Lý do & Bằng chứng trực quan |
|---|---|---:|---|
| 1 | **UI/UX Layout** | **7.2** | Bố cục 3 cột hiện đại, sạch, bỏ tab milestone đúng chuẩn; trừ điểm do cắt chữ 4 card C++ (`Procedural Bo...`), Hierarchy quá hẹp (`0.36`), và dirty `*` khi xem trước lighting. |
| 2 | **Viewport Presentation** | **4.8** | Căn phòng chỉ chiếm `22.5%–26.7%` khung hình trên nền tối phẳng `#171A20`, trục RGB debug và lưới bật sẵn cắt ngang đồ nội thất. |
| 3 | **Camera Framing** | **4.5** | `Camera::fitBoundingBox` đẩy camera quá xa vì cộng cả ngoại biên tường ẩn + `paddingFactor = 1.22` + `projHalfZ`, làm mất cảm giác đắm chìm vào không gian phòng. |
| 4 | **Lighting Quality** | **5.2** | Có 4 preset và gamma sRGB đúng chuẩn, nhưng thiếu tone mapping khiến `Warm Interior` cháy sáng `8.42%`, `Daylight/Studio` phẳng do ambient cao, `computeRoomCornerAO` làm tối sai mép cutaway và nội thất. |
| 5 | **Shadow Quality** | **4.8** | Đã có shadow thật nhưng bị **wall shadow acne vân Moiré (BUG-01)**, răng cưa bậc thang `GL_NEAREST`, peter-panning ở `Evening` (BUG-03) và thiếu contact shadow grounding dưới chân nội thất. |
| 6 | **Material Realism** | **4.5** | Lỗi hệ thống `Ks = 0.50` nhựa hóa toàn bộ 8 asset Blender (kể cả vải/thảm/đất), Blinn-Phong chưa bảo toàn năng lượng, không có Fresnel/roughness differentiation, đồ procedural đơn sắc. |
| 7 | **Furniture Modeling Quality** | **5.8** | 8 mẫu Blender có silhouette khá tốt (`Bed`, `Sofa`, `Gaming Chair`, `Wardrobe`), nhưng 6 mẫu C++ quá vuông sắc (`Desk`, `Bookshelf` trống rỗng) tạo sự chênh lệch phong cách rõ rệt; `Indoor Plant` lệch kích thước catalog 53%. |
| 8 | **Room Composition** | **5.0** | `Gaming Workspace` có sẵn lỗi `1 overlap, 1 bounds` khi mở; `Living Room` quá loãng; cả 3 phòng đều thiếu len chân tường, cửa sổ, tranh/phụ kiện tường. |
| 9 | **Visual Consistency** | **5.0** | Sự tương phản quá lớn giữa giao diện ImGui hiện đại (7.2) với cảnh 3D trông như demo CAD, và giữa nội thất Blender có texture với nội thất Procedural khối hộp trơn. |
| 10 | **Professional Appearance** | **5.5** | Có dáng dấp ứng dụng thực thụ hơn nhiều so với M4–M6, nhưng các lỗi acne/cháy sáng/cảnh nhỏ/preset lỗi khiến ấn tượng đầu tiên chưa đạt chất lượng phát hành `v1.0.0`. |
| 11 | **Overall Visual Quality** | **5.1** | Trung bình có trọng số phản ánh đúng thực tế: nền kỹ thuật tốt, nhưng thẩm mỹ cảnh 3D mới ở mức *"prototype kỹ thuật hoàn chỉnh"*, **chưa đạt chuẩn *"ứng dụng thiết kế nội thất đẹp mắt"* (`≥ 7.8/10`)**. |

---

## 16. Prioritized Improvement Roadmap

Tất cả giải pháp dưới đây đều tương thích hoàn toàn với **C++17 + OpenGL 3.3 Core + GLSL 330 + FreeGLUT + Dear ImGui + Intel UHD Graphics**, **không thêm dependency mới** và **không phá vỡ kiến trúc hiện tại**.

---

### 🔴 P0 — Phải sửa ngay trước khi phát hành (Blockers cho `v1.0.0`)

#### P0.1 — Sửa triệt để Wall Shadow Acne, răng cưa bóng và Peter-Panning
- **Vấn đề**: Vân Moiré đồng tâm trên tường Bắc/Tây (`BUG-01`), bóng tủ răng cưa và hở chân (`BUG-03`).
- **Nguyên nhân gốc**: Shadow pass vẽ tường không front-face culling / polygon offset; `ShadowMap` dùng `GL_NEAREST` không bật hardware shadow compare; thiếu normal-offset bias; frustum ánh sáng bao quá rộng.
- **Bằng chứng ảnh chụp**: `docs/evidence/independent-qa/analysis_shadow_wall_comparison.png`, `analysis_wall_acne_zoom_2x.png`, `07_lighting_evening.png`.
- **Tệp nguồn liên quan**: `src/graphics/ShadowMap.cpp`, `src/graphics/SceneRenderer.cpp`, `assets/shaders/mesh.vert`, `assets/shaders/mesh.frag`.
- **Giải pháp kỹ thuật đề xuất**:
  1. Trong `ShadowMap.cpp`: chuyển `GL_TEXTURE_MIN_FILTER` / `MAG_FILTER` sang `GL_LINEAR` và bật `GL_TEXTURE_COMPARE_MODE = GL_COMPARE_REF_TO_TEXTURE`, `GL_TEXTURE_COMPARE_FUNC = GL_LEQUAL` (dùng `sampler2DShadow` trong GLSL 330 để có hardware bilinear PCF miễn phí).
  2. Trong `SceneRenderer::renderShadowPass()`: bật `glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(1.8f, 2.5f);` cho pass chiều sâu; tách mặt tường kín hoặc áp dụng `normalOffsetBias` theo kích thước texel (`texelWorldSize * (1.0 - ndotl)`) trực tiếp vào `worldPos` trước khi nhân `uLightSpaceMatrix`.
  3. Nâng cấp lọc bóng trong `mesh.frag` sang **12–16 mẫu Rotated Poisson Disk PCF** với `sampler2DShadow` (cho biên bóng mềm tự nhiên không bậc thang).
  4. Thu gọn light-space orthographic bounds bám sát interior room + furniture bounds thay vì ngoại biên thừa.
- **Độ phức tạp**: Trung bình (`3–4 giờ`).
- **Rủi ro**: Thấp (cần kiểm tra để không tạo ánh sáng lọt khe ở chân tường mỏng).
- **Ảnh hưởng hiệu năng**: `+0.3 – 0.6 ms` GPU trên Intel UHD (vẫn > 220 FPS off-screen).
- **Tiêu chí nghiệm thu (Acceptance Criteria)**: Hiệu ảnh khuếch đại 4× trên vùng tường không bị che khuất có độ lệch chuẩn `< 0.5 luma` (0 vân Moiré); bóng tủ ở `Evening` có biên mềm, không bậc thang và bám khít chân tủ.

#### P0.2 — Bổ sung ACES Filmic Tone Mapping & Cân bằng lại 4 Lighting Presets
- **Vấn đề**: `Warm Interior` cháy sáng **`8.42%`** diện tích phòng, `Gaming Workspace` cháy **`5.95%`** (`BUG-02`); `Daylight` và `Neutral Studio` bị phẳng do ambient quá cao.
- **Nguyên nhân gốc**: `mesh.frag:122` cắt cứng tuyến tính tại `1.0` không có tone mapping; `pointIntensity = 2.0` đặt quá sát tường trắng với attenuation yếu.
- **Bằng chứng ảnh chụp**: `05_lighting_warm_interior.png`, `02_gaming_workspace.png`, `04_lighting_daylight.png`, `06_lighting_neutral_studio.png`.
- **Tệp nguồn liên quan**: `assets/shaders/mesh.frag`, `include/roomcraft/graphics/Lighting.hpp`, `src/graphics/SceneRenderer.cpp`.
- **Giải pháp kỹ thuật đề xuất**:
  1. Thêm hàm **ACES Filmic Tone Mapping** (Narkowicz 2015, chỉ 5 phép toán vec3) trước `displayColor()` khi `uLit && uShadowEnabled != 0` (hoặc điều khiển qua uniform `uToneMapEnabled` để không làm gãy unit test đo pixel tuyến tính M7 cũ).
  2. Cân chỉnh 4 preset trong `Lighting.hpp`: hạ `Daylight.ambient` từ `0.42 → 0.28`, tăng chiều sâu hemisphere ambient (trời xanh ấm nhẹ trên, phản xạ sàn gỗ ấm dưới — 2 màu sky/ground thay vì đơn sắc xám); hạ `WarmInterior.pointIntensity` từ `2.0 → 1.15`, dời đèn điểm về vị trí trung tâm trần `(0.0, 2.45, -0.35)` với attenuation vật lý hơn `(1.0, 0.35, 0.28)`; giảm `NeutralStudio.ambient` xuống `0.36`.
  3. Sửa `computeRoomCornerAO` (`BUG-07`): truyền uniform mặt nạ tường đang hiển thị (`uVisibleWalls`) để **không làm tối mép sàn cutaway trống** và chỉ áp dụng room AO khi đang vẽ mesh phòng (`uIsRoomSurface == 1`), không làm bẩn màu bàn ghế.
- **Độ phức tạp**: Thấp – Trung bình (`2–3 giờ`).
- **Rủi ro**: Thấp (giữ nguyên đường dẫn test M7 khi `uToneMapEnabled == 0`).
- **Ảnh hưởng hiệu năng**: `< 0.1 ms` GPU (không đáng kể).
- **Tiêu chí nghiệm thu**: Số pixel cháy sáng (`max ≥ 248`) ở cả 4 lighting presets trên cả 3 phòng mẫu bằng **`0%`** (hoặc `< 0.1%` chỉ ở highlight kim loại); tương phản giữa vùng sáng và vùng bóng trên tường/sàn tăng rõ rệt.

#### P0.3 — Sửa lỗi Preset mặc định `Gaming Workspace` & Đồng bộ kích thước `Indoor Plant`
- **Vấn đề**: `Gaming Workspace` mở lên có sẵn `Warnings: 1 overlap, 1 bounds` (`BUG-04`); `blender-indoor-plant` sai lệch kích thước catalog `+53%` (`BUG-06`).
- **Nguyên nhân gốc**: Tọa độ `Desk Lamp` và `Indoor Plant` trong `ScenePresets.cpp:84-98` được tính theo kích thước catalog cũ `0.58 m` thay vì kích thước OBJ thật `0.89 m`.
- **Bằng chứng ảnh chụp**: `02_gaming_workspace.png`.
- **Tệp nguồn liên quan**: `src/scene/ScenePresets.cpp`, `src/assets/FurnitureCatalog.cpp`, `tests/unit/SceneUnitTests.cpp`.
- **Giải pháp kỹ thuật đề xuất**:
  1. Cập nhật `defaultDimensions` của `blender-indoor-plant` khớp kích thước chuẩn hóa thực tế (`{0.89f, 1.08f, 0.87f}` — hoặc thu gọn tán lá trong script Blender về `0.62 m` cho đúng tỷ lệ cây trong nhà).
  2. Điều chỉnh tọa độ `Desk Lamp` (`x = 0.56f, z = -2.18f`) và `Indoor Plant` (`x = -1.42f, z = -1.75f` hoặc góc Đông-Nam) trong `Gaming Workspace` sao cho khoảng hở tường/vật thể ≥ `8 cm`.
  3. Bổ sung unit test bắt buộc trong `M4_SceneUnitTests` hoặc `M8_CollisionUnitTests`: **cả 3 preset mặc định phải đạt `overlapCount == 0 && boundaryViolationCount == 0`**.
- **Độ phức tạp**: Thấp (`1 giờ`).
- **Rủi ro**: Rất thấp.
- **Ảnh hưởng hiệu năng**: `0 ms`.
- **Tiêu chí nghiệm thu**: Cả 3 preset mặc định hiển thị `Warnings: 0` trên status bar và vượt qua unit test va chạm tự động.

#### P0.4 — Tối ưu Camera Framing mặc định & Làm sạch Viewport Presentation
- **Vấn đề**: Căn phòng chỉ chiếm `22.5% – 26.7%` diện tích viewport; nền tối phẳng đơn điệu; trục tọa độ RGB debug & lưới sàn bật sẵn gây rối.
- **Nguyên nhân gốc**: `Camera::fitBoundingBox()` (`Camera.cpp:159-161`) dùng ngoại biên cả tường ẩn, `paddingFactor = 1.22` (`CameraController.cpp:150`) và cộng dư `+ projHalfZ`; mặc định bật `showAxes = true` & `showGrid = true`.
- **Bằng chứng ảnh chụp**: `01_modern_bedroom.png`, `02_gaming_workspace.png`, `03_minimalist_living_room.png`.
- **Tệp nguồn liên quan**: `src/graphics/Camera.cpp`, `src/systems/CameraController.cpp`, `src/graphics/SceneRenderer.cpp`, `src/scene/ScenePresets.cpp`, `src/editor/RoomEditor.cpp`.
- **Giải pháp kỹ thuật đề xuất**:
  1. Tính framing cho `fitRoom` theo **bounding box thực của các mặt đang hiển thị** (bỏ tường đã ẩn & chiều cao trống trên đỉnh) với `paddingFactor ≈ 0.92–1.02` và góc camera mặc định 3/4 ấm hơn (`yaw ≈ 42°`, `pitch ≈ 28°–30°`), đưa tỷ lệ chiếm viewport của căn phòng lên **`50% – 62%`**.
  2. Tắt mặc định trục RGB trung tâm (`showAxes = false`) trong Room Editor (giữ toggle trong `View → World Axes` và giữ 1 **mini orientation gizmo nhỏ ở góc trên phải viewport** giống Blender/mockup M6).
  3. Đặt `showGrid = false` mặc định khi khởi động (tự động bật lưới mờ khi bật `Snap` hoặc người dùng nhấn nút `Grid`).
  4. Vẽ một fullscreen background quad nhẹ (1 draw call, không depth write) tạo **radial studio gradient tinh tế** (`#262B36` ở tâm → `#14171D` ở góc) thay vì nền đen chết phẳng `#171A20`.
- **Độ phức tạp**: Trung bình (`3–4 giờ`).
- **Rủi ro**: Thấp (kiểm tra lại M1/M5 camera unit tests để không phá hàm `fitBoundingBox` tổng quát — tạo tham số/overload riêng cho `fitRoomView`).
- **Ảnh hưởng hiệu năng**: `+0.05 ms` GPU.
- **Tiêu chí nghiệm thu**: Tỷ lệ chiếm viewport của cả 3 phòng mẫu đạt **`50% – 65%`**; không có đường trục RGB đâm xuyên đồ nội thất ở trạng thái mặc định; nền viewport có chiều sâu studio mượt không banding.

---

### 🟠 P1 — Nâng cấp Thẩm mỹ & Cảm nhận Chuyên nghiệp (Rất khuyến nghị cho `v1.0.0`)

#### P1.1 — Sửa vật liệu `Ks = 0.50`, nâng cấp Shading gần PBR (Roughness + Fresnel + Contact Grounding)
- **Vấn đề**: Tất cả vải, thảm, gỗ, đất bị bóng nhựa đồng loạt (`BUG-05`); đồ nội thất thiếu bóng tiếp xúc mềm dưới đáy nên trông hơi "lơ lửng".
- **Nguyên nhân gốc**: 8 file `.mtl` đều có `Ks = 0.50`; Blinn-Phong thiếu năng lượng chuẩn hóa & Fresnel; chưa có contact shadow cho đồ nội thất.
- **Bằng chứng ảnh chụp**: `01_modern_bedroom.png`, `04_lighting_daylight.png`, `analysis_wall_acne_zoom_2x.png`.
- **Tệp nguồn liên quan**: `assets/models/*/*.mtl`, `blender/scripts/create_*.py`, `assets/shaders/mesh.frag`, `src/graphics/SceneRenderer.cpp`, `src/graphics/Texture.cpp`.
- **Giải pháp kỹ thuật đề xuất**:
  1. Chuẩn hóa đúng thông số vật lý cho toàn bộ 8 file `.mtl` (và cập nhật script Blender nguồn):
     - **Vải / Thảm / Chụp đèn / Đất**: `Ks = 0.03–0.05`, `Ns = 12–24`.
     - **Gỗ mờ / Tường sơn mờ**: `Ks = 0.08–0.14`, `Ns = 32–56`.
     - **Gốm / Da PU**: `Ks = 0.22–0.32`, `Ns = 80–128`.
     - **Kim loại / Màn hình kính**: `Ks = 0.55–0.80`, `Ns = 180–320`.
  2. Nâng cấp `illuminate()` trong `mesh.frag` thêm **Energy-Conserving Blinn-Phong + Schlick Fresnel** nhẹ (`F = F0 + (1 - F0)*(1 - V·H)^5`) để cạnh vật liệu bắt ánh sáng tự nhiên.
  3. Bổ sung **Soft Analytical Contact Shadow Decal** dưới đáy AABB/OBB từng món đồ nội thất đặt gần sàn (`y ≤ 0.05 m`): 1 quad/ellipse alpha mềm cực nhẹ trên sàn (hoặc tính trong shader sàn từ danh sách tối đa 16–32 box gần nhất) giúp giường, sofa, tủ, bàn, ghế **bám chặt xuống sàn nhà ngay cả trong vùng bóng râm**.
  4. Bật **Anisotropic Filtering (`4×–8×`)** trong `Texture.cpp` nếu extension `GL_EXT_texture_filter_anisotropic` được hỗ trợ.
- **Độ phức tạp**: Trung bình (`4–6 giờ`).
- **Rủi ro**: Trung bình (cần kiểm tra để M7 lighting tests vẫn PASS).
- **Ảnh hưởng hiệu năng**: `+0.2 – 0.4 ms` GPU.
- **Tiêu chí nghiệm thu**: Nhìn rõ sự khác biệt chất liệu giữa vải mờ (sofa/thảm), gỗ satin (giường/tủ/sàn), gốm (bình/chậu) và kim loại (tay nắm/màn hình); mọi chân nội thất đều có vùng tối tiếp xúc tự nhiên trên sàn.

#### P1.2 — Bổ sung chi tiết kiến trúc phòng: Len chân tường (Baseboard), Cửa sổ & Nâng cấp sàn gỗ
- **Vấn đề**: Căn phòng giống hộp 2 mặt phẳng trơn; sàn gỗ có sọc mạch hơi gắt ở góc xa.
- **Nguyên nhân gốc**: `Room.cpp` chỉ sinh 1 khối sàn + 4 khối tường trơn phẳng không có kiến trúc phụ.
- **Bằng chứng ảnh chụp**: `01_modern_bedroom.png`, `03_minimalist_living_room.png`, `analysis_shadow_wall_comparison.png`.
- **Tệp nguồn liên quan**: `src/scene/Room.cpp`, `include/roomcraft/scene/Room.hpp`, `src/graphics/SceneRenderer.cpp`, `assets/shaders/mesh.frag`.
- **Giải pháp kỹ thuật đề xuất**:
  1. Trong `Room.cpp`: sinh thêm **Baseboard (len chân tường cao `0.09 m`, dày `0.018 m`, màu trắng ngà hoặc gỗ sáng)** dọc chân các bức tường đang hiển thị — chi tiết nhỏ này tăng cảm giác "kiến trúc thật" cực mạnh!
  2. Thêm tùy chọn **Architectural Window** (khung cửa sổ tối giản 2 cánh có viền trắng + kính xanh phản chiếu bầu trời nhẹ trên tường Bắc hoặc tường Tây, bật/tắt được trong `Room Settings`) — đúng tinh thần mockup M6 người dùng từng yêu thích.
  3. Tinh chỉnh vân gỗ sàn trong `mesh.frag`: lọc anti-alias mạch ván bằng `fwidth()` để triệt tiêu hoàn toàn moiré ở xa, làm mềm độ tương phản khe ván (`0.91` thay vì `0.82`).
- **Độ phức tạp**: Trung bình (`4–5 giờ`).
- **Rủi ro**: Thấp (tường/baseboard dùng chung cơ chế visibility cutaway, không ảnh hưởng collision bounds bên trong).
- **Ảnh hưởng hiệu năng**: `+0.1 ms` GPU (~40 tam giác tăng thêm).
- **Tiêu chí nghiệm thu**: Ranh giới sàn–tường có len chân tường sắc nét; cửa sổ hiển thị gọn gàng trên tường đang bật và tự ẩn cùng tường khi chuyển Cutaway; sàn gỗ không còn sọc moiré khi nhìn xa.

#### P1.3 — Nâng cấp thẩm mỹ nội thất Procedural C++ & Cân bằng bố cục `Minimalist Living Room`
- **Vấn đề**: `Procedural Desk` và `Procedural Bookshelf` quá vuông sắc/đơn sắc khi đứng cạnh mẫu Blender; `Minimalist Living Room` (`6×7 m`) quá thưa (`22.5%` viewport).
- **Nguyên nhân gốc**: `FurnitureGenerator.cpp` chỉ dùng `createCuboid` thô; `Minimalist Living Room` chọn kích thước phòng quá lớn so với số đồ.
- **Bằng chứng ảnh chụp**: `01_modern_bedroom.png`, `02_gaming_workspace.png`, `03_minimalist_living_room.png`.
- **Tệp nguồn liên quan**: `src/geometry/FurnitureGenerator.cpp`, `src/geometry/PrimitiveGenerator.cpp`, `src/scene/ScenePresets.cpp`.
- **Giải pháp kỹ thuật đề xuất**:
  1. Nâng cấp `FurnitureGenerator.cpp` (giữ nguyên thuật toán C++ M2, chỉ nâng chất lượng hình học): thêm **bevel nhẹ / viền bo cạnh**, thanh giằng & hộc kéo tối giản cho `Desk`, thêm **chân đế + 6–8 cuốn sách màu pastel tối giản** đặt sẵn trên các ngăn `Bookshelf` (sinh hoàn toàn bằng C++ procedural!), và gán màu gỗ ấm hài hòa với bảng màu Scandinavian.
  2. Thu gọn `Minimalist Living Room` từ `6.0 × 7.0 m` xuống **`5.2 × 5.4 m`**, bổ sung thêm 1–2 món từ catalog có sẵn (ví dụ 1 ghế đọc sách `Curved Chair` xoay hướng vào bàn trà + bình gốm + đèn sàn) để không gian đầy đặn, có trọng tâm thị giác.
- **Độ phức tạp**: Trung bình (`3–4 giờ`).
- **Rủi ro**: Thấp (kiểm tra giữ nguyên hoặc cập nhật hợp lý các assertion số đỉnh tối thiểu trong `M2_GeometryUnitTests`).
- **Ảnh hưởng hiệu năng**: `< 0.1 ms` GPU.
- **Tiêu chí nghiệm thu**: Bàn học và kệ sách không còn tạo cảm giác "hộp thô" khi đứng cạnh giường/sofa Blender; `Minimalist Living Room` đạt bố cục cân đối, 0 va chạm và đẹp ngang hoặc hơn `Modern Bedroom`.

#### P1.4 — Đánh bóng UI/UX cục bộ (KHÔNG thiết kế lại giao diện)
- **Vấn đề**: Tên 4/6 card C++ bị cắt chữ (`BUG-08`); khung `SCENE HIERARCHY` quá thấp (`0.36`); đổi Lighting Preset làm hiện dấu `*` dirty không cần thiết.
- **Nguyên nhân gốc**: Tiền tố `"Procedural "` thừa trong `FurnitureCatalog.cpp`; `m_hierarchyRatio = 0.36f` cứng trong `RoomEditor.hpp`; `setLightingPreset` gọi `markDirty()`.
- **Bằng chứng ảnh chụp**: `01_modern_bedroom.png`, `06_lighting_neutral_studio.png`.
- **Tệp nguồn liên quan**: `src/assets/FurnitureCatalog.cpp`, `include/roomcraft/editor/RoomEditor.hpp`, `src/editor/RoomEditor.cpp`.
- **Giải pháp kỹ thuật đề xuất**:
  1. Đổi `displayName` 6 mẫu C++ bỏ tiền tố `"Procedural "` (`Desk`, `Chair`, `Bookshelf`, `Desk Lamp`, `Ceramic Vase`, `Curved Chair`) — giữ nguyên `assetId` (`procedural-desk`, ...) để tương thích ngược 100% với file `.roomcraft` đã lưu!
  2. Tăng `m_hierarchyRatio` mặc định lên `0.45f` (hoặc tự động giãn nhẹ khi không có object được chọn) để hiển thị trọn vẹn 7–8 object không cần cuộn ngay lần mở đầu.
  3. Bổ sung **Viewport Orientation Axis Gizmo** nhỏ gọn góc trên phải (35×35 px) thay thế trục tọa độ khổng lồ giữa phòng.
- **Độ phức tạp**: Thấp (`1–2 giờ`).
- **Rủi ro**: Rất thấp (tương thích ngược hoàn toàn vì không đổi `assetId`).
- **Ảnh hưởng hiệu năng**: `0 ms`.
- **Tiêu chí nghiệm thu**: 14/14 card trong Furniture Library hiển thị trọn vẹn tên không bị `...`; toàn bộ 7 object của phòng mặc định thấy rõ trong Scene Hierarchy mà không cần cuộn.

---

### 🟢 P2 — Hoàn thiện nâng cao (Có thể làm sau hoặc gộp vào `v1.0.0-rc2` / `v1.1.0`)

#### P2.1 — Chế độ Emissive nhẹ cho đèn (`Desk Lamp`, `Floor Lamp`) & Màn hình máy tính trong `Evening` / `Warm Interior`
- **Vấn đề**: Ở chế độ `Evening`, đèn sàn và đèn bàn không phát sáng dù căn phòng có ánh sáng đèn ấm, làm ánh sáng thiếu "nguồn gốc thị giác" (motivated lighting).
- **Nguyên nhân gốc**: Shader chưa có kênh emissive nhẹ cho vật liệu chụp đèn / màn hình.
- **Bằng chứng ảnh chụp**: `07_lighting_evening.png`, `02_gaming_workspace.png`.
- **Tệp nguồn liên quan**: `assets/shaders/mesh.frag`, `include/roomcraft/assets/AssetData.hpp`, `src/graphics/SceneRenderer.cpp`.
- **Giải pháp kỹ thuật đề xuất**: Truyền `uEmissiveColor` theo vật liệu (chỉ kích hoạt cho chụp đèn khi `pointEnabled == true` và màn hình máy tính), giúp bóng tối buổi tối trở nên sống động và có điểm nhấn tự nhiên.
- **Độ phức tạp**: Thấp (`2 giờ`).
- **Rủi ro**: Rất thấp.
- **Ảnh hưởng hiệu năng**: `0 ms`.
- **Tiêu chí nghiệm thu**: Chụp đèn ấm lên dịu dàng trong `Warm Interior` / `Evening`, không cháy trắng nhờ ACES tone mapping.

#### P2.2 — Bổ sung bộ ảnh chụp đầy đủ & kiểm thử hồi quy chất lượng hình ảnh tự động
- **Vấn đề**: Chưa có ảnh chụp cho `Shadow 512`, `Shadow 2048`, `Isometric`, `Grid Off`, `Room Settings Panel`, `Library List View`; test hiện tại không phát hiện overexposure hay wall acne.
- **Nguyên nhân gốc**: `--m9-capture-evidence` mới chỉ chụp 12 kịch bản cố định và chỉ kiểm tra `AvgLuma`.
- **Bằng chứng ảnh chụp**: Bảng Mục 14.
- **Tệp nguồn liên quan**: `src/main.cpp`, `tests/unit/ReliabilityTests.cpp`.
- **Giải pháp kỹ thuật đề xuất**: Mở rộng `--m9-capture-evidence` (chế độ kiểm chứng) chụp đủ toàn bộ 18 trạng thái (gồm 512/1024/2048, Iso, Grid Off, Room Settings panel, List view) và tự động tính chỉ số kiểm toán (Viewport Coverage %, Clipped Pixels %, Wall Acne Variance) để chặn hồi quy thị giác về sau.
- **Độ phức tạp**: Thấp (`2 giờ`).
- **Rủi ro**: Rất thấp.
- **Ảnh hưởng hiệu năng**: Không ảnh hưởng runtime người dùng.
- **Tiêu chí nghiệm thu**: Đủ 100% ảnh bằng chứng, 0 hạng mục `NOT CAPTURED`, tự động báo FAIL nếu `clippedPixels > 0.5%` hoặc preset có collision warning.

---

## 17. Release Readiness Assessment

### Quyết định cổng phát hành: **NO-GO cho `v1.0.0` (Giữ trạng thái `v1.0.0-rc1`)**

| Tiêu chí phát hành `v1.0.0` | Trạng thái hiện tại | Đạt chuẩn `v1.0.0`? |
|---|---|---|
| CMake Build Debug & Release (0 errors, 0 project warnings) | PASS | Có |
| 19/19 CTest PASS (Debug & Release) | PASS | Có |
| Portable Package `win64` khởi chạy sạch & Unicode đúng | PASS | Có |
| Hiệu năng Release ≥ 60 FPS trên Intel UHD Graphics | PASS (`292–366 FPS` off-screen) | Có |
| Không có P0 Visual / Functional Bugs | **FAIL (`BUG-01`, `BUG-02`, `BUG-03`, `BUG-04`)** | **KHÔNG** |
| Chất lượng thẩm mỹ tổng thể đạt ≥ `7.5/10` | **FAIL (`5.1/10`)** | **KHÔNG** |
| Manual Desktop Acceptance (chuột, Win32 dialog, `WM_CLOSE`) | **NOT VERIFIED / BLOCKED** | **KHÔNG** |
| User Visual Acceptance (Người dùng hài lòng với thẩm mỹ) | **CHƯA ĐẠT** | **KHÔNG** |

### Lộ trình ngắn nhất để đạt `v1.0.0` chất lượng cao (Đề xuất cho `v1.0.0-rc2`)
1. **Đợt 1 (P0 — Sửa toàn bộ lỗi chí mạng + Framing)**: Triển khai `P0.1` (Shadow Map clean PCF + loại bỏ hoàn toàn wall acne) + `P0.2` (ACES Tone Mapping + sửa cháy sáng `Warm Interior` + sửa `computeRoomCornerAO`) + `P0.3` (Sửa preset `Gaming Workspace` 0 lỗi + đồng bộ kích thước `Indoor Plant`) + `P0.4` (Camera Framing 55–60% viewport + Studio gradient background + tắt mặc định trục RGB & lưới).
2. **Đợt 2 (P1 — Lột xác thẩm mỹ rõ rệt)**: Triển khai `P1.1` (Sửa `Ks` chuẩn vật liệu trên 8 `.mtl` + Fresnel + Contact Grounding Shadow) + `P1.2` (Len chân tường + Khung cửa sổ kiến trúc + Sàn gỗ chống moiré) + `P1.3` (Nâng cấp Desk/Bookshelf procedural + cân đối Living Room) + `P1.4` (Sửa tên card bị cắt chữ + cân bằng Hierarchy + Mini Orientation Axis).
3. **Đợt 3 (Nghiệm thu `v1.0.0-rc2`)**: Chụp lại bộ ảnh đối chiếu Before/After đầy đủ 100%, chạy lại toàn bộ CTest + Benchmark, và gửi người dùng kiểm thử tương tác tay 5 phút trên bản portable trước khi gắn nhãn phát hành chính thức `v1.0.0`.

> **Trạng thái hiện tại**: Báo cáo kiểm toán độc lập đã hoàn tất. **Chưa có bất kỳ dòng mã nguồn nào bị thay đổi.** Chờ quyết định phê duyệt của người dùng về lộ trình cải thiện **P0 + P1** trước khi chuyển sang giai đoạn triển khai.
