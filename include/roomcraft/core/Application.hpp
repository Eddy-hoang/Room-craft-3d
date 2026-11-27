#pragma once

#include "roomcraft/core/Window.hpp"
#include "roomcraft/geometry/BezierCurve.hpp"
#include "roomcraft/geometry/BezierSurface.hpp"
#include "roomcraft/geometry/FurnitureGenerator.hpp"
#include "roomcraft/geometry/MeshData.hpp"
#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/graphics/Framebuffer.hpp"
#include "roomcraft/graphics/Mesh.hpp"
#include "roomcraft/graphics/Renderer.hpp"
#include "roomcraft/graphics/Shader.hpp"
#include "roomcraft/math/BoundingBox.hpp"
#include "roomcraft/scene/Transform.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace roomcraft::core {

// Chế độ chạy của ứng dụng RoomCraft 3D tại Milestone M2
enum class ApplicationMode {
    InteractivePlayground, // Chế độ Geometry Playground M2 (mặc định khi mở ứng dụng)
    SmokeTestM0,           // Regression smoke test M0 (tam giác cơ bản + resize + ImGui)
    SmokeTestM1,           // Regression GPU integration smoke test M1 (depth test, FBO, 3 cube)
    SmokeTestM2            // GPU integration & runtime smoke test M2 (primitives, Bézier, revolution, furniture, wireframe)
};

// Nhóm đối tượng trong Geometry Playground
enum class PlaygroundCategory {
    Primitives = 0,
    Parametric = 1,
    Furniture = 2,
    M1ReferenceScene = 3
};

// Cấu hình khởi tạo cho Application
struct ApplicationConfig {
    int width = 1280;
    int height = 720;
    std::string title = "RoomCraft 3D - M2 Procedural Geometry Playground (OpenGL 3.3 Core)";
    ApplicationMode mode = ApplicationMode::InteractivePlayground;
    int smokeTestFrames = 10;
};

// Đối tượng lập phương thử nghiệm giữ lại cho M1 Reference Scene & M1 Regression
struct DemoCubeObject {
    std::string name;
    scene::Transform transform;
    scene::Transform defaultTransform;
    glm::vec4 color{1.0f};
};

// Một thành phần GPU đã upload của mô hình sinh thủ tục hiện tại (tránh upload lại mỗi frame)
struct UploadedRenderPart {
    std::string name;
    graphics::Mesh mesh;
    scene::Transform localTransform;
    glm::vec4 color{0.75f, 0.75f, 0.75f, 1.0f};
    bool isLinePrimitive = false;
};

// Lớp Application điều phối Window, Renderer, Framebuffer, Camera, Dear ImGui và Geometry Playground M2.
class Application {
public:
    explicit Application(ApplicationConfig config);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool initialize(int& argc, char** argv);
    int run();

private:
    bool initImGui();
    bool initM0Resources();
    bool initM1AndM2Resources();
    void setupDefaultM1DemoScene();
    void resetCurrentPlaygroundParameters();

    // Tái sinh hình học CPU và upload GPU duy nhất khi tham số/loại thay đổi (KHÔNG gọi mỗi frame)
    bool regenerateActivePlaygroundGeometry();
    bool uploadPlaygroundParts(std::vector<geometry::FurniturePart>& parts,
                               const math::BoundingBox& overallBox,
                               const geometry::MeshData* controlOverlayLines,
                               const std::vector<glm::vec3>& controlPoints,
                               std::string& outError);

    void shutdownGraphics();
    std::filesystem::path resolveAssetPath(const std::filesystem::path& relativePath) const;

    // Vòng lặp render
    void renderFrame();
    void renderM0RegressionFrame();
    void renderInteractiveFrame();
    void renderM1DemoSceneToFramebuffer(float faceContrast);
    void renderPlaygroundSceneToFramebuffer(float faceContrast);
    void renderPlaygroundControlUi();
    void renderViewportUi();

    // Kiểm thử tự động M1 Regression
    bool runM1IntegrationSuiteOnce();
    bool verifyM1DepthTestingOrderIndependence();
    bool verifyM1FramebufferResizeAndZeroSafety();
    bool verifyM1RaiiMoveLifecycle();
    bool verifyM1MultiCubeRenderedPixels();

    // Kiểm thử tự động M2 GPU Integration Smoke Test
    bool runM2IntegrationSuiteOnce();
    bool verifyM2WireframeStateRestoration();
    bool verifyM2ProjectedGeometryRender(const std::string& label,
                                         const std::vector<UploadedRenderPart>& parts,
                                         const math::BoundingBox& box,
                                         const graphics::Camera& cam);

    // Sự kiện cửa sổ
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

    // Tài nguyên đồ họa nền (M1 + M2)
    graphics::Shader m_meshShader;
    graphics::Shader m_gridShader;
    graphics::Mesh m_cubeMesh;
    graphics::Mesh m_controlMarkerMesh;
    graphics::Mesh m_gridMesh;
    graphics::Mesh m_axisXMesh;
    graphics::Mesh m_axisYMesh;
    graphics::Mesh m_axisZMesh;
    std::vector<DemoCubeObject> m_demoCubes;

    // Tài nguyên Geometry Playground hiện đang hiển thị (chỉ cập nhật khi regenerate)
    PlaygroundCategory m_category = PlaygroundCategory::Furniture;
    int m_primitiveTypeIndex = 3;   // Cylinder
    int m_parametricTypeIndex = 1;  // Bezier Surface
    int m_furnitureTypeIndex = 0;   // Desk
    std::vector<UploadedRenderPart> m_activeParts;
    graphics::Mesh m_activeBBoxLines;
    graphics::Mesh m_activeControlLines;
    graphics::Mesh m_activeNormalLines;
    std::vector<glm::vec3> m_activeControlPoints;
    math::BoundingBox m_activeBoundingBox;
    scene::Transform m_rootTransform;
    std::string m_lastGeometryError;
    double m_lastGenerationTimeMs = 0.0;
    std::uint32_t m_activeVertexCount = 0;
    std::uint32_t m_activeIndexCount = 0;
    std::uint32_t m_activeTriangleCount = 0;

    // Tham số tương tác cho Geometry Playground
    bool m_autoRegenerate = true;
    bool m_wireframeMode = false;
    bool m_depthTestEnabled = true;
    bool m_showGrid = true;
    bool m_showAxes = true;
    bool m_showBoundingBox = false;
    bool m_showControlOverlay = true;
    bool m_showVertexNormals = false;
    float m_normalVisualLength = 0.08f;

    // Tham số Primitives
    float m_planeWidth = 2.0f;
    float m_planeDepth = 2.0f;
    int m_planeSubX = 4;
    int m_planeSubZ = 4;
    float m_cubeSize = 1.0f;
    float m_cuboidW = 1.4f;
    float m_cuboidH = 0.8f;
    float m_cuboidD = 0.9f;
    float m_cylRadius = 0.45f;
    float m_cylHeight = 1.2f;
    int m_cylSegments = 24;
    bool m_cylCapped = true;
    float m_sphereRadius = 0.55f;
    int m_sphereLatSeg = 18;
    int m_sphereLonSeg = 28;
    float m_coneRadius = 0.50f;
    float m_coneHeight = 1.15f;
    int m_coneSegments = 24;
    bool m_coneCapped = true;
    float m_frustumBottomR = 0.55f;
    float m_frustumTopR = 0.25f;
    float m_frustumHeight = 1.0f;
    int m_frustumSegments = 24;
    bool m_frustumCapBottom = true;
    bool m_frustumCapTop = true;

    // Tham số Parametric
    std::array<glm::vec3, 4> m_bezierCurvePts = {
        glm::vec3(-1.0f, 0.2f, 0.0f),
        glm::vec3(-0.4f, 1.5f, 0.4f),
        glm::vec3( 0.4f, 1.5f, -0.4f),
        glm::vec3( 1.0f, 0.2f, 0.0f)
    };
    int m_bezierCurveSegments = 32;
    float m_bezierSurfWidth = 1.8f;
    float m_bezierSurfDepth = 1.8f;
    float m_bezierSurfCurvature = 0.55f;
    int m_bezierSurfUSeg = 16;
    int m_bezierSurfVSeg = 16;
    float m_revHeight = 1.1f;
    float m_revBaseR = 0.22f;
    float m_revBellyR = 0.48f;
    float m_revNeckR = 0.16f;
    float m_revRimR = 0.28f;
    int m_revRadialSeg = 32;
    int m_revProfileSeg = 20;
    bool m_revCapBottom = true;
    bool m_revCapTop = false;

    // Tham số 6 mẫu Furniture
    geometry::DeskParams m_deskParams;
    geometry::ChairParams m_chairParams;
    geometry::BookshelfParams m_bookshelfParams;
    geometry::DeskLampParams m_lampParams;
    geometry::VaseParams m_vaseParams;
    geometry::CurvedChairParams m_curvedChairParams;

    // Tài nguyên M0 regression
    graphics::Shader m_m0BasicShader;
    unsigned int m_m0Vao = 0;
    unsigned int m_m0Vbo = 0;
    bool m_showM0Triangle = true;

    // Trạng thái chung
    bool m_initialized = false;
    bool m_imguiInitialized = false;
    int m_selectedCubeIndex = 0;
    glm::vec4 m_clearColor{0.094f, 0.102f, 0.125f, 1.0f}; // #181A20

    // Thống kê thời gian & FPS thực tế
    int m_frameCount = 0;
    int m_lastFpsTimeMs = 0;
    int m_lastFrameTimeMs = 0;
    int m_framesSinceLastFps = 0;
    float m_currentFps = 0.0f;
    float m_frameTimeMs = 0.0f;
    bool m_glErrorDetected = false;

    // Trạng thái Smoke Tests (M0, M1, M2)
    bool m_m0ResizeTriggered = false;
    bool m_m0ResizeObserved = false;
    bool m_m0PixelVerified = false;
    std::array<unsigned char, 4> m_m0SampledPixel{0, 0, 0, 0};

    bool m_m1SuiteExecuted = false;
    bool m_m1SuitePassed = false;
    bool m_m1MultiCubePixelsVerified = false;

    bool m_m2SuiteExecuted = false;
    bool m_m2SuitePassed = false;
};

} // namespace roomcraft::core
