#pragma once
#include <glm/vec3.hpp>

namespace roomcraft::graphics {

enum class LightingPreset { Daylight, WarmInterior, NeutralStudio, Evening };

struct LightingSettings {
    float ambient = 0.42f;
    bool sunEnabled = true;
    glm::vec3 direction{-0.6f, -1.0f, -0.45f};
    glm::vec3 sunColor{1.0f, 0.95f, 0.86f};
    float sunIntensity = 0.72f;
    bool pointEnabled = false;
    glm::vec3 position{-1.0f, 2.3f, 0.7f};
    glm::vec3 pointColor{1.0f, 0.68f, 0.42f};
    float pointIntensity = 1.3f;
    glm::vec3 attenuation{1.0f, 0.18f, 0.12f};

    static LightingSettings preset(LightingPreset preset)
    {
        LightingSettings s;
        switch (preset) {
        case LightingPreset::WarmInterior:
            s.ambient = 0.32f;
            s.sunIntensity = 0.36f;
            s.pointEnabled = true;
            s.pointIntensity = 2.0f;
            break;
        case LightingPreset::NeutralStudio:
            s.ambient = 0.50f;
            s.sunColor = glm::vec3(1.0f);
            s.sunIntensity = 0.62f;
            break;
        case LightingPreset::Evening:
            s.ambient = 0.17f;
            s.sunColor = {0.48f, 0.62f, 1.0f};
            s.sunIntensity = 0.35f;
            s.direction = {-1.0f, -0.28f, 0.4f};
            s.pointEnabled = true;
            break;
        case LightingPreset::Daylight: break;
        }
        return s;
    }
};
} // namespace roomcraft::graphics
