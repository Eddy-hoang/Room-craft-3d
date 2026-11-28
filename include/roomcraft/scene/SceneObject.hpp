#pragma once

#include "roomcraft/math/BoundingBox.hpp"
#include "roomcraft/scene/ObjectId.hpp"
#include "roomcraft/scene/Transform.hpp"

#include <glm/mat4x4.hpp>

#include <string>

namespace roomcraft::scene {

// Lớp SceneObject đại diện cho một THỂ HIỆN (instance) của đồ nội thất đặt trong căn phòng.
//
// Phân tách trách nhiệm (Asset vs. SceneObject):
// - Asset (AssetData / GpuAsset trong AssetManager): chứa hình học, vật liệu, texture dùng chung.
// - SceneObject: sở hữu ObjectId duy nhất, tên hiển thị, Transform (Position/Rotation/Scale)
//   và trạng thái hiển thị riêng; KHÔNG trực tiếp sở hữu VAO/VBO/EBO hay OpenGL texture handle.
//   Nhiều SceneObject có thể tham chiếu cùng một assetId nhưng biến đổi hoàn toàn độc lập.
struct SceneObject {
    ObjectId id = kInvalidObjectId;
    std::string name;
    std::string assetId;
    Transform transform;
    bool visible = true;
    bool locked = false;
    float shininessScale = 1.0f;

    glm::mat4 getModelMatrix() const
    {
        return transform.getModelMatrix();
    }

    // Tính hộp bao trong không gian thế giới (World-space AABB) bằng cách biến đổi 8 góc
    // của Local BoundingBox của asset theo ma trận Model của đối tượng.
    math::BoundingBox getWorldBounds(const math::BoundingBox& assetLocalBounds) const
    {
        if (!assetLocalBounds.isValid()) {
            return {};
        }
        return assetLocalBounds.transformed(transform.getModelMatrix());
    }
};

} // namespace roomcraft::scene
