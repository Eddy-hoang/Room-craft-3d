#pragma once

#include <cstdint>

namespace roomcraft::scene {

// Định danh ổn định (Stable Object ID) cho từng SceneObject trong phạm vi một Scene.
// Không dùng chỉ mục vector (index) làm ID để tránh thay đổi định danh khi xóa/sắp xếp đối tượng.
// Giá trị 0 (kInvalidObjectId) đại diện cho trạng thái không hợp lệ / không có lựa chọn.
using ObjectId = std::uint64_t;

inline constexpr ObjectId kInvalidObjectId = 0;

inline constexpr bool isValidObjectId(ObjectId id) noexcept
{
    return id != kInvalidObjectId;
}

} // namespace roomcraft::scene
