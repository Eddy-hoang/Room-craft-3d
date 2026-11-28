#pragma once

#include "roomcraft/assets/FurnitureCatalog.hpp"
#include "roomcraft/persistence/ProjectData.hpp"
#include "roomcraft/scene/Scene.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace roomcraft::persistence {

// Quản lý vòng đời dự án .roomcraft (M8.7, M8.8, M8.9):
// - Ghi tệp an toàn qua tệp tạm cùng thư mục + thay thế nguyên tử (MoveFileExW trên Windows),
//   KHÔNG bao giờ cắt cụt hay phá hỏng tệp dự án hợp lệ cũ khi ghi thất bại.
// - Nạp giao dịch (Transactional Loading): đọc -> parse -> validate -> dựng Scene & Camera ứng viên ->
//   chỉ thay thế dự án hiện tại khi tất cả bước đều thành công.
// - Quản lý trạng thái chưa lưu (dirty state), đường dẫn tệp hiện hành và danh sách Recent Projects (P1).
class ProjectManager {
public:
    static constexpr std::size_t kMaxRecentProjects = 8;

    ProjectManager() = default;

    // Ghi an toàn ProjectData xuống đĩa (tệp tạm trong cùng thư mục -> replace đích).
    // Hỗ trợ đường dẫn Unicode và đường dẫn có khoảng trắng.
    static bool writeProjectFileSafe(const std::filesystem::path& destinationPath,
                                     const ProjectData& data,
                                     const assets::FurnitureCatalog& catalog,
                                     std::string* outError = nullptr);

    // Đọc, phân tích và xác thực đầy đủ tệp dự án vào ProjectData ứng viên + Scene ứng viên.
    // KHÔNG chỉnh sửa bất kỳ trạng thái hiện hành nào nếu thất bại.
    static bool loadAndPrepareProject(const std::filesystem::path& sourcePath,
                                      const assets::FurnitureCatalog& catalog,
                                      ProjectData& outData,
                                      scene::Scene& outCandidateScene,
                                      graphics::Camera& outCandidateCamera,
                                      std::string* outError = nullptr);

    // Trạng thái dự án đang mở trong Editor
    bool saveCurrentOrAs(const std::filesystem::path& path,
                         const ProjectData& data,
                         const assets::FurnitureCatalog& catalog,
                         std::string* outError = nullptr);

    bool openTransactional(const std::filesystem::path& path,
                           const assets::FurnitureCatalog& catalog,
                           scene::Scene& activeScene,
                           graphics::Camera& activeCamera,
                           ProjectData& outLoadedData,
                           std::string* outError = nullptr);

    void resetForNewProject(const std::string& projectName, bool dirty = false);

    bool hasCurrentFilePath() const { return !m_currentFilePath.empty(); }
    const std::filesystem::path& getCurrentFilePath() const { return m_currentFilePath; }
    const std::string& getProjectName() const { return m_projectName; }
    void setProjectName(const std::string& name) { m_projectName = name; }

    bool isDirty() const { return m_dirty; }
    void markDirty() { m_dirty = true; }
    void clearDirty() { m_dirty = false; }

    const std::vector<std::filesystem::path>& getRecentProjects() const { return m_recentProjects; }
    void recordRecentProject(const std::filesystem::path& path);

    // Helper lấy tên hiển thị từ đường dẫn tệp (không kèm đuôi .roomcraft)
    static std::string deriveProjectNameFromPath(const std::filesystem::path& path);

private:
    std::filesystem::path m_currentFilePath;
    std::string m_projectName = "Modern Bedroom";
    bool m_dirty = false;
    std::vector<std::filesystem::path> m_recentProjects;
};

} // namespace roomcraft::persistence
