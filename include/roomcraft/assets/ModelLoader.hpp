#pragma once

#include "roomcraft/assets/AssetData.hpp"

#include <glm/mat4x4.hpp>

#include <filesystem>
#include <string>

namespace roomcraft::assets {

// Tùy chọn nạp mô hình 3D qua Assimp
struct ModelLoadOptions {
    std::string assetId;
    std::string displayName;
    AssetCategory category = AssetCategory::Decor;
    float unitScale = 1.0f;
    bool normalizeBottomCenterPivot = true;
    std::string author = "RoomCraft 3D";
    std::string license = "MIT";
    std::string sourceReference;
};

// Lớp ModelLoader nhập mô hình 3D (OBJ/MTL) bằng Assimp sang cấu trúc AssetData thuần CPU.
// Không giữ con trỏ aiScene/aiMesh sau khi trả về và không chứa bất kỳ lệnh OpenGL nào.
class ModelLoader {
public:
    // Chuyển đổi tường minh ma trận 4x4 row-major của Assimp (a1..d4) sang column-major của GLM (không memcpy)
    static glm::mat4 convertRowMajor4x4(float a1, float a2, float a3, float a4,
                                        float b1, float b2, float b3, float b4,
                                        float c1, float c2, float c3, float c4,
                                        float d1, float d2, float d3, float d4);

    // Nạp mô hình từ đường dẫn tương đối/tuyệt đối, duyệt cây node, đọc material/texture và chuẩn hóa
    static bool loadFromFile(const std::filesystem::path& filePath,
                             const ModelLoadOptions& options,
                             AssetData& outAsset,
                             std::string* outError = nullptr);
};

} // namespace roomcraft::assets
