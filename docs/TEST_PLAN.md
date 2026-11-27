# Kế hoạch kiểm thử — RoomCraft 3D (Test Plan)

## 1. Chiến lược kiểm thử

Dự án áp dụng mô hình kiểm thử 3 tầng:
1. **Unit Testing** (`RoomCraft3D_UnitTests`, thuần CPU, không phụ thuộc GPU): Kiểm thử thuật toán toán học (`Transform`, `Camera`) và kiểm tra tính hợp lệ dữ liệu hình học (`Mesh::validateData`).
2. **Integration & Runtime Smoke Testing** (`RoomCraft3D --smoke-test`, `RoomCraft3D --m1-smoke-test`): Kiểm thử trên ngữ cảnh OpenGL 3.3 Core thật (biên dịch/lỗi shader, upload VAO/VBO/EBO, chứng minh Depth Testing độc lập thứ tự draw, hoàn thiện & resize Framebuffer, đa vật thể, vòng đời RAII).
3. **Manual GUI Testing**: Người dùng chạy trực tiếp `RoomCraft3D.exe`, kiểm tra trực quan và thao tác giao diện Dear ImGui.

## 2. Danh mục Test Case — Milestone M0 (Project Foundation)

| Test ID | Hạng mục | Kịch bản kiểm thử | Kết quả mong đợi |
|---|---|---|---|
| **TC-M0-01** | CMake Configure | Chạy `cmake --preset mingw-x64-debug` | Cấu hình thành công, mã thoát `0` |
| **TC-M0-02** | Build Debug | Biên dịch `RoomCraft3D` ở cấu hình Debug | Biên dịch và liên kết thành công |
| **TC-M0-03** | Khởi động | Chạy `RoomCraft3D.exe --smoke-test` | Tạo cửa sổ, OpenGL 3.3 Core + thông tin GPU |
| **TC-M0-04** | Render Triangle | Vẽ tam giác bằng VAO, VBO, GLSL 330 | Pixel tâm khớp màu nội suy, không lỗi GL |
| **TC-M0-05** | Resize Window | Thay đổi kích thước cửa sổ (`960x540`) | `glViewport` và ImGui cập nhật đúng |
| **TC-M0-06** | Dear ImGui | Khởi tạo `imgui_impl_glut` + `imgui_impl_opengl3` | Không lỗi khởi tạo, render UI ổn định |
| **TC-M0-07** | Đóng chương trình | Giải phóng tài nguyên trước khi hủy context | Thoát bình thường mã `0`, không double-delete |

## 3. Danh mục Test Case — Milestone M1 (Core Graphics Engine)

| Test ID | Hạng mục | Kịch bản kiểm thử | Kết quả mong đợi |
|---|---|---|---|
| **TC-M1-01** | Regression M0 | Chạy `M0_SmokeTest` sau khi refactor M1 | Toàn bộ kiểm thử M0 vẫn đạt `PASS` |
| **TC-M1-02** | Shader hợp lệ | Nạp, biên dịch, link `mesh.vert/frag` và `grid.vert/frag` | Trả về `true`, `isValid() == true` |
| **TC-M1-03** | Shader sai cú pháp | Biên dịch mã nguồn GLSL có lỗi cú pháp | Trả về `false`, handle `0`, có thông báo lỗi từ driver |
| **TC-M1-04** | Indexed Mesh | Upload hình lập phương đơn vị (24 đỉnh, 36 chỉ mục) và vẽ bằng `glDrawElements` | `isValid() == true`, đúng số đỉnh/chỉ mục, không lỗi GL |
| **TC-M1-05** | Invalid Indices | Truyền chỉ mục vượt biên (`>= vertexCount`), buffer rỗng, sai bội số topology hoặc NaN | Phát hiện lỗi, trả về `false` kèm thông báo rõ ràng, không tạo GPU handle rò rỉ |
| **TC-M1-06** | Translation | Biến đổi điểm bằng `Transform` với `position = (2, -1.5, 4.25)` | Điểm đích khớp nghiệm lý thuyết trong ngưỡng `epsilon = 1e-4` |
| **TC-M1-07** | Rotation | Xoay +90° quanh trục X, Y, Z theo hệ tay phải Y-up | Vector đơn vị quay đúng trục và chiều bàn tay phải |
| **TC-M1-08** | Scaling | Áp dụng Uniform Scale (2×), Non-uniform Scale `(2, 0.5, 3)` và kiểm tra `M = T*R*S` + từ chối scale ≤ 0 / NaN | Tọa độ đích chính xác; scale không hợp lệ bị từ chối |
| **TC-M1-09** | Perspective Projection | Kiểm tra `glm::perspective` (foreshortening: điểm gần có NDC lớn hơn điểm xa) + từ chối aspect/clip/FOV/lookAt lỗi | Ma trận hữu hạn, $w_{\text{far}} > w_{\text{near}}$, input lỗi bị từ chối |
| **TC-M1-10** | Orthographic Projection | Kiểm tra `glm::ortho` (bảo toàn tỷ lệ song song, $w = 1$) + kiểm tra Top/Front presets | Tọa độ chiếu gần/xa trùng khớp tuyệt đối, $w = 1$ |
| **TC-M1-11** | Depth Testing | Render 2 khối chồng nhau (Đỏ gần, Xanh dương xa) theo cả 2 thứ tự draw (Xa→Gần và Gần→Xa) khi Depth ON + đối chứng Depth OFF | Cả 2 thứ tự draw khi Depth ON đều cho pixel Đỏ; khi Depth OFF khối vẽ sau (Xanh dương) ghi đè |
| **TC-M1-12** | Multi-object Rendering | Render 3 cube 3D khác vị trí/tỷ lệ/màu + Grid + XYZ Axis trong 10 frames | `drawCalls >= 3`, đọc pixel tại vị trí chiếu của 3 cube cho 3 màu chủ đạo Đỏ/Xanh dương/Xanh lá |
| **TC-M1-13** | Framebuffer Completeness | Tạo off-screen FBO (`RGBA8` + `DEPTH24_STENCIL8`) | `isComplete() == true` (`GL_FRAMEBUFFER_COMPLETE`), `colorTextureId != 0` |
| **TC-M1-14** | Framebuffer Resize | Kiểm tra no-op khi resize cùng kích thước, an toàn khi `resize(0, 0)`, và cập nhật khi đổi sang `800x600` | Không cấp phát lại khi giữ nguyên kích thước; giữ FBO hợp lệ khi `0x0`; kích thước mới complete |
| **TC-M1-15** | Resource Lifecycle (RAII) | Move constructor cho `Shader`, `Mesh`, `Framebuffer`, gọi `destroy()` lặp lại và giải phóng trước `onClose` | Source handle về `0`, không double-delete, thoát sạch trước khi hủy context |
| **TC-M1-16** | OpenGL Errors | Quét `glGetError()` sau toàn bộ integration suite và các khung hình render | Không phát sinh lỗi OpenGL ngoài dự kiến (`GL_NO_ERROR`) |

## 4. Quy tắc ghi nhận kết quả

Kết quả thực tế được ghi tại [`docs/TEST_RESULTS.md`](TEST_RESULTS.md). Chỉ cập nhật `PASS` hoặc `FAIL` sau khi thực thi thật; kiểm thử thủ công chưa được người dùng xác nhận giữ nguyên trạng thái `NOT VERIFIED`.
