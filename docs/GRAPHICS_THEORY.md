# Cơ sở lý thuyết Computer Graphics — RoomCraft 3D

Tài liệu này tổng hợp các khái niệm và thuật toán Computer Graphics được triển khai trong mã nguồn, phục vụ quá trình học tập và bảo vệ đồ án.

---

## 1. Modern OpenGL 3.3 Core: VAO, VBO, EBO và Vertex Layout

### 1.1. Tại sao dùng VAO, VBO, EBO thay vì `glBegin`/`glEnd`?
- **Legacy OpenGL** gửi từng đỉnh qua CPU mỗi khung hình, gây nghẽn băng thông và không lập trình được shader.
- **Modern OpenGL 3.3 Core** nạp dữ liệu hình học một lần lên bộ nhớ GPU (`GL_STATIC_DRAW`) và vẽ lại với chi phí rất thấp:
  - **VBO (Vertex Buffer Object)**: Mảng byte liên tục trên GPU chứa dữ liệu đỉnh (`std::vector<Vertex>`).
  - **EBO / IBO (Element Buffer Object)**: Mảng chỉ mục nguyên không dấu (`uint32_t`) tham chiếu vào VBO, giúp tái sử dụng đỉnh chung giữa các tam giác và vẽ bằng `glDrawElements`.
  - **VAO (Vertex Array Object)**: Đối tượng lưu toàn bộ cấu hình đọc VBO (`glVertexAttribPointer`) và liên kết EBO. Khi vẽ chỉ cần `glBindVertexArray(vao)` + `glDrawElements(...)`.

### 1.2. Interleaved Vertex Layout với `sizeof` và `offsetof`
Mỗi đỉnh `roomcraft::graphics::Vertex` có cấu trúc liên tục trong bộ nhớ (32 byte/đỉnh):

| Attribute `location` | Trường dữ liệu | Kiểu GLSL | Số float | Offset (byte) |
|---|---|---|---|---|
| `0` | `position` | `vec3` | 3 | `offsetof(Vertex, position)` = 0 |
| `1` | `normal` | `vec3` | 3 | `offsetof(Vertex, normal)` = 12 |
| `2` | `texCoord` | `vec2` | 2 | `offsetof(Vertex, texCoord)` = 24 |

Việc dùng `sizeof(Vertex)` làm stride và `offsetof(...)` làm offset (kết hợp `static_assert(std::is_standard_layout_v<Vertex>)`) đảm bảo an toàn ngay cả khi compiler chèn padding.

---

## 2. Biên dịch và liên kết Shader (GLSL Compilation Pipeline)

Một chương trình shader (`roomcraft::graphics::Shader`) đi qua 4 bước bắt buộc:
1. **Đọc mã nguồn GLSL** (`#version 330 core`) từ đường dẫn tương đối (`assets/shaders/*.vert`, `*.frag`).
2. **Biên dịch từng giai đoạn** (`glCreateShader`, `glShaderSource`, `glCompileShader`) và kiểm tra `GL_COMPILE_STATUS`. Nếu lỗi cú pháp, đọc chi tiết từ `glGetShaderInfoLog` và hủy shader ngay lập tức.
3. **Liên kết chương trình** (`glCreateProgram`, `glAttachShader`, `glLinkProgram`) và kiểm tra `GL_LINK_STATUS`.
4. **Giải phóng đối tượng shader trung gian** (`glDeleteShader`) sau khi link xong; giữ lại `m_programId` và cache vị trí biến uniform (`glGetUniformLocation`) để tránh truy vấn driver lặp lại mỗi khung hình.

---

## 3. Tọa độ thuần nhất (Homogeneous Coordinates) & Hệ biến đổi MVP

### 3.1. Tọa độ thuần nhất (Homogeneous Coordinates)
Điểm 3D $(x, y, z)$ được biểu diễn dưới dạng vector cột 4 chiều $(x, y, z, 1)^T$, còn vector hướng (như pháp tuyến) có dạng $(x, y, z, 0)^T$. Biểu diễn 4D cho phép gộp phép **tịnh tiến (Translation)** — vốn không tuyến tính trong 3D — cùng với phép **xoay (Rotation)**, **tỷ lệ (Scale)** và **chiếu phối cảnh (Perspective Projection)** thành phép nhân ma trận $4 \times 4$ thống nhất.

### 3.2. Chuỗi biến đổi Model – View – Projection (MVP)

$$p_{\text{clip}} = P \cdot V \cdot M \cdot p_{\text{local}}$$

1. **Model Matrix ($M$)** — `Transform::getModelMatrix()`:
   Chuyển đỉnh từ không gian cục bộ (Local Space) sang không gian thế giới (World Space, hệ tọa độ tay phải, `+Y` hướng lên, đơn vị mét):
   $$M = T \cdot R_y(\text{yaw}) \cdot R_x(\text{pitch}) \cdot R_z(\text{roll}) \cdot S$$
   Vì sử dụng quy ước **vector cột**, phép biến đổi áp dụng lên $p_{\text{local}}$ theo thứ tự từ phải sang trái: **Scale → Roll (Z) → Pitch (X) → Yaw (Y) → Translate**.
2. **View Matrix ($V$)** — `Camera::getViewMatrix()`:
   Tính bằng `glm::lookAt(position, target, up)`, biến đổi toàn bộ thế giới về hệ tọa độ gắn với camera (camera nằm tại gốc tọa độ, nhìn dọc theo trục $-Z$).
3. **Projection Matrix ($P$)** — `Camera::getProjectionMatrix()`:
   - **Phối cảnh (`glm::perspective`)**: Tạo hình chóp cụt nhìn (view frustum); thành phần $w_{\text{clip}} = -z_{\text{view}}$ tăng theo khoảng cách tới camera. Sau đó phần cứng thực hiện **phép chia phối cảnh (Perspective Division)**:
     $$p_{\text{NDC}} = \left(\frac{x_{\text{clip}}}{w_{\text{clip}}}, \frac{y_{\text{clip}}}{w_{\text{clip}}}, \frac{z_{\text{clip}}}{w_{\text{clip}}}\right) \in [-1, 1]^3$$
     nhờ đó vật ở xa tự động thu nhỏ (foreshortening).
   - **Trực giao (`glm::ortho`)**: Tạo hình hộp chiếu song song, $w_{\text{clip}} = 1$, bảo toàn kích thước không phụ thuộc khoảng cách — rất phù hợp cho bản vẽ mặt bằng 2D (Top View) và mặt đứng (Front View).

---

## 4. Depth Buffer & Depth Testing (Z-Buffer)

Khi nhiều vật thể 3D chồng lên nhau trên cùng một pixel màn hình:
- **Nếu tắt Depth Test**: Vật thể vẽ sau luôn ghi đè màu lên vật thể vẽ trước (*Painter's Algorithm* thất bại khi thứ tự vẽ thay đổi hoặc các đa giác cắt nhau).
- **Khi bật Depth Test (`glEnable(GL_DEPTH_TEST)`, `glDepthFunc(GL_LESS)`)**:
  1. Mỗi khung hình, `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)` đặt toàn bộ bộ đệm độ sâu về `1.0` (xa nhất).
  2. Với mỗi fragment, GPU so sánh độ sâu $z_{\text{frag}} \in [0, 1]$ với giá trị đang lưu trong Depth Buffer.
  3. Chỉ khi $z_{\text{frag}} < z_{\text{buffer}}$ (gần camera hơn), màu fragment mới được ghi vào Color Buffer và Depth Buffer được cập nhật.
  4. Nhờ vậy kết quả che khuất **độc lập với thứ tự gọi draw call** (đã được kiểm chứng tự động ở `TC-M1-11`).

---

## 5. Framebuffer (Off-screen Rendering)

Mặc định OpenGL vẽ thẳng vào *Default Framebuffer* (màn hình). Để hiển thị khung nhìn 3D bên trong một panel giao diện Dear ImGui (nền tảng cho Editor ở M6), RoomCraft 3D sử dụng **Framebuffer Object (FBO)**:
- **Color Attachment (`GL_COLOR_ATTACHMENT0`)**: Một texture 2D `GL_RGBA8` lưu ảnh màu kết quả.
- **Depth-Stencil Attachment (`GL_DEPTH_STENCIL_ATTACHMENT`)**: Một Renderbuffer `GL_DEPTH24_STENCIL8` cung cấp Z-buffer cho cảnh vẽ off-screen.
- **Completeness Validation**: Sau khi gắn attachment, luôn kiểm tra `glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE`.
- **Lật trục tọa độ UV khi hiển thị trong ImGui**: OpenGL đặt gốc texture `(0, 0)` ở **góc dưới-trái**, trong khi Dear ImGui đặt gốc ảnh `(0, 0)` ở **góc trên-trái**. Do đó khi gọi `ImGui::Image(...)` cần truyền `uv0 = (0, 1)` và `uv1 = (1, 0)` để hình ảnh không bị lộn ngược.

---

## 6. RAII và vòng đời tài nguyên OpenGL

Tài nguyên GPU (`VAO`, `VBO`, `EBO`, `Shader Program`, `FBO`, `Texture`, `RBO`) là các handle số nguyên gắn chặt với OpenGL context:
- Áp dụng **RAII** kết hợp quy tắc **Non-copyable, Movable** (`= delete` cho copy constructor/assignment; dùng `std::exchange(other.handle, 0)` cho move) để mỗi handle luôn chỉ có **một chủ sở hữu duy nhất**, ngăn chặn lỗi giải phóng trùng (double-delete) hoặc giải phóng sớm khi bản sao tạm bị hủy.
- Với **FreeGLUT**, `glutMainLoop()` mặc định có thể hủy OpenGL context trước khi hàm trả về. RoomCraft 3D thiết lập `GLUT_ACTION_GLUTMAINLOOP_RETURNS` và gọi `shutdownGraphics()` ngay trong `glutCloseFunc` **trước khi context bị phá hủy**, đảm bảo mọi lời gọi `glDelete*` và `ImGui_ImplOpenGL3_Shutdown()` đều hợp lệ.
