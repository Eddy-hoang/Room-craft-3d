#include "roomcraft/assets/ModelLoader.hpp"

#include <glm/geometric.hpp>
#include <glm/vec4.hpp>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

constexpr float kEps = 1e-3f;

std::filesystem::path fixturePath(const std::string& filename)
{
    return std::filesystem::path(ROOMCRAFT_SOURCE_DIR) / "tests" / "fixtures" / filename;
}

bool approxEqual(float a, float b, float eps = kEps)
{
    return std::fabs(a - b) <= eps;
}

} // namespace

int runModelLoaderTests()
{
    int failures = 0;
    const auto check = [&failures](const char* name, bool condition) {
        std::cout << "  [ModelLoader] " << name << ": " << (condition ? "PASS" : "FAIL") << std::endl;
        if (!condition) {
            ++failures;
        }
    };

    // TC-M3-03: Từ chối đường dẫn không tồn tại
    {
        roomcraft::assets::AssetData asset;
        std::string err;
        const bool ok = roomcraft::assets::ModelLoader::loadFromFile(
            fixturePath("does_not_exist_model.obj"), {}, asset, &err);
        check("TC-M3-03 Nonexistent model path rejected cleanly", !ok && !err.empty());
    }

    // TC-M3-04: Từ chối model rỗng / không hợp lệ
    {
        roomcraft::assets::AssetData asset;
        std::string err;
        const bool ok = roomcraft::assets::ModelLoader::loadFromFile(
            fixturePath("invalid_empty.obj"), {}, asset, &err);
        check("TC-M3-04 Empty/invalid OBJ model rejected cleanly", !ok && !err.empty());
    }

    // TC-M3-02, TC-M3-05, TC-M3-08, TC-M3-09, TC-M3-14, TC-M3-15:
    // Import quad box không có normals và không có UVs -> triangulate 12 tris, sinh normals, UV (0,0), pivot đáy
    {
        roomcraft::assets::AssetData asset;
        std::string err;
        const bool ok = roomcraft::assets::ModelLoader::loadFromFile(
            fixturePath("quad_no_normals_no_uv.obj"), {}, asset, &err);

        bool normalsValid = ok;
        bool uvsZeroFinite = ok;
        if (ok) {
            for (const auto& sub : asset.subMeshes) {
                for (const auto& v : sub.meshData.vertices) {
                    if (std::fabs(glm::length(v.normal) - 1.0f) > 1e-2f) {
                        normalsValid = false;
                    }
                    if (!std::isfinite(v.texCoord.x) || !std::isfinite(v.texCoord.y) ||
                        std::fabs(v.texCoord.x) > 1e-4f || std::fabs(v.texCoord.y) > 1e-4f) {
                        uvsZeroFinite = false;
                    }
                }
            }
        }

        const glm::vec3 size = asset.localBounds.getSize();
        const glm::vec3 center = asset.localBounds.getCenter();
        check("TC-M3-02/05 Valid quad OBJ triangulated to 12 triangles",
              ok && asset.getTotalTriangleCount() == 12);
        check("TC-M3-08/09 Missing normals generated & missing UVs defaulted to (0,0)",
              normalsValid && uvsZeroFinite);
        check("TC-M3-14/15 Bottom-center pivot normalized (min.y=0, center XZ=0) & size=2x1x3m",
              ok &&
              approxEqual(asset.localBounds.min.y, 0.0f) &&
              approxEqual(center.x, 0.0f) &&
              approxEqual(center.z, 0.0f) &&
              approxEqual(size.x, 2.0f) &&
              approxEqual(size.y, 1.0f) &&
              approxEqual(size.z, 3.0f));
    }

    // TC-M3-06, TC-M3-07, TC-M3-13, TC-M3-16: Multi-mesh, material mapping & diffuse colors
    {
        roomcraft::assets::AssetData asset;
        std::string err;
        const bool ok = roomcraft::assets::ModelLoader::loadFromFile(
            fixturePath("multi_material.obj"), {}, asset, &err);

        bool foundRedMat = false;
        bool foundBlueMat = false;
        bool subMeshMaterialMapped = ok && (asset.subMeshes.size() == 2);
        if (ok) {
            for (const auto& sub : asset.subMeshes) {
                const auto& mat = asset.materials[sub.materialIndex];
                if (mat.name == "MatRed" && mat.baseColor.r > 0.9f && mat.baseColor.b < 0.1f) {
                    foundRedMat = true;
                }
                if (mat.name == "MatBlue" && mat.baseColor.b > 0.9f && mat.baseColor.r < 0.1f) {
                    foundBlueMat = true;
                }
            }
        }

        // Bất đối xứng: bảng trái cao 2m, bảng phải cao 1m -> tổng chiều rộng 2m, chiều cao 2m (+Y)
        const glm::vec3 sz = asset.localBounds.getSize();
        check("TC-M3-06/07/16 Multi-mesh (2 submeshes) + material index & RGB colors",
              subMeshMaterialMapped && foundRedMat && foundBlueMat);
        check("TC-M3-13 Y-up asymmetric bounds verified (2.0 x 2.0 x 0.0m)",
              ok && approxEqual(sz.x, 2.0f) && approxEqual(sz.y, 2.0f) && approxEqual(asset.localBounds.min.y, 0.0f));
    }

    // TC-M3-11: Chuyển đổi tường minh Assimp row-major sang GLM column-major (không memcpy)
    {
        // Ma trận tịnh tiến (tx=3, ty=-4, tz=5) theo quy ước hàng của Assimp (a4=3, b4=-4, c4=5)
        const glm::mat4 m = roomcraft::assets::ModelLoader::convertRowMajor4x4(
            1.0f, 0.0f, 0.0f,  3.0f,
            0.0f, 1.0f, 0.0f, -4.0f,
            0.0f, 0.0f, 1.0f,  5.0f,
            0.0f, 0.0f, 0.0f,  1.0f);
        const glm::vec4 moved = m * glm::vec4(1.0f, 2.0f, 3.0f, 1.0f);
        check("TC-M3-11 Explicit row-major Assimp to column-major GLM conversion",
              approxEqual(moved.x, 4.0f) && approxEqual(moved.y, -2.0f) && approxEqual(moved.z, 8.0f));
    }

    return failures;
}
