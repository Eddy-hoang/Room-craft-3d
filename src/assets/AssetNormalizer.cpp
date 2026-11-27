#include "roomcraft/assets/AssetNormalizer.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec4.hpp>

#include <cmath>
#include <utility>

namespace roomcraft::assets {

namespace {

bool failWith(std::string* outError, const std::string& message)
{
    if (outError != nullptr) {
        *outError = message;
    }
    return false;
}

bool isFiniteMat4(const glm::mat4& m)
{
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            if (!std::isfinite(m[c][r])) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

bool AssetNormalizer::bakeTransformIntoMesh(geometry::MeshData& mesh,
                                            const glm::mat4& transform,
                                            std::string* outError)
{
    if (!isFiniteMat4(transform)) {
        return failWith(outError, "Transform matrix contains non-finite values.");
    }

    const glm::mat3 linear(transform);
    const float det = glm::determinant(linear);
    if (!std::isfinite(det) || std::fabs(det) < 1e-8f) {
        return failWith(outError, "Singular node transformation matrix (determinant near zero).");
    }

    // Ma trận biến đổi pháp tuyến chuẩn khi có non-uniform scaling: N = (M_3x3^-1)^T
    const glm::mat3 normalMatrix = glm::inverseTranspose(linear);

    for (auto& v : mesh.vertices) {
        const glm::vec4 transformedPos = transform * glm::vec4(v.position, 1.0f);
        v.position = glm::vec3(transformedPos);

        const glm::vec3 transformedNormal = normalMatrix * v.normal;
        const float nLen = glm::length(transformedNormal);
        if (std::isfinite(nLen) && nLen > 1e-6f) {
            v.normal = transformedNormal / nLen;
        } else {
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }

    // Nếu ma trận có định thức âm (phép phản xạ / negative scale), đảo thứ tự đỉnh để bảo toàn CCW winding
    if (det < 0.0f && mesh.topology == graphics::PrimitiveTopology::Triangles) {
        for (std::size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
            std::swap(mesh.indices[i + 1], mesh.indices[i + 2]);
        }
    }

    mesh.recomputeBoundingBox();
    return true;
}

bool AssetNormalizer::normalize(AssetData& asset,
                                float unitScale,
                                bool normalizeBottomCenterPivot,
                                std::string* outError)
{
    if (!std::isfinite(unitScale) || unitScale <= 1e-5f || unitScale > 1000.0f) {
        return failWith(outError, "AssetNormalizer unitScale must be finite and positive.");
    }
    if (asset.subMeshes.empty()) {
        return failWith(outError, "Cannot normalize AssetData with no submeshes.");
    }

    // 1. Áp dụng tỷ lệ đơn vị đồng nhất trên cả đỉnh và vị trí cục bộ của từng submesh
    if (std::fabs(unitScale - 1.0f) > 1e-6f) {
        for (auto& sub : asset.subMeshes) {
            for (auto& v : sub.meshData.vertices) {
                v.position *= unitScale;
            }
            sub.localTransform.position *= unitScale;
            sub.meshData.recomputeBoundingBox();
        }
    }

    asset.recomputeBounds();
    if (!asset.localBounds.isValid()) {
        return failWith(outError, "Normalized asset bounds are invalid.");
    }

    // 2. Tính điểm tâm đáy (bottom-center pivot) và dịch toàn bộ submesh về gốc tọa độ (0, 0, 0)
    if (normalizeBottomCenterPivot) {
        const glm::vec3& mn = asset.localBounds.min;
        const glm::vec3& mx = asset.localBounds.max;
        const glm::vec3 bottomCenter(
            0.5f * (mn.x + mx.x),
            mn.y,
            0.5f * (mn.z + mx.z)
        );

        for (auto& sub : asset.subMeshes) {
            // Nếu submesh có localTransform khác Identity (ví dụ từ procedural), dịch chuyển localTransform.position
            // Còn nếu submesh đã bake node transform (localTransform = Identity), dịch trực tiếp tọa độ đỉnh
            const bool isIdentityLocal =
                (glm::length(sub.localTransform.position) < 1e-6f) &&
                (glm::length(sub.localTransform.rotationDegrees) < 1e-6f) &&
                (glm::length(sub.localTransform.scale - glm::vec3(1.0f)) < 1e-6f);

            if (isIdentityLocal) {
                for (auto& v : sub.meshData.vertices) {
                    v.position -= bottomCenter;
                }
                sub.meshData.recomputeBoundingBox();
            } else {
                sub.localTransform.position -= bottomCenter;
            }
        }

        asset.recomputeBounds();
    }

    asset.unitScale = unitScale;
    return true;
}

} // namespace roomcraft::assets
