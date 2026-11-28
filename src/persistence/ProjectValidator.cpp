#include "roomcraft/persistence/ProjectValidator.hpp"

#include "roomcraft/scene/Scene.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace roomcraft::persistence {

namespace {

bool isFiniteVec3(const glm::vec3& v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

bool isValidColor3(const glm::vec3& c)
{
    return isFiniteVec3(c) &&
           c.r >= 0.0f && c.r <= 1.0f &&
           c.g >= 0.0f && c.g <= 1.0f &&
           c.b >= 0.0f && c.b <= 1.0f;
}

bool isValidColor4(const glm::vec4& c)
{
    return std::isfinite(c.r) && std::isfinite(c.g) && std::isfinite(c.b) && std::isfinite(c.a) &&
           c.r >= 0.0f && c.r <= 1.0f &&
           c.g >= 0.0f && c.g <= 1.0f &&
           c.b >= 0.0f && c.b <= 1.0f &&
           c.a >= 0.0f && c.a <= 1.0f;
}

} // namespace

bool ProjectValidator::validate(const ProjectData& data,
                                const assets::FurnitureCatalog& catalog,
                                std::string* outError)
{
    auto fail = [&](const std::string& msg) {
        if (outError != nullptr) *outError = msg;
        return false;
    };

    if (data.format != kProjectFormatId) {
        return fail("Unsupported project format identifier: '" + data.format +
                    "' (expected '" + std::string(kProjectFormatId) + "').");
    }

    if (data.schemaVersion != kCurrentSchemaVersion) {
        return fail("Unsupported project schema version: " + std::to_string(data.schemaVersion) +
                    " (this version of RoomCraft 3D supports schema version " +
                    std::to_string(kCurrentSchemaVersion) + ").");
    }

    if (scene::Scene::normalizeObjectName(data.projectName).empty()) {
        return fail("Project name cannot be empty.");
    }

    if (data.objects.size() > kMaxProjectObjects) {
        return fail("Project object count (" + std::to_string(data.objects.size()) +
                    ") exceeds maximum allowed limit (" + std::to_string(kMaxProjectObjects) + ").");
    }

    // 1. Kiểm tra RoomSettings (kích thước, độ dày, bước lưới, màu sắc, chế độ tường)
    std::string roomErr;
    if (!data.room.validate(&roomErr)) {
        return fail("Invalid room settings: " + roomErr);
    }
    if (!isValidColor4(data.room.floorColor) ||
        !isValidColor4(data.room.wallColor) ||
        !isValidColor4(data.room.ceilingColor)) {
        return fail("Room surface colors must be within [0.0, 1.0].");
    }

    // 2. Kiểm tra danh sách SceneObject, ID duy nhất, bộ cấp phát nextObjectId và tham chiếu Asset ID
    std::unordered_set<scene::ObjectId> seenIds;
    seenIds.reserve(data.objects.size());
    scene::ObjectId maxId = 0;

    for (std::size_t i = 0; i < data.objects.size(); ++i) {
        const auto& obj = data.objects[i];
        if (!scene::isValidObjectId(obj.id)) {
            return fail("Object #" + std::to_string(i) + " has invalid objectId (0).");
        }
        if (obj.id == std::numeric_limits<scene::ObjectId>::max()) {
            return fail("Object #" + std::to_string(i) + " objectId overflows allocator.");
        }
        if (!seenIds.insert(obj.id).second) {
            return fail("Duplicate objectId detected in project file: " + std::to_string(obj.id) + ".");
        }
        if (scene::Scene::normalizeObjectName(obj.name).empty()) {
            return fail("Object " + std::to_string(obj.id) + " has an empty name.");
        }
        if (catalog.findById(obj.assetId) == nullptr) {
            return fail("Missing or unknown assetId '" + obj.assetId +
                        "' referenced by object '" + obj.name + "' (ID " + std::to_string(obj.id) + ").");
        }
        std::string trErr;
        if (!scene::Scene::isValidTransform(obj.transform, &trErr)) {
            return fail("Invalid transform for object '" + obj.name + "' (ID " + std::to_string(obj.id) + "): " + trErr);
        }
        if (!std::isfinite(obj.shininessScale) || obj.shininessScale < 0.05f || obj.shininessScale > 20.0f) {
            return fail("Invalid material override shininessScale for object '" + obj.name +
                        "' (must be finite and in [0.05, 20.0]).");
        }
        maxId = std::max(maxId, obj.id);
    }

    const scene::ObjectId minNext = (maxId == 0) ? 1 : (maxId + 1);
    if (data.nextObjectId < minNext) {
        return fail("Invalid nextObjectId (" + std::to_string(data.nextObjectId) +
                    "): must be >= " + std::to_string(minNext) + ".");
    }

    // 3. Kiểm tra cấu hình ánh sáng M7 đầy đủ
    const auto& l = data.lighting;
    if (data.lightingPreset < -1 || data.lightingPreset > 3) {
        return fail("Invalid lighting preset index.");
    }
    if (!std::isfinite(l.ambient) || l.ambient < 0.0f || l.ambient > 2.0f) {
        return fail("Lighting ambient must be finite and in [0.0, 2.0].");
    }
    if (!isFiniteVec3(l.direction) || glm::dot(l.direction, l.direction) < 1e-8f) {
        return fail("Directional light direction must be finite and non-zero.");
    }
    if (!isValidColor3(l.sunColor) || !std::isfinite(l.sunIntensity) || l.sunIntensity < 0.0f || l.sunIntensity > 10.0f) {
        return fail("Directional light color/intensity out of valid range.");
    }
    if (!isFiniteVec3(l.position) || !isValidColor3(l.pointColor) ||
        !std::isfinite(l.pointIntensity) || l.pointIntensity < 0.0f || l.pointIntensity > 20.0f) {
        return fail("Point light position/color/intensity out of valid range.");
    }
    if (!isFiniteVec3(l.attenuation) ||
        l.attenuation.x <= 0.0f || l.attenuation.x > 20.0f ||
        l.attenuation.y < 0.0f || l.attenuation.y > 20.0f ||
        l.attenuation.z < 0.0f || l.attenuation.z > 20.0f) {
        return fail("Point light attenuation factors must be finite (constant > 0, linear/quadratic >= 0).");
    }

    // 4. Kiểm tra Render Settings & Editor Settings
    if (data.renderSettings.msaaSampleIndex < 0 || data.renderSettings.msaaSampleIndex > 2) {
        return fail("Invalid MSAA sample index (expected 0=Off, 1=2x, 2=4x).");
    }

    // 5. Kiểm tra trạng thái Camera hợp lệ (dựng thử trên Camera tạm)
    graphics::Camera testCam;
    if (!data.camera.applyToCamera(testCam)) {
        return fail("Invalid camera parameters (position/target/up/FOV/clip planes/orthoHalfHeight).");
    }

    return true;
}

} // namespace roomcraft::persistence
