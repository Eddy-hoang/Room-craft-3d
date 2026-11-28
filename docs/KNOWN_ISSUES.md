# Danh sách lỗi và giới hạn đã biết — RoomCraft 3D (Known Issues)

Cập nhật lần cuối: **2026-11-28**

## 1. Giới hạn về môi trường & build

| ID | Mức độ | Mô tả | Trạng thái / Hướng xử lý |
|---|---|---|---|
| **KI-M0-01** | Trung bình | Máy phát triển chưa cài Visual Studio 2022 / MSVC Build Tools và `vcpkg`; preset `msvc-x64-vcpkg-debug` chưa chạy kiểm chứng (`NOT RUN`). | Duy trì môi trường chính: WinLibs MinGW-w64 x64 UCRT (`GCC 16.2.0`) + CMake FetchContent khóa commit & `URL_HASH SHA256` + Ninja. |
| **KI-M0-02** | Thấp | Trong System PATH vẫn có `C:\MinGW\bin` (32-bit GCC 6.3.0 cũ). | Preset khóa `x86_64-w64-mingw32-gcc/g++`; cần đưa WinLibs `mingw64\bin` lên đầu PATH khi chạy `cmake`/`ninja`. |
| **KI-M1-01** | Thấp | CMake `4.4.4` báo 2 cảnh báo deprecation từ file `CMakeLists.txt` nội bộ của `GLM 1.0.1` (`cmake_minimum_required < 3.10`). | Không ảnh hưởng build/runtime; mã nguồn dự án đạt `0` warnings dưới `-Wall -Wextra -Wpedantic`. |
| **KI-M3-01** | Thấp | WinLibs CMake `curl` trên Windows không có CA bundle mặc định khi tải archive HTTPS. | `CMakeLists.txt` tự gán `CMAKE_TLS_CAINFO` tới CA bundle của Git for Windows và xác thực `URL_HASH SHA256`. |
| **KI-M3-02** | Thấp | `Blender 5.2.3 LTS` giải nén portable ngoài System PATH. | Chỉ cần khi sinh lại asset bằng `bpy`; `RoomCraft3D.exe` chạy độc lập không cần Blender. |

## 2. Giới hạn về kiểm thử thủ công, thị giác & phạm vi Milestone M2–M6

| ID | Mức độ | Mô tả | Hướng xử lý / Ghi chú |
|---|---|---|---|
| **KI-M2-01** | Trung bình | Các kiểm thử tương tác GUI thủ công `TC-M0-MANUAL`..`TC-M6-MANUAL` và kiểm tra thị giác / ảnh chụp giao diện M6 đang ở trạng thái **`NOT VERIFIED`**. | Người dùng mở trực tiếp `.\build\mingw-x64-debug\RoomCraft3D.exe` và xác nhận checklist trực quan trong báo cáo M6. |
| **KI-M4-01** | Thấp | Benchmark 10/30/100 vật thể đo trên bản `Debug` off-screen FBO + `glFinish()` (20–30 frame), chưa đo bản `Release` và phiên dài end-to-end. | Sẽ mở rộng benchmark dài hơi hơn và đo Release ở **Milestone M9**. |
| **KI-M4-02** | Đã xử lý (M8) | Kiểm tra va chạm nội thất (AABB + 3D OBB SAT) và xác thực biên phòng đã hoàn tất trong M8; giới hạn hộp bao lồi OBB được ghi nhận tại `KI-M8-01`. | Xem [`docs/COLLISION_SYSTEM.md`](COLLISION_SYSTEM.md). |
| **KI-M5-02** | Thấp | `M5_IntegrationSmokeTest` & `M6_IntegrationSmokeTest` kiểm thử thuật toán Ray Casting / Picking / Move Gizmo / Drag / Cancel trên ngữ cảnh OpenGL thật nhưng mô phỏng tọa độ chuột thay vì gửi sự kiện chuột vật lý của hệ điều hành qua FreeGLUT/ImGui. | Ghi nhận trung thực; thao tác chuột/phím thực tế cần được người dùng xác nhận qua `TC-M5-MANUAL` / `TC-M6-MANUAL`. |
| **KI-M6-01** | Thấp | **UI Scaling (`100% / 125% / 150%`)** được chọn chủ động qua `View -> UI Scale`; FreeGLUT không cung cấp sự kiện tự động đổi DPI động khi kéo cửa sổ giữa hai màn hình khác hệ số scale Windows. | Hành vi thiết kế an toàn trong phạm vi FreeGLUT; chữ và vector icon sắc nét ngay sau khi chọn mức scale. |
| **KI-M6-02** | Thấp | Hạng mục tùy chọn **P2 (`M6-T22`: Rotate / Scale Visual Gizmos)** và **P1 (`M6-T23`: UI Layout Persistence giữa các phiên chạy)** đang giữ trạng thái **`TODO`**. | Hiện tại **3-Axis Move Gizmo (P1 — `M6-T21`)** đã hoạt động đầy đủ; Rotate/Scale thao tác trực tiếp bằng `R` / `S` + trục `X/Y/Z` và Inspector. |
| **KI-M6-03** | Thông tin | Chưa có tính năng **Shadow Mapping (M7 P2)**, **Collision Detection & JSON Save/Load (M8)**, **Undo/Redo History đầy đủ** (cố tình không đặt nút giả trên thanh menu). | Thuộc phạm vi các milestone kế tiếp theo [`docs/ROADMAP.md`](ROADMAP.md). |

## M7

- Nợ nghiệm thu thủ công M6 còn mở cho tương tác chuột/phím và Furniture Library ở Grid View. Bộ ảnh M7 cho thấy bố cục và màu sắc trong các cấu hình đã chụp. Việc ký duyệt trải nghiệm trên máy người dùng còn chờ.
- Bóng đổ thời gian thực được hoãn. Một lượt dựng depth từ đèn hướng sẽ cần vẽ lại caster, đo ngân sách p95 và kiểm tra bias của vật nhỏ ở nhiều kích thước phòng.
- MSAA resolve trung bình các mẫu đã mã hóa để hiển thị. Ở đường biên tương phản mạnh, kết quả có thể tối hơn resolve trong HDR tuyến tính. Ảnh chụp và số đo trong báo cáo dùng Intel UHD.

### Final QA M7

- CMake và CTest dùng bản M7 tại đường dẫn mặc định. Output trong m7-bin là bản cũ.
- p95 biến động giữa các vòng đo với WGL swap interval 1. Nguyên nhân chênh lệch giữa hai lượt đo lịch sử chưa xác định. Shadow Mapping giữ DEFERRED.
- Ảnh Neutral Studio và Evening còn thiếu. Checklist Manual/Visual của M7 và nợ Manual M6 vẫn mở. [Bằng chứng QA](M7_DELIVERY.md#kiểm-toán-kỹ-thuật-cuối-m7)

## M8

- **KI-M8-01 (Giới hạn hình học hộp bao)**: Hệ thống va chạm M8 dùng **World OBB + 15-axis SAT** làm xấp xỉ hình học lồi cho nội thất (không phải triangle-level mesh collision hay Physics Engine). Các vật có khoang rỗng (như gầm bàn) có thể báo giao OBB khi đặt ghế sát gầm → giữ `Warning Only` làm mặc định.
- **KI-M8-02 (Hạng mục P2)**: `UI Layout Persistence` (`M8-F18`) và lưu `Recent Projects` giữa các phiên chạy đã hoàn tất trong **M9.3** (`PreferencesManager` tại `%APPDATA%\RoomCraft3D\preferences.json`); chỉ còn `Rotate/Scale Visual Gizmos` (`M6-T22` / `M9-F18`) giữ trạng thái `DEFERRED` (thao tác `R`/`S` + trục `X/Y/Z` + Inspector hoạt động đầy đủ).
- **KI-M8-03 (Kiểm thử thủ công)**: Đã chụp `12/12` ảnh GPU thật `1280×720` (`docs/evidence/m9/`) và hoàn tất benchmark Release (`docs/PERFORMANCE_REPORT.md`); nghiệm thu thao tác thủ công cuối cùng trên cửa sổ tương tác vẫn chờ người dùng xác nhận trong [`docs/RELEASE_CHECKLIST.md`](RELEASE_CHECKLIST.md) trước khi gắn thẻ `v1.0.0`.

## M9 (`v1.0.0-rc1` — Release Candidate)

- **KI-M9-01 (Kiểm thử Clean Machine)**: Bản portable ZIP đã kiểm chứng chạy trong thư mục cô lập chứa dấu tiếng Việt & khoảng trắng với `PATH` sạch (`C:\Windows\System32;C:\Windows`) trên máy phát triển; chưa chạy thử trên một máy tính Windows vật lý thứ hai hoàn toàn mới.
- **KI-M9-02 (Cảnh báo nhẹ trong Preset M4 `Gaming Workspace`)**: Ảnh chụp `02_gaming_workspace.png` ghi nhận chậu cây góc tường & đèn trên bàn gaming có chạm nhẹ biên/vật khác theo bộ kiểm tra OBB nghiêm ngặt (`Warnings: 1 overlap, 1 bounds`). Giữ nguyên tọa độ preset M4 để bảo toàn tương thích test M4; người dùng có thể dịch nhẹ hoặc bật `Off` nếu muốn.
- **KI-M9-03 (Giới hạn số Draw Call ở cảnh rất đông >500 vật thể)**: Ở cảnh stress `250` vật thể (`2,896` draw calls), bản Release vẫn đạt `110.6 FPS` (`9.04 ms`), nhưng nếu tương lai mở rộng lên `500+` vật thể cần cân nhắc instanced rendering.
- **KI-M9-04 (Phạm vi hoãn chính thức)**: `Rotate/Scale Visual Gizmos` (`M9-F18`), đa nguồn bóng đổ (multi-light shadows) và Physics Engine được ghi nhận hoãn ngoài phạm vi `v1.0.0`.
