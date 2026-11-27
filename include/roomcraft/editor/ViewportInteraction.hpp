#pragma once

#include "roomcraft/core/InputManager.hpp"
#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/math/Ray.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/systems/CameraController.hpp"
#include "roomcraft/systems/InteractionState.hpp"
#include "roomcraft/systems/PickingSystem.hpp"
#include "roomcraft/systems/SelectionState.hpp"

#include <string>

namespace roomcraft::editor {

// Ngữ cảnh định tuyến đầu vào từ Dear ImGui sang Viewport 3D.
// Tách khỏi lời gọi ImGui trực tiếp để có thể kiểm thử toàn bộ luật Input Routing trên CPU.
struct ViewportInputContext {
    math::ViewportRect viewportRect;
    bool viewportImageHovered = false;
    bool uiWantsTextInput = false;
    bool uiPopupOpen = false;
    bool anyUiWidgetActive = false;
};

struct ViewportInteractionOutcome {
    bool sceneModified = false;
    bool consumedEscape = false;
    std::string statusMessage;
    std::string errorMessage;
};

// Bộ định tuyến & điều phối tương tác Viewport 3D (Orbit, Pan, Zoom, Picking, Drag, Rotate, Scale, Shortcuts).
// Không phụ thuộc trực tiếp vào Dear ImGui hay OpenGL.
class ViewportInteraction {
public:
    static ViewportInteractionOutcome processInput(const core::InputManager& input,
                                                   const ViewportInputContext& ctx,
                                                   scene::Scene& scene,
                                                   systems::SelectionState& selection,
                                                   systems::CameraController& cameraController,
                                                   graphics::Camera& camera,
                                                   systems::InteractionState& state,
                                                   const systems::PickingSystem::AssetDataLookupFn& assetLookup,
                                                   const systems::PickOptions& pickOptions = {});
};

} // namespace roomcraft::editor
