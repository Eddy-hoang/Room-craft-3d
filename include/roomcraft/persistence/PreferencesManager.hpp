#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace roomcraft::persistence {

// Tùy chọn cá nhân của người dùng (lưu RIÊNG biệt với tệp dự án .roomcraft — M9.3 / M8-F18).
// Toàn bộ giá trị đều được xác thực và kẹp (clamp) an toàn khi nạp từ đĩa.
struct UserPreferences {
    int version = 1;
    float leftPanelWidth = 272.0f;
    float rightPanelWidth = 316.0f;
    float hierarchyHeightRatio = 0.36f;
    float uiScale = 1.0f;
    bool showLeftPanel = true;
    bool showRightPanel = true;
    bool libraryGridView = true;
    std::vector<std::filesystem::path> recentProjects;

    void clampToValidRanges();
};

// Quản lý đọc/ghi tệp preferences.json theo tài khoản người dùng (%APPDATA%\RoomCraft3D\preferences.json).
// Xử lý an toàn khi tệp thiếu, hỏng cú pháp hoặc chứa giá trị ngoài miền.
class PreferencesManager {
public:
    static constexpr std::size_t kMaxRecentProjects = 8;

    static std::filesystem::path getDefaultPreferencesPath();

    // Nạp tùy chọn từ đường dẫn chỉ định; nếu thiếu/hỏng thì trả về false và điền mặc định an toàn.
    static bool loadFromFile(const std::filesystem::path& path,
                             UserPreferences& outPrefs,
                             std::string* outError = nullptr);

    // Lưu tùy chọn an toàn qua tệp tạm cùng thư mục + MoveFileExW (tự tạo thư mục cha nếu chưa có).
    static bool saveToFile(const std::filesystem::path& path,
                           const UserPreferences& prefs,
                           std::string* outError = nullptr);
};

} // namespace roomcraft::persistence
