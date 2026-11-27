#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string>

namespace roomcraft::graphics {

// Cấu trúc dữ liệu vật liệu thuần CPU (không chứa GPU handle).
// Lưu màu cơ bản, đường dẫn texture diffuse và các thuộc tính chuẩn bị cho M7 (Phong/Blinn-Phong).
// Lưu ý giới hạn M3: renderer preview M3 mới sử dụng baseColor và diffuseTexturePath;
// specularColor, shininess và opacity được lưu metadata phục vụ M7.
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
