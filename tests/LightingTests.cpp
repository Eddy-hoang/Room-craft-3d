#include "roomcraft/core/Window.hpp"
#include "roomcraft/graphics/Framebuffer.hpp"
#include "roomcraft/graphics/Renderer.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cmath>
#include <iostream>

using namespace roomcraft::graphics;

int main(int argc, char** argv)
{
    roomcraft::core::Window window({128, 128, "M7 lighting tests"});
    if (!window.initialize(argc, argv)) return 1;
    Shader shader;
    Framebuffer target;
    Mesh quad;
    const glm::vec3 normal = glm::normalize(glm::vec3(1, 0, 1));
    if (!shader.loadFromFiles("assets/shaders/mesh.vert", "assets/shaders/mesh.frag") ||
        !target.create(65, 65) ||
        !quad.create({{{-1,-1,0},normal,{}}, {{1,-1,0},normal,{}}, {{1,1,0},normal,{}},
                      {{-1,1,0},normal,{}}}, {0,1,2,0,2,3})) return 1;

    Camera cam;
    cam.lookAt({0,0,3}, {0,0,0});
    cam.setAspectRatio(1);
    Renderer renderer;
    LightingSettings settings;
    MaterialData material;
    material.specularColor = glm::vec3(0);
    glm::mat4 model(1);
    int requested = 1;
    int actual = 0;
    Texture swatch;
    if (!swatch.createSolidColor(128,128,128)) return 1;
    const Texture* texture = nullptr;
    bool floor = false;
    float gridStep = 0;
    auto pixel = [&] {
        actual = target.bindMultisampled(requested);
        renderer.beginFrame(target.getWidth(), target.getHeight(), {0,0,0,1});
        renderer.applyLighting(shader, cam, settings);
        shader.setInt("uFloor", floor);
        shader.setVec3("uGrid", {1,1,gridStep});
        renderer.drawMeshTextured(quad, shader, model, cam, {0.5f,0.5f,0.5f,1}, texture, 1, &material);
        renderer.endFrame();
        target.resolveMultisampled();
        target.bind();
        std::array<unsigned char,4> color{};
        glReadPixels(target.getWidth()/2,target.getHeight()/2,1,1,GL_RGBA,GL_UNSIGNED_BYTE,color.data());
        return color;
    };
    bool passed = true;
    auto check = [&](const char* name, bool result) {
        std::cout << name << (result ? " PASS" : " FAIL") << std::endl;
        passed &= result;
    };
    settings.sunEnabled = settings.pointEnabled = false;
    settings.ambient = 1;
    check("sRGB round trip", std::abs(int(pixel()[0])-128) <= 1);
    texture = &swatch;
    check("sRGB diffuse times base in linear space", std::abs(int(pixel()[0])-61) <= 2);
    texture = nullptr;
    settings.ambient = 0.5f;
    check("linear half irradiance", std::abs(int(pixel()[0])-92) <= 2);
    settings.ambient = 0;
    settings.sunEnabled = true;
    settings.direction = {0,0,-1};
    settings.sunColor = {1,0,0};
    settings.sunIntensity = 1;
    auto sun = pixel();
    check("directional red", sun[0]>95 && sun[1]==0);
    model = glm::scale(glm::mat4(1), {3,1,1});
    check("inverse transpose", pixel()[0]>sun[0]+10);
    model = glm::scale(glm::mat4(1), {1,1,0});
    check("singular model", pixel()[0]==0);
    model = glm::mat4(1);
    material.specularColor = glm::vec3(1);
    material.shininess = 4;
    const int wideHighlight = pixel()[0];
    material.shininess = 96;
    check("adjustable specular lobe", wideHighlight > pixel()[0]+20);
    settings.direction = {0,0,1};
    check("back face specular gate", pixel()[0]==0);
    settings.sunEnabled = false;
    settings.pointEnabled = true;
    settings.pointColor = {0,1,0};
    settings.position = {0,0,1};
    auto near = pixel();
    settings.position.z = 4;
    auto far = pixel();
    check("point light attenuation", near[1]>far[1]+10 && far[1]>0 && near[0]==0);
    GLint limit = 0;
    glGetIntegerv(GL_MAX_SAMPLES,&limit);
    for (int n : {2,4,1}) {
        requested = n;
        auto resolved = pixel();
        check("sample count and resolved color", actual >= std::min(n,limit) && std::abs(int(resolved[1])-far[1])<=2);
        std::cout << "requested=" << n << " actual=" << actual << std::endl;
    }
    requested = 4;
    pixel();
    check("resize multisampled target", target.resize(97,97) && std::abs(int(pixel()[1])-far[1])<=2);
    requested = 1;
    std::array<std::array<unsigned char,4>,4> presets{};
    for (int i = 0; i < 4; ++i) {
        settings = LightingSettings::preset(static_cast<LightingPreset>(i));
        presets[i] = pixel();
        for (int j = 0; j < i; ++j) check("distinct preset radiance", presets[i] != presets[j]);
    }
    settings = LightingSettings::preset(LightingPreset::Daylight);
    check("reset to Daylight", pixel()==presets[0]);
    // Nhìn thẳng xuống sàn. Pháp tuyến nghiêng ban đầu được ép gần +Z trước khi xoay sang +Y.
    floor = true;
    cam.lookAt({0,3,0}, {0,0,0}, {0,0,-1});
    model = glm::rotate(glm::mat4(1),glm::radians(-90.0f),glm::vec3(1,0,0))
            * glm::scale(glm::mat4(1),glm::vec3(4,1,0.1f));
    settings.sunEnabled = false;
    settings.ambient = 1;
    const auto noGrid = pixel();
    gridStep = 0.5f;
    check("grid at world origin", pixel()[0]+8 < noGrid[0]);
    check("zero GL errors", glGetError() == GL_NO_ERROR);
    return passed ? 0 : 1;
}
