#pragma once

#include "roomcraft/scene/ObjectId.hpp"
#include "roomcraft/scene/RoomSettings.hpp"
#include "roomcraft/scene/Scene.hpp"
#include "roomcraft/scene/SceneObject.hpp"

#include <cstddef>
#include <deque>
#include <string>
#include <vector>

namespace roomcraft::systems {

// Ảnh chụp trạng thái Scene gọn nhẹ phục vụ Undo/Redo có giới hạn (P2 - M8.10).
// Mỗi thao tác hoàn chỉnh (Add, Delete, Duplicate, 1 lần kéo Move/Rotate/Scale kết thúc,
// Rename, Visibility, Lock, Shininess, Apply Room Settings) được ghi thành ĐÚNG 1 mục lịch sử.
struct SceneHistorySnapshot {
    std::string description;
    scene::RoomSettings roomSettings;
    std::vector<scene::SceneObject> objects;
    scene::ObjectId nextObjectId = 1;
    scene::ObjectId selectedObjectId = scene::kInvalidObjectId;
};

class CommandHistory {
public:
    static constexpr std::size_t kDefaultMaxEntries = 64;

    explicit CommandHistory(std::size_t maxEntries = kDefaultMaxEntries)
        : m_maxEntries(maxEntries > 0 ? maxEntries : kDefaultMaxEntries)
    {
    }

    static SceneHistorySnapshot capture(const scene::Scene& scene,
                                        scene::ObjectId selectedId,
                                        const std::string& description)
    {
        SceneHistorySnapshot snap;
        snap.description = description;
        snap.roomSettings = scene.getRoom().getSettings();
        snap.objects = scene.getObjects();
        snap.nextObjectId = scene.getNextObjectId();
        snap.selectedObjectId = selectedId;
        return snap;
    }

    // Ghi nhận một thao tác đã hoàn tất (xóa nhánh Redo vì có chỉnh sửa mới)
    void pushUndo(SceneHistorySnapshot beforeState)
    {
        m_redoStack.clear();
        m_undoStack.push_back(std::move(beforeState));
        while (m_undoStack.size() > m_maxEntries) {
            m_undoStack.pop_front();
        }
    }

    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }
    std::size_t getUndoCount() const { return m_undoStack.size(); }
    std::size_t getRedoCount() const { return m_redoStack.size(); }

    std::string getNextUndoDescription() const
    {
        return m_undoStack.empty() ? std::string() : m_undoStack.back().description;
    }

    std::string getNextRedoDescription() const
    {
        return m_redoStack.empty() ? std::string() : m_redoStack.back().description;
    }

    // Thực hiện Undo: lưu trạng thái hiện tại vào Redo và khôi phục trạng thái trước đó
    bool undo(scene::Scene& scene,
              scene::ObjectId& ioSelectedId,
              const assets::FurnitureCatalog& catalog,
              std::string* outDescription = nullptr,
              std::string* outError = nullptr)
    {
        if (m_undoStack.empty()) {
            return false;
        }
        SceneHistorySnapshot target = m_undoStack.back();
        SceneHistorySnapshot current = capture(scene, ioSelectedId, target.description);

        // Giữ nguyên bộ cấp phát nextObjectId cao nhất để không bao giờ tái sử dụng ID đã cấp
        const scene::ObjectId safeNextId = std::max(target.nextObjectId, scene.getNextObjectId());
        if (!scene.restoreFromSnapshot(target.roomSettings, target.objects, safeNextId, catalog, outError)) {
            return false;
        }

        m_undoStack.pop_back();
        m_redoStack.push_back(std::move(current));
        ioSelectedId = (scene.findObject(target.selectedObjectId) != nullptr)
                           ? target.selectedObjectId
                           : scene::kInvalidObjectId;
        if (outDescription != nullptr) {
            *outDescription = target.description;
        }
        return true;
    }

    // Thực hiện Redo: lưu trạng thái hiện tại vào Undo và khôi phục trạng thái Redo
    bool redo(scene::Scene& scene,
              scene::ObjectId& ioSelectedId,
              const assets::FurnitureCatalog& catalog,
              std::string* outDescription = nullptr,
              std::string* outError = nullptr)
    {
        if (m_redoStack.empty()) {
            return false;
        }
        SceneHistorySnapshot target = m_redoStack.back();
        SceneHistorySnapshot current = capture(scene, ioSelectedId, target.description);

        const scene::ObjectId safeNextId = std::max(target.nextObjectId, scene.getNextObjectId());
        if (!scene.restoreFromSnapshot(target.roomSettings, target.objects, safeNextId, catalog, outError)) {
            return false;
        }

        m_redoStack.pop_back();
        m_undoStack.push_back(std::move(current));
        ioSelectedId = (scene.findObject(target.selectedObjectId) != nullptr)
                           ? target.selectedObjectId
                           : scene::kInvalidObjectId;
        if (outDescription != nullptr) {
            *outDescription = target.description;
        }
        return true;
    }

    void clear()
    {
        m_undoStack.clear();
        m_redoStack.clear();
    }

private:
    std::size_t m_maxEntries = kDefaultMaxEntries;
    std::deque<SceneHistorySnapshot> m_undoStack;
    std::deque<SceneHistorySnapshot> m_redoStack;
};

} // namespace roomcraft::systems
