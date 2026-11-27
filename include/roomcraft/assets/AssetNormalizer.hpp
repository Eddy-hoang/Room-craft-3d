#pragma once

#include "roomcraft/assets/AssetData.hpp"
#include "roomcraft/geometry/MeshData.hpp"

#include <glm/mat4x4.hpp>

#include <string>

namespace roomcraft::assets {

// Lớp AssetNormalizer chuẩn hóa mô hình nhập từ Blender/Assimp về quy ước thống nhất của RoomCraft 3D:
// - Hệ tọa độ tay phải, trục +Y hướng lên, mặt sàn XZ (y = 0), đơn vị mét.
// - Áp dụng unitScale tường minh (KHÔNG ép mô hình về hình lập phương đơn vị làm sai tỷ lệ thật).
// - Đưa gốc tọa độ (root pivot) về tâm đáy: ((min.x + max.x)/2, min.y, (min.z + max.z)/2) -> (0, 0, 0).
// - Biến đổi node transform an toàn với ma trận pháp tuyến (inverse transpose) và xử lý phản xạ (negative det).
class AssetNormalizer {
public:
    // Nướng (bake) một ma trận biến đổi 4x4 vào dữ liệu đỉnh CPU của MeshData:
    // - Vị trí: p' = M * p
    // - Pháp tuyến: n' = normalize(transpose(inverse(mat3(M))) * n)
    // - Nếu det(mat3(M)) < 0 (phép đối xứng/scale âm), tự động đảo chiều quấn tam giác để giữ CCW
    static bool bakeTransformIntoMesh(geometry::MeshData& mesh,
                                      const glm::mat4& transform,
                                      std::string* outError = nullptr);

    // Chuẩn hóa tỷ lệ đơn vị (unitScale) và căn gốc tọa độ về tâm đáy (y = 0) đồng nhất cho mọi submesh
    static bool normalize(AssetData& asset,
                          float unitScale = 1.0f,
                          bool normalizeBottomCenterPivot = true,
                          std::string* outError = nullptr);
};

} // namespace roomcraft::assets
