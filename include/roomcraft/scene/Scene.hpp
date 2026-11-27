#pragma once

#include "roomcraft/assets/FurnitureCatalog.hpp"
#include "roomcraft/math/BoundingBox.hpp"
#include "roomcraft/scene/ObjectId.hpp"
#include "roomcraft/scene/Room.hpp"
#include "roomcraft/scene/SceneObject.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace roomcraft::scene {

// Lớp Scene quản lý toàn bộ dữ liệu cảnh 3D thuần CPU: một Room và danh sách các SceneObject.
//
// Chiến lược Stable Object ID:
// - m_nextObjectId khởi đầu từ 1 và tăng đơn điệu cho mỗi lần addObject / duplicateObject.
// - Xóa đối tượng không làm thay đổi ID của các đối tượng còn lại và không tái sử dụng ID đã xóa
//   trong cùng phiên scene (kể cả sau clearObjects), đảm bảo an toàn cho SelectionState, Hierarchy,
//   Undo/Redo và Serialization về sau.
class Scene {
public:
    Scene() = default;

    Room& getRoom() { return m_room; }
    const Room& getRoom() const { return m_room; }

    // Kiểm tra Transform hợp lệ (position/rotation hữu hạn, scale dương và hữu hạn)
    static bool isValidTransform(const Transform& transform, std::string* outError = nullptr);

    // Chuẩn hóa tên hiển thị (cắt khoảng trắng đầu/cuối), trả về chuỗi rỗng nếu không hợp lệ
    static std::string normalizeObjectName(const std::string& rawName);

    // Thêm một SceneObject mới tham chiếu assetId tồn tại trong FurnitureCatalog
    ObjectId addObject(const std::string& assetId,
                       const std::string& displayName,
                       const Transform& transform,
                       const assets::FurnitureCatalog& catalog,
                       std::string* outError = nullptr);

    // Xóa đối tượng theo ObjectId (trả về false nếu ID không tồn tại, không crash)
    bool removeObject(ObjectId id);

    // Nhân bản một đối tượng hiện có: tạo ObjectId mới, tên mới phân biệt và Transform cộng offset
    ObjectId duplicateObject(ObjectId id,
                             const glm::vec3& offset = glm::vec3(0.4f, 0.0f, 0.4f),
                             std::string* outError = nullptr);

    SceneObject* findObject(ObjectId id);
    const SceneObject* findObject(ObjectId id) const;

    bool renameObject(ObjectId id, const std::string& newName, std::string* outError = nullptr);
    bool setObjectVisibility(ObjectId id, bool visible);
    bool setObjectTransform(ObjectId id, const Transform& newTransform, std::string* outError = nullptr);

    const std::vector<SceneObject>& getObjects() const { return m_objects; }
    std::size_t getObjectCount() const { return m_objects.size(); }
    std::size_t getVisibleObjectCount() const;

    void clearObjects();

    // Kiểm tra sơ bộ xem hộp bao thế giới của đối tượng có nằm trong phạm vi phòng hay không
    // (chỉ phục vụ cảnh báo trực quan ở M4, KHÔNG phải CollisionSystem đầy đủ của M8)
    bool isBoundsWithinRoom(const math::BoundingBox& worldBounds, float tolerance = 1e-3f) const;

private:
    std::string generateDefaultName(const std::string& baseName) const;

private:
    Room m_room;
    std::vector<SceneObject> m_objects;
    ObjectId m_nextObjectId = 1;
};

} // namespace roomcraft::scene
