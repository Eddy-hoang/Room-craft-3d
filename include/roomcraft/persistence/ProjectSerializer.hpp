#pragma once

#include "roomcraft/persistence/ProjectData.hpp"

#include <string>

namespace roomcraft::persistence {

// Chuyển đổi qua lại giữa ProjectData (CPU) và chuỗi JSON định dạng .roomcraft (schema v1).
// - ObjectId và nextObjectId được mã hóa dạng chuỗi thập phân không mất độ chính xác (lossless uint64_t).
// - Từ chối sai kiểu JSON, thiếu trường bắt buộc, giá trị không hữu hạn và schema không hỗ trợ.
class ProjectSerializer {
public:
    static bool toJsonString(const ProjectData& data,
                             std::string& outJson,
                             std::string* outError = nullptr);

    static bool fromJsonString(const std::string& jsonText,
                               ProjectData& outData,
                               std::string* outError = nullptr);
};

} // namespace roomcraft::persistence
