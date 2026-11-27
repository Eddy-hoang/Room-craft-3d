# Phạm vi dự án — RoomCraft 3D (Project Scope)

## 1. Mục tiêu sản phẩm

**RoomCraft 3D** là ứng dụng desktop thiết kế và bố trí nội thất 3D chạy trên Windows x64, được phát triển bằng **C++17**, **Modern OpenGL 3.3 Core Profile**, **FreeGLUT**, **GLAD**, **GLSL** và **Dear ImGui**.

Dự án cân bằng giữa:
- Kiến thức học thuật Computer Graphics (pipeline đồ họa hiện đại, biến đổi ma trận, sinh hình học thủ tục, chiếu sáng Phong/Blinn-Phong, Ray-AABB picking, kiểm tra va chạm AABB).
- Kiến trúc phần mềm hướng đối tượng, tách lớp rõ ràng, dễ bảo trì và giải thích khi bảo vệ đồ án.
- Chất lượng thẩm mỹ thực tế thông qua phương án **Hybrid Modeling** (kết hợp Procedural Modeling bằng C++ và Asset-based Modeling bằng Blender + Assimp).

## 2. Công nghệ bắt buộc (Locked Stack)

| Thành phần | Công nghệ | Mức độ |
|---|---|---|
| Ngôn ngữ | C++17 | Bắt buộc |
| Graphics API | OpenGL 3.3 Core Profile | Bắt buộc |
| Window & Event Loop | FreeGLUT | Bắt buộc |
| Shader Language | GLSL `#version 330 core` | Bắt buộc |
| OpenGL Loader | GLAD | Bắt buộc |
| Toán học | GLM (từ M1) | Bắt buộc |
| Modeling 3D ngoài | Blender (từ M3) | Bắt buộc |
| Import Model | Assimp (từ M3) | Bắt buộc |
| Giao diện | Dear ImGui (`imgui_impl_glut` + `imgui_impl_opengl3`) | Bắt buộc |
| Đọc Texture | stb_image (từ M7) | Bắt buộc |
| Serialization | nlohmann/json (từ M8) | Bắt buộc |
| Build System | CMake | Bắt buộc |
| Dependency Management | vcpkg / FetchContent khóa commit | Khuyến nghị |
| Unit Testing | Catch2 / CTest | Khuyến nghị |

## 3. Phân loại ưu tiên chức năng

### P0 — Bắt buộc (Sản phẩm cốt lõi)
- [ ] Nền tảng OpenGL 3.3 Core + FreeGLUT + GLAD + Dear ImGui (M0)
- [ ] Core Graphics Engine: Shader, Mesh, Camera, Transform, Framebuffer (M1)
- [ ] Procedural Modeling: Plane, Cube/Cuboid, Cylinder, Sphere, Cone/Frustum, Bézier Curve/Surface, Surface of Revolution + ≥ 5 loại nội thất tự sinh (M2)
- [ ] Blender Asset Pipeline qua Assimp (OBJ/MTL) + ≥ 8 mô hình nội thất (M3)
- [ ] Room Editor & Scene Management (M4)
- [ ] Furniture Library (M4/M6)
- [ ] Transform System: Translation, Rotation, Scale (M5)
- [ ] Camera 2D Top View & 3D Perspective (M5)
- [ ] Object Picking bằng Ray-AABB Intersection (M5)
- [ ] Giao diện Editor chuyên nghiệp bằng Dear ImGui (M6)
- [ ] Phong/Blinn-Phong Lighting (Ambient, Directional, Point) (M7)
- [ ] Texture Mapping & Material Properties (M7)
- [ ] Collision Detection (World-space AABB + biên phòng) (M8)
- [ ] Project Persistence (Save/Load JSON có `schemaVersion`) (M8)

### P1 — Nên có (Hoàn thiện trải nghiệm)
- [ ] Grid Snapping
- [ ] Isometric View
- [ ] Furniture Search
- [ ] Undo/Redo (Command Pattern)
- [ ] Transform Gizmo (ImGuizmo)
- [ ] Day/Night Lighting Mode
- [ ] Cửa ra vào và cửa sổ

### P2 — Mở rộng (Chỉ triển khai khi P0 và P1 ổn định)
- [ ] Shadow Mapping
- [ ] First-person Walkthrough
- [ ] Normal Mapping
- [ ] OBB Collision
- [ ] Xuất ảnh bản thiết kế
- [ ] Công cụ đo khoảng cách
- [ ] Thiết kế phòng đa giác

## 4. Ngoài phạm vi hiện tại

- Không dùng Game Engine thương mại (Unity, Unreal Engine, Godot) hoặc thư viện WebGL/Three.js.
- Không thay thế FreeGLUT bằng GLFW.
- Không đầu tư sớm vào PBR/Ray Tracing phức tạp trước khi hoàn thành bộ P0.
