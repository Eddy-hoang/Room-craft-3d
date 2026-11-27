# Blender Assets — RoomCraft 3D

Thư mục này chứa mã nguồn Python API (`bpy`) và các file dự án `.blend` dùng để tạo và chỉnh sửa 8 mô hình nội thất theo phong cách *Stylized Low-poly / Scandinavian Modern* cho **Milestone M3**.

## Cấu trúc thư mục

- `scripts/`:
  - `common.py` — Tiện ích dùng chung: tạo hình học có bevel, gán box/planar UV tương thích chế độ `--background`, sinh texture PNG thủ tục, lưu `.blend` và xuất `.obj/.mtl` theo chuẩn Y-up / Forward `-Z`.
  - `create_sofa.py` → `assets/models/sofa/modern_sofa.obj`
  - `create_bed.py` → `assets/models/bed/modern_bed.obj`
  - `create_gaming_chair.py` → `assets/models/gaming_chair/gaming_chair.obj`
  - `create_computer.py` → `assets/models/computer/computer_setup.obj`
  - `create_plant.py` → `assets/models/plant/indoor_plant.obj`
  - `create_floor_lamp.py` → `assets/models/floor_lamp/floor_lamp.obj`
  - `create_wardrobe.py` → `assets/models/wardrobe/wardrobe.obj`
  - `create_carpet.py` → `assets/models/carpet/minimalist_carpet.obj`
  - `generate_all.py` — Script chạy hàng loạt tạo đủ 8 tài sản và in thống kê tam giác thực tế.
- `source/` — Lưu 8 file `.blend` gốc để mở và chỉnh sửa trực tiếp bằng Blender GUI.

## Tái tạo toàn bộ tài sản bằng Blender CLI

Xem hướng dẫn chi tiết tại [`docs/BLENDER_GUIDE.md`](../docs/BLENDER_GUIDE.md):
```powershell
blender --background --factory-startup --python blender/scripts/generate_all.py
```

> **Lưu ý học thuật**: 8 mô hình Blender này thuộc phần *Asset-based Modeling* của chiến lược **Hybrid Modeling**, bổ trợ cho 6 mô hình *Procedural Modeling* tự sinh bằng thuật toán C++ tại Milestone M2 (`src/geometry/FurnitureGenerator.cpp`), không thay thế chúng.
