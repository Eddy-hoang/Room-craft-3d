#include "roomcraft/assets/AssetNormalizer.hpp"
#include "roomcraft/geometry/PrimitiveGenerator.hpp"
#include "roomcraft/graphics/Camera.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>

#include <cmath>
#include <iostream>
#include <string>

namespace {

constexpr float kEps = 1e-3f;

bool approxEqual(float a, float b, float eps = kEps)
{
    return std::fabs(a - b) <= eps;
}

bool allCornersInsideNdc(const roomcraft::graphics::Camera& cam, const roomcraft::math::BoundingBox& box)
{
    const glm::mat4 vp = cam.getProjectionMatrix() * cam.getViewMatrix();
    const glm::vec3 corners[8] = {
        {box.min.x, box.min.y, box.min.z}, {box.max.x, box.min.y, box.min.z},
        {box.min.x, box.max.y, box.min.z}, {box.max.x, box.max.y, box.min.z},
        {box.min.x, box.min.y, box.max.z}, {box.max.x, box.min.y, box.max.z},
        {box.min.x, box.max.y, box.max.z}, {box.max.x, box.max.y, box.max.z}
    };
    for (const auto& c : corners) {
        const glm::vec4 clip = vp * glm::vec4(c, 1.0f);
        if (clip.w <= 0.0f) {
            return false;
        }
        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        if (std::fabs(ndc.x) > 1.01f || std::fabs(ndc.y) > 1.01f || ndc.z < -1.01f || ndc.z > 1.01f) {
            return false;
        }
    }
    return true;
}

} // namespace

int runAssetNormalizerTests()
{
    int failures = 0;
    const auto check = [&failures](const char* name, bool condition) {
        std::cout << "  [AssetNormalizer] " << name << ": " << (condition ? "PASS" : "FAIL") << std::endl;
        if (!condition) {
            ++failures;
        }
    };

    // TC-M3-10: Node transform accumulation (parent-child, translation, rotation, non-uniform scale & reflection)
    {
        roomcraft::geometry::MeshData cube;
        roomcraft::geometry::PrimitiveGenerator::createCuboid(1.0f, 1.0f, 1.0f, cube);

        // Parent: translate (2, 0, 0) + rotate Y +90°; Child: non-uniform scale (2, 3, 1)
        const glm::mat4 parentMat = glm::rotate(glm::translate(glm::mat4(1.0f), {2.0f, 0.0f, 0.0f}),
                                                glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 childMat = glm::scale(glm::mat4(1.0f), {2.0f, 3.0f, 1.0f});
        const glm::mat4 combined = parentMat * childMat;

        std::string err;
        const bool ok = roomcraft::assets::AssetNormalizer::bakeTransformIntoMesh(cube, combined, &err);
        const glm::vec3 sz = cube.boundingBox.getSize();
        bool normalsUnit = ok;
        for (const auto& v : cube.vertices) {
            if (std::fabs(glm::length(v.normal) - 1.0f) > 1e-2f) {
                normalsUnit = false;
            }
        }
        // Scale (2,3,1) xoay Y 90° -> kích thước thế giới (1, 3, 2)
        check("TC-M3-10 Parent-child node transform + non-uniform scale inverse-transpose normals",
              ok && normalsUnit && approxEqual(sz.x, 1.0f) && approxEqual(sz.y, 3.0f) && approxEqual(sz.z, 2.0f));

        // Kiểm tra ma trận phản xạ (negative scale det < 0) phải tự đảo winding giữ CCW hướng ngoài
        roomcraft::geometry::MeshData reflected;
        roomcraft::geometry::PrimitiveGenerator::createCube(1.0f, reflected);
        const glm::mat4 mirrorX = glm::scale(glm::mat4(1.0f), {-1.0f, 1.0f, 1.0f});
        const bool reflOk = roomcraft::assets::AssetNormalizer::bakeTransformIntoMesh(reflected, mirrorX, &err);
        bool windingPreserved = reflOk;
        for (std::size_t t = 0; t + 2 < reflected.indices.size(); t += 3) {
            const auto& v0 = reflected.vertices[reflected.indices[t]];
            const auto& v1 = reflected.vertices[reflected.indices[t + 1]];
            const auto& v2 = reflected.vertices[reflected.indices[t + 2]];
            const glm::vec3 fn = glm::normalize(glm::cross(v1.position - v0.position, v2.position - v0.position));
            if (glm::dot(fn, v0.normal) <= 0.0f) {
                windingPreserved = false;
            }
        }
        check("TC-M3-10 Negative determinant reflection flips triangle winding to preserve CCW", windingPreserved);

        // Từ chối ma trận suy biến (scale = 0)
        const glm::mat4 singular = glm::scale(glm::mat4(1.0f), {1.0f, 0.0f, 1.0f});
        const bool rejectSingular = !roomcraft::assets::AssetNormalizer::bakeTransformIntoMesh(reflected, singular, &err);
        check("TC-M3-10 Singular node matrix rejected cleanly", rejectSingular);
    }

    // TC-M3-12 & TC-M3-14: Unit scale (cm -> m = 0.01) giữ tỷ lệ thật, KHÔNG ép về unit cube
    {
        roomcraft::assets::AssetData asset;
        asset.materials.push_back(roomcraft::graphics::MaterialData::makeDefault());
        roomcraft::assets::SubMeshData sub;
        sub.name = "CentimeterSofa";
        roomcraft::geometry::PrimitiveGenerator::createCuboid(2.0f, 0.8f, 0.9f, sub.meshData);
        // Quy đổi sang đơn vị cm (x100) và dịch lệch khỏi gốc để kiểm thử unitScale = 0.01f -> mét
        for (auto& v : sub.meshData.vertices) {
            v.position = v.position * 100.0f + glm::vec3(50.0f, 25.0f, -30.0f);
        }
        sub.meshData.recomputeBoundingBox();
        asset.subMeshes.push_back(std::move(sub));

        std::string err;
        const bool ok = roomcraft::assets::AssetNormalizer::normalize(asset, 0.01f, true, &err);
        const glm::vec3 sz = asset.localBounds.getSize();
        const glm::vec3 center = asset.localBounds.getCenter();
        check("TC-M3-12/14 Unit scale 0.01 preserves real proportions (2.0x0.8x0.9m) & bottom-center pivot",
              ok &&
              approxEqual(sz.x, 2.0f) && approxEqual(sz.y, 0.8f) && approxEqual(sz.z, 0.9f) &&
              approxEqual(asset.localBounds.min.y, 0.0f) &&
              approxEqual(center.x, 0.0f) && approxEqual(center.z, 0.0f));

        const bool rejectBadScale = !roomcraft::assets::AssetNormalizer::normalize(asset, -1.0f, true, &err);
        check("TC-M3-12 Invalid unit scale rejected cleanly", rejectBadScale);
    }

    // TC-M3-25: Camera Auto-Fit kiểm thử trên nhiều kích thước (nhỏ, cao, rộng, phẳng) & aspect ngang/dọc
    {
        const roomcraft::math::BoundingBox boxes[] = {
            {{-0.10f, 0.0f, -0.10f}, {0.10f, 0.35f, 0.10f}, true}, // Bình hoa nhỏ
            {{-0.70f, 0.0f, -0.30f}, {0.70f, 2.10f, 0.30f}, true}, // Tủ cao
            {{-1.10f, 0.0f, -0.75f}, {1.10f, 0.02f, 0.75f}, true}, // Thảm rộng phẳng
            {{-1.05f, 0.0f, -0.45f}, {1.05f, 0.85f, 0.45f}, true}  // Sofa rộng
        };
        const float aspects[] = {16.0f / 9.0f, 9.0f / 16.0f, 1.0f};
        bool allFit = true;

        for (const auto& box : boxes) {
            for (float aspect : aspects) {
                roomcraft::graphics::Camera camPersp;
                camPersp.setProjectionMode(roomcraft::graphics::ProjectionMode::Perspective);
                camPersp.setAspectRatio(aspect);
                allFit &= camPersp.fitBoundingBox(box, 1.25f) && allCornersInsideNdc(camPersp, box);

                roomcraft::graphics::Camera camOrtho;
                camOrtho.setProjectionMode(roomcraft::graphics::ProjectionMode::Orthographic);
                camOrtho.setAspectRatio(aspect);
                allFit &= camOrtho.fitBoundingBox(box, 1.25f) && allCornersInsideNdc(camOrtho, box);
            }
        }
        check("TC-M3-25 Camera Auto-Fit frames small/tall/flat/wide bounds in Persp/Ortho & landscape/portrait", allFit);
    }

    return failures;
}
