#pragma once

#include "roomcraft/scene/ObjectId.hpp"
#include "roomcraft/scene/Scene.hpp"

namespace roomcraft::systems {

// Lớp SelectionState quản lý đối tượng đang được chọn thông qua Stable ObjectId
// (tuyệt đối không lưu raw pointer hay iterator có thể mất hiệu lực khi vector thay đổi).
class SelectionState {
public:
    void selectObject(scene::ObjectId id) { m_selectedObjectId = id; }
    void clearSelection() { m_selectedObjectId = scene::kInvalidObjectId; }

    scene::ObjectId getSelectedObjectId() const { return m_selectedObjectId; }
    bool hasSelection() const { return scene::isValidObjectId(m_selectedObjectId); }
    bool isSelected(scene::ObjectId id) const
    {
        return hasSelection() && m_selectedObjectId == id;
    }

    // Đồng bộ lại trạng thái chọn với Scene hiện tại: nếu object đang chọn đã bị xóa thì xóa selection
    void reconcileWithScene(const scene::Scene& scene)
    {
        if (hasSelection() && scene.findObject(m_selectedObjectId) == nullptr) {
            clearSelection();
        }
    }

private:
    scene::ObjectId m_selectedObjectId = scene::kInvalidObjectId;
};

} // namespace roomcraft::systems
