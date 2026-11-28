# AGENTS.md — Quy tắc phát triển RoomCraft 3D

Tài liệu này quy định các nguyên tắc bắt buộc cho mọi lập trình viên và AI coding agent khi tham gia phát triển dự án **RoomCraft 3D**.

## 1. Công nghệ bắt buộc (Locked Tech Stack)

- **Ngôn ngữ**: C++17 (`CMAKE_CXX_STANDARD 17`, `CMAKE_CXX_STANDARD_REQUIRED ON`).
- **Graphics API**: OpenGL 3.3 Core Profile.
  - Tuyệt đối **không** dùng `glBegin`/`glEnd`, fixed-function matrix stack (`glMatrixMode`, `glPushMatrix`) hoặc fixed-function lighting (`glLightfv`).
- **Window & Event Management**: FreeGLUT.
  - Không được tự ý chuyển sang GLFW, SDL2, Unity, Unreal Engine hoặc Three.js.
- **OpenGL Loader**: GLAD (Core Profile 3.3).
- **Shader Language**: GLSL `#version 330 core`.
- **GUI Framework**: Dear ImGui kết hợp `imgui_impl_glut` + `imgui_impl_opengl3` (không dùng `imgui_impl_opengl2`).
- **Build System**: CMake (+ `CMakePresets.json`).
- **Dependency Management**: Khóa phiên bản dependency rõ ràng (`vcpkg.json` có `builtin-baseline` và/hoặc `FetchContent` khóa tag/commit).
- **Target Platform**: Windows x64.

## 2. Quy tắc phát triển theo Milestone

1. Phát triển **từng milestone một** (`M0` → `M9`), không tự ý triển khai trước chức năng của milestone sau.
2. Không tạo hàng loạt class rỗng chỉ để lấp đầy cấu trúc thư mục tương lai.
3. Không sửa module ngoài phạm vi milestone hiện tại nếu không cần thiết.
4. Không tuyên bố hoàn thành khi chưa có bằng chứng thực tế (build, test, runtime).
5. Nếu không thể chạy test/runtime do thiếu môi trường, ghi rõ `NOT RUN`, `BLOCKED` hoặc `NOT VERIFIED`. Tuyệt đối không tạo log, FPS hoặc ảnh chụp giả.

## 3. Quy ước mã nguồn & Tài liệu

- Tên file, class, method, biến và commit message dùng **tiếng Anh**.
- Tài liệu (`docs/`, `README.md`) và chú thích giải thích thuật toán Computer Graphics/toán học trong mã nguồn dùng **tiếng Việt**.
- Không hardcode đường dẫn tuyệt đối phụ thuộc máy phát triển; luôn đọc tài nguyên bằng đường dẫn tương đối theo thư mục thực thi hoặc thư mục project.
- Quản lý tài nguyên OpenGL an toàn (RAII hoặc giải phóng tường minh **trước khi** OpenGL context bị hủy; chống double-delete).

## 4. Quy ước hệ tọa độ thống nhất (áp dụng từ M1 trở đi)

- Hệ tọa độ tay phải (Right-handed).
- Trục `Y` hướng lên trên (`+Y` = Up).
- Mặt sàn nằm trên mặt phẳng `XZ` (`Y = 0`).
- Đơn vị thế giới: **mét (m)**.
- Pivot mặc định của nội thất đặt tại **tâm đáy** (bottom center).
- Công thức biến đổi ma trận (vector cột): `Model = Translation * Rotation * Scale`.

## 5. Definition of Done (DoD)

Một tính năng hoặc milestone chỉ đạt trạng thái `DONE` khi:
- Mã nguồn triển khai đầy đủ đúng phạm vi.
- CMake configure và build thành công.
- Acceptance criteria và các test liên quan đã chạy và đạt (`PASS`).
- Runtime đã kiểm chứng (nếu phụ thuộc GUI/GPU).
- Hướng dẫn sử dụng và build được cập nhật khi quy trình thay đổi. Kết quả kiểm thử thực tế được ghi trong nội dung bàn giao.

Trạng thái công việc chuẩn: `TODO`, `IN_PROGRESS`, `BLOCKED`, `IMPLEMENTED`, `VERIFIED`, `DONE`.

## 6. Quy ước Git Commit

Sử dụng Conventional Commits ngắn gọn:
- `chore(core): initialize project structure`
- `build(cmake): configure project dependencies`
- `feat(renderer): implement shader abstraction`
- `fix(picking): correct viewport coordinates`
- `test(collision): cover AABB intersection`
- `docs(project): update milestone progress`
