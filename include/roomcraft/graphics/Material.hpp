#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string>

namespace roomcraft::graphics {

// Cấu trúc dữ liệu vật liệu thuần CPU (không chứa GPU handle).
// Màu nền dùng quy ước sRGB của editor. Ks là hệ số phản xạ tuyến tính.
struct MaterialData {
    std::string name = "DefaultMaterial";
    glm::vec4 baseColor{0.80f, 0.80f, 0.82f, 1.0f};
    glm::vec3 specularColor{0.25f, 0.25f, 0.25f};
    float shininess = 32.0f;
    float opacity = 1.0f;

    // Đường dẫn texture diffuse tương đối (hoặc đã giải phân giải theo thư mục model)
    std::string diffuseTexturePath;
    bool hasDiffuseTexture = false;

    static MaterialData makeDefault(const std::string& matName = "DefaultMaterial")
    {
        MaterialData mat;
        mat.name = matName;
        return mat;
    }
};

} // namespace roomcraft::graphics
