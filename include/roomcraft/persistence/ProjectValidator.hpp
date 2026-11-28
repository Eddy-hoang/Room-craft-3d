#pragma once

#include "roomcraft/assets/FurnitureCatalog.hpp"
#include "roomcraft/persistence/ProjectData.hpp"

#include <string>

namespace roomcraft::persistence {

// Kiểm tra toàn diện dữ liệu ProjectData trước khi lưu hoặc sau khi deserialize (M8.6 / M8.7).
// Đảm bảo dữ liệu hợp lệ hoàn toàn trước khi đụng vào file đích hoặc Scene đang mở.
class ProjectValidator {
public:
    static bool validate(const ProjectData& data,
                         const assets::FurnitureCatalog& catalog,
                         std::string* outError = nullptr);
};

} // namespace roomcraft::persistence
