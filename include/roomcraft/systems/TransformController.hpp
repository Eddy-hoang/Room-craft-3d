#pragma once

#include "roomcraft/math/Ray.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/systems/InteractionState.hpp"

#include <glm/vec2.hpp>

#include <string>

namespace roomcraft::systems {

// Bộ điều khiển biến đổi đối tượng tương tác thuần CPU (Move / Rotate / Scale / Cancel).
// - Move: giao tia với mặt phẳng ngang tại initialTransform.position.y, giữ nguyên grabOffset & cao độ Y.
// - Rotate: xoay quanh trục Y từ initialTransform + tổng delta chuột (tùy chọn snap góc 15°).
// - Scale: tỷ lệ đồng nhất (hoặc theo trục X/Y/Z) luôn dương hữu hạn trong giới hạn [kMinScale, kMaxScale].
// - Cancel: khôi phục nguyên trạng initialTransform (không biến dạng mesh dùng chung).
class TransformController {
public:
    static constexpr float kMinScale = 0.10f;
    static constexpr float kMaxScale = 10.0f;

    // Bắt đầu kéo di chuyển đối tượng trên mặt phẳng ngang Y = initialTransform.position.y
    static bool beginMove(scene::Scene& scene,
                          scene::ObjectId objectId,
                          const math::Ray& startRay,
                          const glm::vec2& startMousePos,
                          InteractionState& state,
                          std::string* outError = nullptr);

    static bool updateMove(scene::Scene& scene,
                           const math::Ray& currentRay,
                           const glm::vec2& currentMousePos,
                           InteractionState& state,
                           std::string* outError = nullptr);

    // Bắt đầu xoay đối tượng quanh trục Y (hoặc trục X/Z khi bật AxisConstraint)
    static bool beginRotate(scene::Scene& scene,
                            scene::ObjectId objectId,
                            const glm::vec2& startMousePos,
                            InteractionState& state,
                            std::string* outError = nullptr);

    static bool updateRotate(scene::Scene& scene,
                             const glm::vec2& currentMousePos,
                             InteractionState& state,
                             std::string* outError = nullptr);

    // Bắt đầu phóng to / thu nhỏ đối tượng (mặc định Uniform Scale)
    static bool beginScale(scene::Scene& scene,
                           scene::ObjectId objectId,
                           const glm::vec2& startMousePos,
                           InteractionState& state,
                           std::string* outError = nullptr);

    static bool updateScale(scene::Scene& scene,
                            const glm::vec2& currentMousePos,
                            InteractionState& state,
                            std::string* outError = nullptr);

    // Chốt thao tác hiện tại và đưa máy trạng thái về Idle
    static void confirmInteraction(InteractionState& state);

    // Hủy thao tác hiện tại và khôi phục hoàn toàn initialTransform
    static bool cancelInteraction(scene::Scene& scene, InteractionState& state);
};

} // namespace roomcraft::systems
