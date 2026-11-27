# Quản lý nguồn gốc và giấy phép tài nguyên — RoomCraft 3D (Asset Licenses)

Tài liệu này theo dõi bản quyền, tác giả và giấy phép của mọi thư viện bên thứ ba, mô hình 3D, texture, icon và font chữ sử dụng trong dự án.

## 1. Thư viện mã nguồn mở (Dependencies — Milestone M0–M3)

| Thư viện | Phiên bản / Commit khóa | Giấy phép | Nguồn chính thức | Mục đích |
|---|---|---|---|---|
| **FreeGLUT** | `v3.8.0` (`3db1649ce1f5e42f1338b51e3fa14849be547d5d`) | X-Consortium (MIT-like) | https://github.com/FreeGLUTProject/freeglut | Quản lý cửa sổ và vòng lặp sự kiện (M0) |
| **GLAD** | `v0.1.36` (`1ecd45775d96f35170458e6b148eb0708967e402`) | MIT (generator) / Public Domain (generated code) | https://github.com/Dav1dde/glad | OpenGL 3.3 Core Function Loader (M0) |
| **Dear ImGui** | `v1.93.0-docking` (`b916186e66aa3c02b72c6ca17455516512ed8049`) | MIT | https://github.com/ocornut/imgui | Giao diện người dùng đồ họa `imgui_impl_glut` + `imgui_impl_opengl3` (M0) |
| **GLM** | `1.0.1` (`0af55ccecd98d4e5a8d1fad7de25ba429d60e863`) | Happy Bunny License / MIT | https://github.com/g-truc/glm | Toán học vector, ma trận 3D chuẩn OpenGL (M1) |
| **Assimp** | `v6.0.6` (`a79ff6bbce54d3e792691ef4b72ae26434c3e799`) | BSD 3-Clause | https://github.com/assimp/assimp | Import mô hình 3D `.obj`/`.mtl` thuần CPU (M3) |
| **stb (`stb_image.h`)** | Commit `6b5799bae8a1fe3792691ef4b72ae26434c3e799` | MIT / Public Domain | https://github.com/nothings/stb | Giải mã ảnh texture `.png`/`.jpg` (M3) |

## 2. Mô hình 3D (Blender Assets — Milestone M3)

Toàn bộ 8 mô hình nội thất dưới đây được thiết kế gốc cho dự án **RoomCraft 3D** bằng kịch bản Python (`bpy`) chạy trên **Blender 5.2.3 LTS** (không tải lậu hay sử dụng tài sản có bản quyền thương mại của bên thứ ba). Tất cả mô hình và file `.blend` gốc được phát hành cùng dự án theo giấy phép **MIT**.

| Asset ID | Tên hiển thị | Tác giả / Nguồn | Script `bpy` | File `.blend` nguồn | File xuất `.obj` | Giấy phép |
|---|---|---|---|---|---|---|
| `blender-modern-sofa` | Modern Sofa | RoomCraft 3D (tự sinh bằng `bpy`) | `blender/scripts/create_sofa.py` | `blender/source/modern_sofa.blend` | `assets/models/sofa/modern_sofa.obj` | MIT |
| `blender-modern-bed` | Modern Bed | RoomCraft 3D (tự sinh bằng `bpy`) | `blender/scripts/create_bed.py` | `blender/source/modern_bed.blend` | `assets/models/bed/modern_bed.obj` | MIT |
| `blender-gaming-chair` | Gaming Chair | RoomCraft 3D (tự sinh bằng `bpy`) | `blender/scripts/create_gaming_chair.py` | `blender/source/gaming_chair.blend` | `assets/models/gaming_chair/gaming_chair.obj` | MIT |
| `blender-computer-setup` | Computer Setup | RoomCraft 3D (tự sinh bằng `bpy`) | `blender/scripts/create_computer.py` | `blender/source/computer_setup.blend` | `assets/models/computer/computer_setup.obj` | MIT |
| `blender-indoor-plant` | Indoor Plant | RoomCraft 3D (tự sinh bằng `bpy`) | `blender/scripts/create_plant.py` | `blender/source/indoor_plant.blend` | `assets/models/plant/indoor_plant.obj` | MIT |
| `blender-floor-lamp` | Floor Lamp | RoomCraft 3D (tự sinh bằng `bpy`) | `blender/scripts/create_floor_lamp.py` | `blender/source/floor_lamp.blend` | `assets/models/floor_lamp/floor_lamp.obj` | MIT |
| `blender-wardrobe` | Wardrobe | RoomCraft 3D (tự sinh bằng `bpy`) | `blender/scripts/create_wardrobe.py` | `blender/source/wardrobe.blend` | `assets/models/wardrobe/wardrobe.obj` | MIT |
| `blender-carpet` | Minimalist Carpet | RoomCraft 3D (tự sinh bằng `bpy`) | `blender/scripts/create_carpet.py` | `blender/source/minimalist_carpet.blend` | `assets/models/carpet/minimalist_carpet.obj` | MIT |

## 3. Texture, Test Fixtures, Icon và Font

- **11 file texture `.png` đi kèm 8 mô hình Blender** (`assets/models/*/*.png`): Được sinh bằng thuật toán từ `blender/scripts/common.py` (vân gỗ, vân vải, họa tiết thảm, mặt màn hình, chao đèn), thuộc quyền sở hữu của dự án **RoomCraft 3D** theo giấy phép **MIT**.
- **Ảnh fixture kiểm thử `tests/fixtures/asymmetric_uv.png`**: Tự sinh phục vụ `TC-M3-17` (nửa trên Đỏ, nửa dưới Xanh dương), giấy phép **MIT**.
- **Icon & Font bên thứ ba**: Chưa sử dụng font/icon ngoài tại Milestone M3 (dùng font mặc định của Dear ImGui; sẽ cập nhật nếu bổ sung ở Milestone M6).
