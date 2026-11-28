#include "roomcraft/scene/Scene.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace roomcraft::scene {

namespace {

bool isFiniteVec3(const glm::vec3& v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

} // namespace

bool Scene::isValidTransform(const Transform& transform, std::string* outError)
{
    if (!isFiniteVec3(transform.position)) {
        if (outError != nullptr) *outError = "Transform position contains non-finite values.";
        return false;
    }
    if (!isFiniteVec3(transform.rotationDegrees)) {
        if (outError != nullptr) *outError = "Transform rotation contains non-finite values.";
        return false;
    }
    if (!Transform::isValidScale(transform.scale)) {
        if (outError != nullptr) *outError = "Transform scale must be finite and strictly positive.";
        return false;
    }
    return true;
}

std::string Scene::normalizeObjectName(const std::string& rawName)
{
    std::size_t start = 0;
    while (start < rawName.size() && std::isspace(static_cast<unsigned char>(rawName[start])) != 0) {
        ++start;
    }
    std::size_t end = rawName.size();
    while (end > start && std::isspace(static_cast<unsigned char>(rawName[end - 1])) != 0) {
        --end;
    }
    return rawName.substr(start, end - start);
}

std::string Scene::generateDefaultName(const std::string& baseName) const
{
    const std::string cleanBase = normalizeObjectName(baseName);
    const std::string root = cleanBase.empty() ? "Object" : cleanBase;

    int counter = 1;
    while (true) {
        const std::string candidate = root + " #" + std::to_string(counter);
        bool exists = false;
        for (const auto& obj : m_objects) {
            if (obj.name == candidate) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            return candidate;
        }
        ++counter;
    }
}

ObjectId Scene::addObject(const std::string& assetId,
                          const std::string& displayName,
                          const Transform& transform,
                          const assets::FurnitureCatalog& catalog,
                          std::string* outError)
{
    const assets::CatalogEntry* entry = catalog.findById(assetId);
    if (entry == nullptr) {
        if (outError != nullptr) *outError = "Unknown Asset ID: '" + assetId + "'.";
        return kInvalidObjectId;
    }

    if (!isValidTransform(transform, outError)) {
        return kInvalidObjectId;
    }

    std::string finalName = normalizeObjectName(displayName);
    if (finalName.empty()) {
        finalName = generateDefaultName(entry->displayName);
    }

    if (m_nextObjectId == std::numeric_limits<ObjectId>::max()) {
        if (outError != nullptr) *outError = "Object ID allocator overflow.";
        return kInvalidObjectId;
    }

    SceneObject obj;
    obj.id = m_nextObjectId++;
    obj.name = std::move(finalName);
    obj.assetId = assetId;
    obj.transform = transform;
    obj.visible = true;

    m_objects.push_back(std::move(obj));
    return m_objects.back().id;
}

bool Scene::removeObject(ObjectId id)
{
    if (!isValidObjectId(id)) {
        return false;
    }
    const auto it = std::find_if(m_objects.begin(), m_objects.end(), [id](const SceneObject& obj) {
        return obj.id == id;
    });
    if (it == m_objects.end()) {
        return false;
    }
    m_objects.erase(it);
    return true;
}

ObjectId Scene::duplicateObject(ObjectId id, const glm::vec3& offset, std::string* outError)
{
    const SceneObject* src = findObject(id);
    if (src == nullptr) {
        if (outError != nullptr) *outError = "Cannot duplicate: Object ID does not exist.";
        return kInvalidObjectId;
    }

    if (!isFiniteVec3(offset)) {
        if (outError != nullptr) *outError = "Duplicate offset must be finite.";
        return kInvalidObjectId;
    }

    if (m_nextObjectId == std::numeric_limits<ObjectId>::max()) {
        if (outError != nullptr) *outError = "Object ID allocator overflow.";
        return kInvalidObjectId;
    }

    SceneObject copy = *src;
    copy.id = m_nextObjectId++;
    copy.transform.position += offset;
    copy.name = generateDefaultName(src->name + " Copy");

    if (!isValidTransform(copy.transform, outError)) {
        return kInvalidObjectId;
    }

    m_objects.push_back(std::move(copy));
    return m_objects.back().id;
}

SceneObject* Scene::findObject(ObjectId id)
{
    if (!isValidObjectId(id)) {
        return nullptr;
    }
    for (auto& obj : m_objects) {
        if (obj.id == id) {
            return &obj;
        }
    }
    return nullptr;
}

const SceneObject* Scene::findObject(ObjectId id) const
{
    if (!isValidObjectId(id)) {
        return nullptr;
    }
    for (const auto& obj : m_objects) {
        if (obj.id == id) {
            return &obj;
        }
    }
    return nullptr;
}

bool Scene::renameObject(ObjectId id, const std::string& newName, std::string* outError)
{
    SceneObject* obj = findObject(id);
    if (obj == nullptr) {
        if (outError != nullptr) *outError = "Cannot rename: Object ID does not exist.";
        return false;
    }

    const std::string clean = normalizeObjectName(newName);
    if (clean.empty()) {
        if (outError != nullptr) *outError = "Object name cannot be empty.";
        return false;
    }

    obj->name = clean;
    return true;
}

bool Scene::setObjectVisibility(ObjectId id, bool visible)
{
    SceneObject* obj = findObject(id);
    if (obj == nullptr) {
        return false;
    }
    obj->visible = visible;
    return true;
}

bool Scene::setObjectTransform(ObjectId id, const Transform& newTransform, std::string* outError)
{
    SceneObject* obj = findObject(id);
    if (obj == nullptr) {
        if (outError != nullptr) *outError = "Cannot set transform: Object ID does not exist.";
        return false;
    }
    if (!isValidTransform(newTransform, outError)) {
        return false;
    }
    obj->transform = newTransform;
    return true;
}

std::size_t Scene::getVisibleObjectCount() const
{
    std::size_t count = 0;
    for (const auto& obj : m_objects) {
        if (obj.visible) {
            ++count;
        }
    }
    return count;
}

void Scene::clearObjects()
{
    m_objects.clear();
    // Giữ nguyên m_nextObjectId tăng đơn điệu để không bao giờ tái sử dụng ObjectId đã phát hành
}

bool Scene::restoreFromSnapshot(const RoomSettings& roomSettings,
                                const std::vector<SceneObject>& objects,
                                ObjectId nextObjectId,
                                const assets::FurnitureCatalog& catalog,
                                std::string* outError)
{
    // 1. Dựng Room ứng viên trước (không sửa m_room hiện tại nếu lỗi)
    Room candidateRoom;
    if (!candidateRoom.applySettings(roomSettings, outError)) {
        return false;
    }

    // 2. Xác thực toàn bộ danh sách đối tượng: ID hợp lệ, không trùng, Asset ID tồn tại, Transform hợp lệ
    std::vector<SceneObject> candidateObjects;
    candidateObjects.reserve(objects.size());
    std::unordered_set<ObjectId> seenIds;
    seenIds.reserve(objects.size());
    ObjectId maxId = 0;

    for (std::size_t i = 0; i < objects.size(); ++i) {
        const auto& src = objects[i];
        if (!isValidObjectId(src.id)) {
            if (outError != nullptr) {
                *outError = "Object at index " + std::to_string(i) + " has invalid ObjectId (0).";
            }
            return false;
        }
        if (src.id == std::numeric_limits<ObjectId>::max()) {
            if (outError != nullptr) {
                *outError = "Object at index " + std::to_string(i) + " has overflowing ObjectId.";
            }
            return false;
        }
        if (!seenIds.insert(src.id).second) {
            if (outError != nullptr) {
                *outError = "Duplicate ObjectId detected: #" + std::to_string(src.id) + ".";
            }
            return false;
        }
        if (catalog.findById(src.assetId) == nullptr) {
            if (outError != nullptr) {
                *outError = "Missing asset reference '" + src.assetId + "' for object #" + std::to_string(src.id) + ".";
            }
            return false;
        }
        if (!isValidTransform(src.transform, outError)) {
            return false;
        }
        if (!std::isfinite(src.shininessScale) || src.shininessScale < 0.05f || src.shininessScale > 20.0f) {
            if (outError != nullptr) {
                *outError = "Invalid shininessScale for object #" + std::to_string(src.id) + ".";
            }
            return false;
        }

        const std::string cleanName = normalizeObjectName(src.name);
        if (cleanName.empty()) {
            if (outError != nullptr) {
                *outError = "Empty object name for object #" + std::to_string(src.id) + ".";
            }
            return false;
        }

        SceneObject copy = src;
        copy.name = cleanName;
        candidateObjects.push_back(std::move(copy));
        maxId = std::max(maxId, src.id);
    }

    // 3. Kiểm tra và khôi phục bộ cấp phát ObjectId (bảo đảm ID mới sau này không trùng ID đã có)
    const ObjectId minValidNext = (maxId == 0) ? 1 : (maxId + 1);
    if (nextObjectId < minValidNext) {
        if (outError != nullptr) {
            *outError = "Invalid nextObjectId (" + std::to_string(nextObjectId) +
                        "): must be >= " + std::to_string(minValidNext) + ".";
        }
        return false;
    }

    // 4. Commit giao dịch: chỉ thay thế trạng thái Scene sau khi mọi bước xác thực đều thành công
    m_room = std::move(candidateRoom);
    m_objects = std::move(candidateObjects);
    m_nextObjectId = nextObjectId;
    return true;
}

bool Scene::isBoundsWithinRoom(const math::BoundingBox& worldBounds, float tolerance) const
{
    if (!worldBounds.isValid()) {
        return true;
    }
    const math::BoundingBox interior = m_room.getInteriorBounds();
    return worldBounds.min.x >= interior.min.x - tolerance &&
           worldBounds.max.x <= interior.max.x + tolerance &&
           worldBounds.min.y >= interior.min.y - tolerance &&
           worldBounds.max.y <= interior.max.y + tolerance &&
           worldBounds.min.z >= interior.min.z - tolerance &&
           worldBounds.max.z <= interior.max.z + tolerance;
}

} // namespace roomcraft::scene
