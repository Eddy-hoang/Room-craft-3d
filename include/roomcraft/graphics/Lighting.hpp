#pragma once
#include <glm/vec3.hpp>

namespace roomcraft::graphics {

enum class LightingPreset { Daylight, WarmInterior, NeutralStudio, Evening };

struct LightingSettings {
    // Daylight mặc định (RC2): tương phản hình khối tự nhiên theo thẩm mỹ Scandinavian Modern
    float ambient = 0.30f;
    bool sunEnabled = true;
    glm::vec3 direction{-0.58f, -0.95f, -0.42f};
    glm::vec3 sunColor{1.0f, 0.96f, 0.90f};
    float sunIntensity = 0.92f;
    bool pointEnabled = false;
    glm::vec3 position{0.25f, 2.40f, 0.20f};
    glm::vec3 pointColor{1.0f, 0.78f, 0.56f};
    float pointIntensity = 1.25f;
    glm::vec3 attenuation{1.0f, 0.34f, 0.24f};

    // Directional Shadow Mapping (M9.4 / RC2.1 — tùy chọn bật/tắt & độ phân giải 512/1024/2048)
    bool shadowsEnabled = true;
    int shadowMapSize = 1024;
    float shadowBias = 0.0012f;

    static LightingSettings preset(LightingPreset preset)
    {
        LightingSettings s;
        switch (preset) {
        case LightingPreset::WarmInterior:
            s.ambient = 0.25f;
            s.direction = {-0.70f, -0.78f, -0.35f};
            s.sunColor = {1.0f, 0.88f, 0.72f};
            s.sunIntensity = 0.48f;
            s.pointEnabled = true;
            s.position = {0.25f, 2.40f, 0.20f};
            s.pointColor = {1.0f, 0.78f, 0.56f};
            s.pointIntensity = 1.25f;
            s.attenuation = {1.0f, 0.34f, 0.24f};
            break;
        case LightingPreset::NeutralStudio:
            s.ambient = 0.38f;
            s.direction = {-0.50f, -1.0f, -0.55f};
            s.sunColor = {0.98f, 0.99f, 1.0f};
            s.sunIntensity = 0.76f;
            s.pointEnabled = false;
            break;
        case LightingPreset::Evening:
            s.ambient = 0.14f;
            s.direction = {-0.85f, -0.48f, 0.38f};
            s.sunColor = {0.52f, 0.66f, 0.98f};
            s.sunIntensity = 0.34f;
            s.pointEnabled = true;
            s.position = {0.25f, 2.40f, 0.20f};
            s.pointColor = {1.0f, 0.74f, 0.48f};
            s.pointIntensity = 1.35f;
            s.attenuation = {1.0f, 0.30f, 0.22f};
            break;
        case LightingPreset::Daylight: break;
        }
        return s;
    }
};
} // namespace roomcraft::graphics
