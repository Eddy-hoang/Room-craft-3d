# Kế hoạch kiểm thử — RoomCraft 3D (Test Plan)

## 1. Chiến lược kiểm thử

Dự án áp dụng mô hình kiểm thử 3 tầng:
1. **Unit Testing** (M2, M4, M5, M8, M9): Kiểm thử độc lập các thuật toán toán học và logic không phụ thuộc GPU (Bézier, sinh mesh, transform matrix, Ray-AABB, AABB collision, JSON validation).
2. **Integration Testing** (M0, M1, M3, M4, M8): Kiểm thử sự phối hợp giữa các module (CMake build, FreeGLUT + GLAD + ImGui, Shader loading, Assimp model import, Save/Load round-trip).
3. **Manual / Runtime GUI Testing** (M0–M9): Kiểm thử trực quan trên cửa sổ 3D thật, thao tác chuột/bàn phím, thay đổi kích thước cửa sổ và đo FPS thực tế.

## 2. Danh mục Test Case — Milestone M0 (Project Foundation)

| Test ID | Hạng mục | Kịch bản kiểm thử | Kết quả mong đợi |
|---|---|---|---|
| **TC-M0-01** | CMake Configure | Chạy cấu hình CMake với preset mục tiêu | Cấu hình thành công, mã thoát `0`, không thiếu dependency |
| **TC-M0-02** | Build Debug | Biên dịch target `RoomCraft3D` ở cấu hình Debug | Biên dịch và liên kết thành công, tạo ra `RoomCraft3D.exe` |
| **TC-M0-03** | Khởi động ứng dụng | Chạy `RoomCraft3D.exe` | Cửa sổ FreeGLUT mở ra, tạo được OpenGL 3.3 Core Context và in thông tin GPU/OpenGL/GLSL |
| **TC-M0-04** | Render Triangle | Vẽ tam giác bằng VAO, VBO và GLSL `#version 330 core` | Shader biên dịch/link thành công, khung hình vẽ không phát sinh `glGetError()` |
| **TC-M0-05** | Resize Window | Thay đổi kích thước cửa sổ (`glutReshapeWindow` hoặc kéo góc cửa sổ) | Callback reshape được gọi, `glViewport` và ImGui DisplaySize cập nhật đúng |
| **TC-M0-06** | Tích hợp Dear ImGui | Khởi tạo `imgui_impl_glut` + `imgui_impl_opengl3` | Không lỗi khởi tạo, cửa sổ chẩn đoán ImGui render thành công đè lên nền OpenGL 3.3 Core |
| **TC-M0-07** | Đóng chương trình | Đóng cửa sổ / nhấn ESC / `glutLeaveMainLoop()` | Giải phóng VAO/VBO/Shader và ImGui trước khi hủy context, thoát với mã `0` |

## 3. Kế hoạch Test Case cho các milestone sau (tham chiếu)

- **M1**: `TC-M1-01`..`TC-M1-06` (Shader hợp lệ/lỗi, Cube, Depth Test, Resize Framebuffer, Giải phóng Mesh).
- **M2**: `TC-M2-01`..`TC-M2-07` (Primitive mesh, Normal, Bézier $t=0/1$, Segment, Surface of Revolution, Input validation).
- **M3**: `TC-M3-01`..`TC-M3-07` (Import OBJ/MTL, multi-mesh, scale mét, pivot đáy, fallback thiếu texture, model lỗi).
- **M4**: `TC-M4-01`..`TC-M4-06` (Tạo/resize phòng, thêm/xóa/nhân bản nội thất, từ chối kích thước âm).
- **M5**: `TC-M5-01`..`TC-M5-08` (Move/Rotate/Scale, chuyển 2D/3D, picking gần nhất, không pick xuyên UI, resize viewport).
- **M6**: `TC-M6-01`..`TC-M6-06` (Library, Hierarchy, Inspector, Resize UI, Input focus, Error handling).
- **M7**: `TC-M7-01`..`TC-M7-06` (Lighting toggle/intensity, Texture, Fallback, Non-uniform scale normal, Benchmark 30 vật thể).
- **M8**: `TC-M8-01`..`TC-M8-08` (Collision tách rời/chồng lấn/tiếp xúc/ngoài phòng, Save/Load JSON, JSON lỗi, thiếu asset).

## 4. Quy tắc ghi nhận kết quả

Kết quả thực tế được ghi tại [`docs/TEST_RESULTS.md`](TEST_RESULTS.md). Chỉ ghi `PASS` hoặc `FAIL` sau khi thực thi thật; nếu chưa chạy hoặc bị chặn bởi môi trường, ghi rõ `NOT RUN` hoặc `BLOCKED`.
