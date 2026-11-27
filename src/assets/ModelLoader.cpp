#include "roomcraft/assets/ModelLoader.hpp"
#include "roomcraft/assets/AssetNormalizer.hpp"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/geometric.hpp>

#include <chrono>
#include <cmath>
#include <iostream>

namespace roomcraft::assets {

namespace {

bool failWith(std::string* outError, const std::string& message)
{
    if (outError != nullptr) {
        *outError = message;
    }
    return false;
}

glm::mat4 convertAssimpMatrix(const aiMatrix4x4& m)
{
    return ModelLoader::convertRowMajor4x4(m.a1, m.a2, m.a3, m.a4,
                                            m.b1, m.b2, m.b3, m.b4,
                                            m.c1, m.c2, m.c3, m.c4,
                                            m.d1, m.d2, m.d3, m.d4);
}

bool processAssimpNode(const aiNode* node,
                       const aiScene* scene,
                       const glm::mat4& parentTransform,
                       AssetData& outAsset,
                       std::string* outError)
{
    if (node == nullptr) {
        return true;
    }

    // Tích lũy ma trận phân cấp từ node cha xuống node con
    const glm::mat4 nodeLocal = convertAssimpMatrix(node->mTransformation);
    const glm::mat4 worldTransform = parentTransform * nodeLocal;

    for (unsigned int m = 0; m < node->mNumMeshes; ++m) {
        const unsigned int meshIdx = node->mMeshes[m];
        if (meshIdx >= scene->mNumMeshes) {
            continue;
        }
        const aiMesh* aiM = scene->mMeshes[meshIdx];
        if (aiM == nullptr || aiM->mNumVertices == 0 || aiM->mNumFaces == 0) {
            continue;
        }

        // Bỏ qua các primitive không phải tam giác (point/line) có cảnh báo rõ ràng
        if ((aiM->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) == 0) {
            std::cerr << "[RoomCraft3D][WARN] Skipping non-triangle primitive mesh '"
                      << aiM->mName.C_Str() << "'." << std::endl;
            continue;
        }

        geometry::MeshData meshData;
        meshData.topology = graphics::PrimitiveTopology::Triangles;
        meshData.vertices.reserve(aiM->mNumVertices);

        const bool hasNormals = aiM->HasNormals();
        const bool hasTexCoords = aiM->HasTextureCoords(0);

        for (unsigned int v = 0; v < aiM->mNumVertices; ++v) {
            graphics::Vertex vert;
            vert.position = glm::vec3(aiM->mVertices[v].x, aiM->mVertices[v].y, aiM->mVertices[v].z);

            if (hasNormals) {
                glm::vec3 n(aiM->mNormals[v].x, aiM->mNormals[v].y, aiM->mNormals[v].z);
                const float nLen = glm::length(n);
                vert.normal = (std::isfinite(nLen) && nLen > 1e-6f) ? (n / nLen) : glm::vec3(0.0f, 1.0f, 0.0f);
            } else {
                vert.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            if (hasTexCoords) {
                vert.texCoord = glm::vec2(aiM->mTextureCoords[0][v].x, aiM->mTextureCoords[0][v].y);
            } else {
                vert.texCoord = glm::vec2(0.0f, 0.0f);
            }

            meshData.vertices.push_back(vert);
        }

        meshData.indices.reserve(static_cast<std::size_t>(aiM->mNumFaces) * 3);
        for (unsigned int f = 0; f < aiM->mNumFaces; ++f) {
            const aiFace& face = aiM->mFaces[f];
            if (face.mNumIndices != 3) {
                continue;
            }
            meshData.indices.push_back(face.mIndices[0]);
            meshData.indices.push_back(face.mIndices[1]);
            meshData.indices.push_back(face.mIndices[2]);
        }

        if (meshData.indices.empty()) {
            continue;
        }

        // Nướng (bake) node worldTransform vào đỉnh và pháp tuyến (không áp dụng transform 2 lần)
        if (!AssetNormalizer::bakeTransformIntoMesh(meshData, worldTransform, outError)) {
            return false;
        }

        if (!meshData.validate(outError, false)) {
            return false;
        }

        SubMeshData sub;
        sub.name = (aiM->mName.length > 0) ? aiM->mName.C_Str() : ("SubMesh_" + std::to_string(outAsset.subMeshes.size()));
        sub.meshData = std::move(meshData);
        sub.materialIndex = (aiM->mMaterialIndex < outAsset.materials.size()) ? aiM->mMaterialIndex : 0;
        sub.localTransform = scene::Transform(); // Đã bake node transform nên localTransform là Identity
        outAsset.subMeshes.push_back(std::move(sub));
    }

    for (unsigned int c = 0; c < node->mNumChildren; ++c) {
        if (!processAssimpNode(node->mChildren[c], scene, worldTransform, outAsset, outError)) {
            return false;
        }
    }

    return true;
}

} // namespace

glm::mat4 ModelLoader::convertRowMajor4x4(float a1, float a2, float a3, float a4,
                                          float b1, float b2, float b3, float b4,
                                          float c1, float c2, float c3, float c4,
                                          float d1, float d2, float d3, float d4)
{
    // Assimp lưu ma trận theo hàng (row-major): hàng 0 = (a1, a2, a3, a4), cột tịnh tiến = (a4, b4, c4).
    // GLM lưu theo cột (column-major): m[col][row]. Chuyển đổi tường minh từng phần tử, không memcpy.
    glm::mat4 out(1.0f);
    out[0][0] = a1; out[1][0] = a2; out[2][0] = a3; out[3][0] = a4;
    out[0][1] = b1; out[1][1] = b2; out[2][1] = b3; out[3][1] = b4;
    out[0][2] = c1; out[1][2] = c2; out[2][2] = c3; out[3][2] = c4;
    out[0][3] = d1; out[1][3] = d2; out[2][3] = d3; out[3][3] = d4;
    return out;
}

bool ModelLoader::loadFromFile(const std::filesystem::path& filePath,
                               const ModelLoadOptions& options,
                               AssetData& outAsset,
                               std::string* outError)
{
    const auto startTime = std::chrono::steady_clock::now();

    std::error_code ec;
    if (filePath.empty() || !std::filesystem::exists(filePath, ec) || !std::filesystem::is_regular_file(filePath, ec)) {
        return failWith(outError, "Model file does not exist or is not a regular file: " + filePath.string());
    }

    Assimp::Importer importer;

    // Chọn lọc các post-process flag hợp lý:
    // - Triangulate: chuyển đa giác (quad/n-gon) thành tam giác
    // - JoinIdenticalVertices: tối ưu chia sẻ đỉnh
    // - GenSmoothNormals: CHỈ sinh pháp tuyến nếu model gốc không có normals
    // - ValidateDataStructure: kiểm tra toàn vẹn cấu trúc Assimp
    // KHÔNG bật aiProcess_FlipUVs (vì stb_image đã flip dọc đồng nhất cho OpenGL)
    // KHÔNG bật aiProcess_MakeLeftHanded (giữ hệ tọa độ tay phải Y-up)
    const unsigned int flags = aiProcess_Triangulate |
                               aiProcess_JoinIdenticalVertices |
                               aiProcess_GenSmoothNormals |
                               aiProcess_ValidateDataStructure;

    const aiScene* scene = importer.ReadFile(filePath.string(), flags);
    if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || scene->mRootNode == nullptr) {
        return failWith(outError, std::string("Assimp failed to load '") + filePath.string() +
                                      "': " + importer.GetErrorString());
    }

    AssetData asset;
    asset.assetId = options.assetId.empty() ? filePath.stem().string() : options.assetId;
    asset.displayName = options.displayName.empty() ? filePath.stem().string() : options.displayName;
    asset.sourceType = AssetSourceType::Imported;
    asset.category = options.category;
    asset.sourcePath = filePath.string();
    asset.unitScale = options.unitScale;
    asset.author = options.author;
    asset.license = options.license;
    asset.sourceReference = options.sourceReference;

    // 1. Đọc danh sách Material từ Assimp (OBJ/MTL); tạo default material nếu thiếu
    const std::filesystem::path modelDir = filePath.parent_path();
    if (scene->mNumMaterials == 0) {
        std::cerr << "[RoomCraft3D][WARN] Model '" << filePath.filename().string()
                  << "' has no materials; using fallback default material." << std::endl;
        asset.materials.push_back(graphics::MaterialData::makeDefault());
    } else {
        asset.materials.reserve(scene->mNumMaterials);
        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            const aiMaterial* aiMat = scene->mMaterials[i];
            graphics::MaterialData mat = graphics::MaterialData::makeDefault("Material_" + std::to_string(i));

            aiString matName;
            if (aiMat->Get(AI_MATKEY_NAME, matName) == AI_SUCCESS && matName.length > 0) {
                mat.name = matName.C_Str();
            }

            aiColor3D diffuse(0.8f, 0.8f, 0.82f);
            if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS) {
                mat.baseColor = glm::vec4(diffuse.r, diffuse.g, diffuse.b, 1.0f);
            }

            aiColor3D specular(0.25f, 0.25f, 0.25f);
            if (aiMat->Get(AI_MATKEY_COLOR_SPECULAR, specular) == AI_SUCCESS) {
                mat.specularColor = glm::vec3(specular.r, specular.g, specular.b);
            }

            float shininess = 32.0f;
            if (aiMat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS && std::isfinite(shininess) && shininess > 0.0f) {
                mat.shininess = shininess;
            }

            float opacity = 1.0f;
            if (aiMat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS && std::isfinite(opacity)) {
                mat.opacity = opacity;
                mat.baseColor.a = opacity;
            }

            if (aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
                aiString texPath;
                if (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS && texPath.length > 0) {
                    const std::filesystem::path rawTexPath(texPath.C_Str());
                    const std::filesystem::path resolvedTex =
                        rawTexPath.is_absolute() ? rawTexPath : (modelDir / rawTexPath);
                    mat.diffuseTexturePath = resolvedTex.lexically_normal().string();
                    mat.hasDiffuseTexture = true;
                }
            }

            asset.materials.push_back(std::move(mat));
        }
    }

    // 2. Duyệt cây node và trích xuất toàn bộ submesh hợp lệ
    if (!processAssimpNode(scene->mRootNode, scene, glm::mat4(1.0f), asset, outError)) {
        return false;
    }

    if (asset.subMeshes.empty()) {
        return failWith(outError, "Imported model contains no valid triangle meshes: " + filePath.string());
    }

    // 3. Chuẩn hóa tỷ lệ đơn vị và đưa pivot về tâm đáy (y = 0)
    if (!AssetNormalizer::normalize(asset, options.unitScale, options.normalizeBottomCenterPivot, outError)) {
        return false;
    }

    if (!asset.validate(outError)) {
        return false;
    }

    const auto endTime = std::chrono::steady_clock::now();
    asset.importTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    outAsset = std::move(asset);
    return true;
}

} // namespace roomcraft::assets
