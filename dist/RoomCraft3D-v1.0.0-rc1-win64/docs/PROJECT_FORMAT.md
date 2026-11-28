# Đặc tả Định dạng Tệp Dự án — `.roomcraft` (Project Format Specification — M8)

Tài liệu này đặc tả chính thức định dạng tệp dự án **`.roomcraft`** (JSON tuân thủ RFC 8259, UTF-8) được sử dụng từ **Milestone M8 (`v0.8.0`)**.

---

## 1. Nguyên tắc Thiết kế & Phiên bản Schema

1. **Độc lập giữa `schemaVersion` và phiên bản ứng dụng**:
   - `format`: luôn là `"RoomCraft3D"`.
   - `schemaVersion`: hiện tại là **`1`** (số nguyên), độc lập với phiên bản ứng dụng (`0.8.0`).
   - Phiên bản không được hỗ trợ (ví dụ `0`, `2`, `99`) bị từ chối rõ ràng với thông báo hành động được, **không âm thầm bỏ qua dữ liệu**.
2. **Không lưu dữ liệu phụ thuộc GPU hoặc máy cá nhân**:
   - **KHÔNG** lưu `VAO`, `VBO`, `EBO`, OpenGL Texture ID, con trỏ bộ nhớ, hay đường dẫn tuyệt đối (`D:\...`).
   - **KHÔNG** nhúng dữ liệu mesh hay texture vào file dự án; mọi đồ nội thất tham chiếu bằng **Stable Asset ID** trong `FurnitureCatalog` (ví dụ `"blender-modern-sofa"`, `"procedural-desk"`).
3. **Bảo toàn 100% độ chính xác `uint64_t` cho `ObjectId`**:
   - JSON số học dùng IEEE-754 double (chỉ chính xác tuyệt đối tới $2^{53}$). Để tránh mất độ chính xác, `objectId` và `nextObjectId` được serialize thành **chuỗi thập phân chuẩn** (`"1"`, `"9007199254740993"`). Bộ đọc xác thực nghiêm ngặt chuỗi số nguyên không âm 64-bit và chống tràn số `UINT64_MAX`.
4. **Đơn vị thống nhất**:
   - Độ dài / tọa độ: **mét (m)**, hệ tọa độ tay phải, `+Y` hướng lên, tâm sàn tại `(0, 0, 0)`.
   - Góc quay (`rotationDegrees`): **độ (degrees)** theo thứ tự Euler `Y -> X -> Z` của `scene::Transform`.

---

## 2. Cấu trúc JSON Schema v1

```json
{
  "format": "RoomCraft3D",
  "schemaVersion": 1,
  "projectName": "Modern Bedroom",
  "nextObjectId": "8",
  "room": {
    "width": 5.0,
    "length": 6.0,
    "height": 3.0,
    "wallThickness": 0.15,
    "floorThickness": 0.1,
    "gridSpacing": 0.5,
    "floorColor": [0.76, 0.67, 0.55, 1.0],
    "wallColor": [0.89, 0.88, 0.85, 1.0],
    "ceilingColor": [0.93, 0.93, 0.91, 1.0],
    "wallVisibilityMode": "CutawayView",
    "showNorthWall": true,
    "showSouthWall": true,
    "showEastWall": true,
    "showWestWall": true,
    "showCeiling": false,
    "showGrid": true
  },
  "objects": [
    {
      "objectId": "1",
      "assetId": "blender-modern-bed",
      "name": "Modern Bed",
      "position": [0.0, 0.0, -1.5],
      "rotationDegrees": [0.0, 90.0, 0.0],
      "scale": [1.0, 1.0, 1.0],
      "visible": true,
      "locked": false,
      "materialOverrides": {
        "shininessScale": 1.25
      }
    }
  ],
  "lighting": {
    "preset": "Daylight",
    "ambient": 0.42,
    "sunEnabled": true,
    "sunDirection": [-0.6, -1.0, -0.45],
    "sunColor": [1.0, 0.95, 0.86],
    "sunIntensity": 0.72,
    "pointEnabled": false,
    "pointPosition": [-1.0, 2.3, 0.7],
    "pointColor": [1.0, 0.68, 0.42],
    "pointIntensity": 1.3,
    "pointAttenuation": [1.0, 0.18, 0.12],
    "shadowsEnabled": true,
    "shadowMapSize": 1024,
    "shadowBias": 0.0022
  },
  "renderSettings": {
    "msaa": "Off",
    "wireframe": false,
    "showAxes": true,
    "showSelectedBBox": true,
    "showMoveGizmo": true
  },
  "camera": {
    "projectionMode": "Perspective",
    "position": [4.5, 3.6, 5.5],
    "target": [0.0, 0.4, 0.0],
    "up": [0.0, 1.0, 0.0],
    "fovDegrees": 45.0,
    "nearPlane": 0.05,
    "farPlane": 100.0,
    "orthoHalfHeight": 3.5
  },
  "editorSettings": {
    "showGrid": true,
    "gridSnapEnabled": false,
    "placementValidationMode": "WarningOnly",
    "allowRugOverlap": true
  }
}
```

---

## 3. Quy tắc Xác thực & Chính sách Tương thích

### Giới hạn & Ràng buộc Xác thực (`ProjectValidator`)
- Kích thước tệp tối đa: **`16 MB`**, độ sâu lồng JSON tối đa: **`64`**, số đối tượng tối đa: **`5,000`**.
- Từ chối mọi giá trị không hữu hạn (`NaN`/`Inf`), sai kiểu JSON, thiếu trường bắt buộc hoặc ký tự thừa sau tài liệu JSON.
- `room`: kích thước nằm trong giới hạn `room_limits` (`width/length ∈ [2, 20] m`, `height ∈ [2, 6] m`, `gridSpacing ∈ [0.1, 2.0] m`), màu RGBA trong `[0, 1]`.
- `objects`: `objectId >= 1` duy nhất (không trùng), `nextObjectId > max(objectId)`, `assetId` tồn tại trong `FurnitureCatalog` (nếu thiếu asset, từ chối nạp kèm thông báo nêu rõ `assetId` bị thiếu), `scale > 0` hữu hạn, `shininessScale ∈ [0.05, 20.0]`.
- `lighting`: `ambient ∈ [0, 2]`, `sunDirection != 0`, `sunIntensity ∈ [0, 10]`, `pointIntensity ∈ [0, 20]`, `pointAttenuation.constant > 0`.
- `camera`: kiểm tra dựng thực trên `graphics::Camera` (`position != target`, `fov ∈ [10, 120]`, `near > 0`, `far > near`, `orthoHalfHeight > 0`).

### Chính sách Tiến hóa Schema (Forward-Compatibility & Migration)
- Schema v1 không cho phép các bản ứng dụng cũ mở tệp schema cao hơn (`schemaVersion > 1`) để tránh mất dữ liệu khi lưu đè lại.
- Các trường mở rộng M9.4 (`shadowsEnabled`, `shadowMapSize` ∈ `{512, 1024, 2048}`, `shadowBias` ∈ `[0.0, 0.05]`) là **tùy chọn và tương thích ngược hoàn toàn** với các tệp `.roomcraft` schema v1 được tạo từ M8.
- Tùy chọn bố cục giao diện cá nhân (`leftPanelWidth`, `rightPanelWidth`, `hierarchyHeightRatio`, `uiScale`, `libraryGridView`, `recentProjects`) được lưu **tách biệt** trong `%APPDATA%\RoomCraft3D\preferences.json` (`PreferencesManager`) và không lẫn vào tệp dự án `.roomcraft`.
