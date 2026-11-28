#pragma once

#include <filesystem>
#include <string>

namespace roomcraft::core {

// Hộp thoại chọn tệp gốc của Windows (Win32 GetOpenFileNameW / GetSaveFileNameW)
// tương thích trực tiếp với FreeGLUT/Dear ImGui, hỗ trợ Unicode & khoảng trắng
// và bật OFN_NOCHANGEDIR để không làm thay đổi thư mục làm việc hiện hành.
class FileDialog {
public:
    static bool openProjectFile(std::filesystem::path& outPath,
                                const std::filesystem::path& initialDir = {});

    static bool saveProjectFile(std::filesystem::path& outPath,
                                const std::string& defaultFileName = "UntitledRoom.roomcraft",
                                const std::filesystem::path& initialDir = {});
};

} // namespace roomcraft::core
