# Hướng dẫn sử dụng Blender & Sinh Asset — RoomCraft 3D (Blender Guide)

Tài liệu này hướng dẫn quy chuẩn tạo, chỉnh sửa và xuất mô hình nội thất bằng **Blender** cho dự án **RoomCraft 3D** (Milestone M3).

---

## 1. Môi trường Blender đã kiểm chứng

- **Phiên bản**: `Blender 5.2.3 LTS` (Windows x64)
- **Định dạng nguồn**: `.blend` lưu tại `blender/source/`
- **Script tự động (`bpy`)**: lưu tại `blender/scripts/`
- **Định dạng xuất cho OpenGL/Assimp**: Wavefront `.obj` + `.mtl` + `.png` lưu tại `assets/models/<ten_noi_that>/`

## 2. Phong cách & Quy chuẩn thiết kế nội thất

- **Phong cách**: *Stylized Low-poly / Scandinavian Modern* — hình khối sạch sẽ, cạnh vát nhẹ (Bevel), tông màu gỗ sồi ấm, vải be/xám trung tính và điểm nhấn xanh dịu.
- **Đơn vị thế giới**: **Mét (`1 Blender Unit = 1 m`)**.
- **Hệ trục trong Blender Workspace**: `+Z` hướng lên, mặt sàn tại `Z = 0`, mặt trước đồ nội thất hướng về `-Y` (góc nhìn Front `Numpad 1`).
- **Quy chuẩn xuất Wavefront OBJ (`bpy.ops.wm.obj_export`)**:

| Thuộc tính | Giá trị bắt buộc | Ý nghĩa |
|---|---|---|
| `global_scale` | `1.0` | Giữ nguyên kích thước mét |
| `up_axis` | `'Y'` | Chuyển trục đứng từ `+Z` (Blender) → `+Y` (OpenGL) |
| `forward_axis` | `'NEGATIVE_Z'` | Mặt trước `-Y` trong Blender → `+Z` hướng về camera OpenGL |
| `apply_modifiers` | `True` | Nướng Bevel/Solidify trước khi xuất |
| `export_triangulated_mesh` | `True` | Tam giác hóa toàn bộ bề mặt |
| `export_uv` | `True` | Xuất tọa độ texture UV |
| `export_normals` | `True` | Xuất vector pháp tuyến |
| `export_materials` | `True` | Xuất file `.mtl` đi kèm |
| `path_mode` | `'RELATIVE'` | Đường dẫn texture tương đối (không hardcode ổ đĩa cá nhân) |

## 3. Chạy tự động toàn bộ 8 script sinh tài sản

Từ PowerShell tại thư mục gốc `D:\CG`:

```powershell
# Thay đường dẫn blender.exe theo máy của bạn (hoặc dùng "blender" nếu đã thêm vào PATH)
& "D:\blender\Blender-5.2.3\Blender Foundation\Blender 5.2\blender.exe" `
    --background --factory-startup `
    --python "d:\CG\blender\scripts\generate_all.py"
```

Chạy riêng từng mô hình (ví dụ chỉ dựng lại Modern Sofa):
```powershell
& "D:\blender\Blender-5.2.3\Blender Foundation\Blender 5.2\blender.exe" `
    --background --factory-startup `
    --python "d:\CG\blender\scripts\create_sofa.py"
```

## 4. Chỉnh sửa thủ công mô hình bằng Blender GUI

1. Mở trực tiếp file `.blend` tương ứng trong `blender/source/` (ví dụ `blender/source/modern_sofa.blend`).
2. Chỉnh sửa hình học, bevel, UV hoặc vật liệu theo ý muốn.
3. Xuất lại **File → Export → Wavefront (.obj)** vào đúng thư mục `assets/models/<folder>/` với cấu hình mục 2 (Up: `Y`, Forward: `-Z`, Scale: `1.0`, Triangulated Mesh: bật, Path Mode: Relative).
4. Trong ứng dụng **RoomCraft 3D (Asset Browser)**, nhấn **`Reload Asset`** để tải lại ngay lập tức mà không cần khởi động lại chương trình!
