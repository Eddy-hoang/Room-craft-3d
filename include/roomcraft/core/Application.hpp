#pragma once

#include "roomcraft/core/Window.hpp"
#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/graphics/Framebuffer.hpp"
#include "roomcraft/graphics/Mesh.hpp"
#include "roomcraft/graphics/Renderer.hpp"
#include "roomcraft/graphics/Shader.hpp"
#include "roomcraft/scene/Transform.hpp"

#include <glm/vec4.hpp>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace roomcraft::core {

// Chế độ chạy của ứng dụng RoomCraft 3D tại Milestone M1
enum class ApplicationMode {
    InteractiveM1, // Ứng dụng Graphics Engine M1 đầy đủ (nhiều cube 3D, camera, FBO, Grid, Diagnostics)
    SmokeTestM0,   // Regression smoke test của M0 (tam giác cơ bản + resize + ImGui)
    SmokeTestM1    // Kiểm thử tích hợp & runtime tự động toàn bộ Core Graphics Engine M1
};

// Cấu hình khởi tạo cho Application
struct ApplicationConfig {
    int width = 1280;
    int height = 720;
    std::string title = "RoomCraft 3D - M1 Core Graphics Engine (OpenGL 3.3 Core)";
    ApplicationMode mode = ApplicationMode::InteractiveM1;
    int smokeTestFrames = 10;
};

// Đối tượng lập phương thử nghiệm trong demo scene của M1 (chưa phải SceneObject hoàn chỉnh của M4)
struct DemoCubeObject {
    std::string name;
    scene::Transform transform;
    scene::Transform defaultTransform;
    glm::vec4 color{1.0f};
};

// Lớp Application điều phối Window, Renderer, Framebuffer, Camera, Dear ImGui và vòng đời tài nguyên GPU.
class Application {
public:
    explicit Application(ApplicationConfig config);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Khởi tạo cửa sổ, OpenGL 3.3 Core, GLAD, tài nguyên đồ họa và Dear ImGui
    bool initialize(int& argc, char** argv);

    // Chạy vòng lặp chính; trả về 0 nếu thành công
    int run();

private:
    bool initImGui();
    bool initM0Resources();
    bool initM1Resources();
    void setupDefaultDemoScene();

    // Giải phóng toàn bộ tài nguyên GPU & ImGui trước khi OpenGL context bị hủy
    void shutdownGraphics();

    // Tiện ích tìm đường dẫn tài nguyên tương đối (không hardcode đường dẫn máy phát triển)
    std::filesystem::path resolveAssetPath(const std::filesystem::path& relativePath) const;

    // Vòng lặp render theo từng chế độ
    void renderFrame();
    void renderM0RegressionFrame();
    void renderM1InteractiveFrame();
    void renderM1DemoSceneToFramebuffer(float faceContrast);
    void renderM1DiagnosticsUi();
    void renderM1ViewportUi();

    // Các bước kiểm thử tự động cho M1 Integration & Runtime Smoke Test
    bool runM1IntegrationSuiteOnce();
    bool verifyM1DepthTestingOrderIndependence();
    bool verifyM1FramebufferResizeAndZeroSafety();
    bool verifyM1RaiiMoveLifecycle();
    bool verifyM1MultiCubeRenderedPixels();

    // Xử lý sự kiện từ Window
    void handleReshape(int width, int height);
    void handleKeyboard(unsigned char key, int x, int y);
    void handleClose();
    void drainAndCheckOpenGlErrors();

private:
    ApplicationConfig m_config;
    std::filesystem::path m_executableDir;

    Window m_window;
    graphics::Renderer m_renderer;
    graphics::Camera m_camera;
    graphics::Framebuffer m_framebuffer;

    // Tài nguyên M1 (RAII)
    graphics::Shader m_meshShader;
    graphics::Shader m_gridShader;
    graphics::Mesh m_cubeMesh;
    graphics::Mesh m_gridMesh;
    graphics::Mesh m_axisXMesh;
    graphics::Mesh m_axisYMesh;
    graphics::Mesh m_axisZMesh;
    std::vector<DemoCubeObject> m_demoCubes;

    // Tài nguyên M0 regression (giữ nguyên để kiểm chứng không phá vỡ M0)
    graphics::Shader m_m0BasicShader;
    unsigned int m_m0Vao = 0;
    unsigned int m_m0Vbo = 0;

    // Trạng thái điều khiển UI & render
    bool m_initialized = false;
    bool m_imguiInitialized = false;
    bool m_depthTestEnabled = true;
    bool m_showGridAndAxes = true;
    bool m_showM0Triangle = true;
    int m_selectedCubeIndex = 0;
    glm::vec4 m_clearColor{0.094f, 0.102f, 0.125f, 1.0f}; // #181A20

    // Thống kê FPS & thời gian khung hình thực tế
    int m_frameCount = 0;
    int m_lastFpsTimeMs = 0;
    int m_lastFrameTimeMs = 0;
    int m_framesSinceLastFps = 0;
    float m_currentFps = 0.0f;
    float m_frameTimeMs = 0.0f;
    bool m_glErrorDetected = false;

    // Trạng thái phục vụ M0 Regression Smoke Test
    bool m_m0ResizeTriggered = false;
    bool m_m0ResizeObserved = false;
    bool m_m0PixelVerified = false;
    std::array<unsigned char, 4> m_m0SampledPixel{0, 0, 0, 0};

    // Trạng thái phục vụ M1 Integration & Runtime Smoke Test
    bool m_m1SuiteExecuted = false;
    bool m_m1SuitePassed = false;
    bool m_m1MultiCubePixelsVerified = false;
};

} // namespace roomcraft::core
