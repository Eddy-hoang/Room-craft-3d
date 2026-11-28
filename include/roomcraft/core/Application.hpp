#pragma once

#include "roomcraft/assets/AssetManager.hpp"
#include "roomcraft/core/InputManager.hpp"
#include "roomcraft/core/Window.hpp"
#include "roomcraft/editor/EditorTheme.hpp"
#include "roomcraft/editor/RoomEditor.hpp"
#include "roomcraft/geometry/BezierCurve.hpp"
#include "roomcraft/geometry/BezierSurface.hpp"
#include "roomcraft/geometry/FurnitureGenerator.hpp"
#include "roomcraft/geometry/MeshData.hpp"
#include "roomcraft/graphics/Camera.hpp"
#include "roomcraft/graphics/Framebuffer.hpp"
#include "roomcraft/graphics/Mesh.hpp"
#include "roomcraft/graphics/Renderer.hpp"
#include "roomcraft/graphics/Shader.hpp"
#include "roomcraft/graphics/ThumbnailRenderer.hpp"
#include "roomcraft/math/BoundingBox.hpp"
#include "roomcraft/scene/Transform.hpp"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace roomcraft::core {

// Chế độ chạy của ứng dụng RoomCraft 3D tại Milestone M6
enum class ApplicationMode {
    InteractiveApp, // Giao diện sản phẩm RoomCraft 3D (mặc định mở Room Editor; Geometry Lab/Asset Inspector/DevTools trong menu Tools)
    SmokeTestM0,    // Regression smoke test M0
    SmokeTestM1,    // Regression GPU integration smoke test M1
    SmokeTestM2,    // Regression GPU integration smoke test M2
    SmokeTestM3,    // Regression GPU integration smoke test M3
    SmokeTestM4,    // Regression GPU integration smoke test M4
    BenchmarkM4,    // Đo hiệu năng 10 / 30 / 100 vật thể tại 1280x720
    SmokeTestM5,    // Regression GPU integration smoke test M5
    SmokeTestM6,    // GPU integration & UI/Thumbnail/Gizmo/Workspace smoke test M6
    SmokeTestM8,    // GPU integration smoke test M8 (Collision, Persistence, Reload, Picking/Gizmo)
    SmokeTestM9,    // GPU integration smoke test M9 (Directional Shadow Mapping, Preferences, Resource Lifetime)
    BenchmarkM9,    // Reproducible CPU/GPU/Wall-clock benchmark suite (30/100/250, MSAA, Orbit, Gizmo, Reload, Save/Load, Stability)
    CaptureEvidenceM9 // Chụp ảnh GPU thật (PNG) cho M9 visual acceptance & xuất 3 dự án mẫu examples/*.roomcraft
};

// Không gian làm việc hiện hành trong ứng dụng sản phẩm (loại bỏ hoàn toàn tên milestone trên UI)
enum class InteractiveWorkspace {
    RoomEditor = 0,          // Không gian thiết kế phòng mặc định
    AssetInspector = 1,      // Tools -> Asset Inspector
    GeometryLab = 2,         // Tools -> Geometry Lab
    GraphicsReference = 3    // Tools -> Developer Tools -> Graphics Reference Scene
};

// Nhóm đối tượng trong Geometry Lab
enum class PlaygroundCategory {
    Primitives = 0,
    Parametric = 1,
    Furniture = 2
};

struct ApplicationConfig {
    int width = 1280;
    int height = 720;
    std::string title = "RoomCraft 3D — 3D Interior Design Studio";
    ApplicationMode mode = ApplicationMode::InteractiveApp;
    int smokeTestFrames = 10;
    std::filesystem::path outputDir;
};

struct DemoCubeObject {
    std::string name;
    scene::Transform transform;
    scene::Transform defaultTransform;
    glm::vec4 color{1.0f};
};

struct UploadedRenderPart {
    std::string name;
    graphics::Mesh mesh;
    scene::Transform localTransform;
    glm::vec4 color{0.75f, 0.75f, 0.75f, 1.0f};
    bool isLinePrimitive = false;
};

// Lớp Application điều phối Window, Renderer, Framebuffer, Camera, AssetManager, Dear ImGui
// cho cả Asset Browser (M3), Geometry Playground (M2) và M1/M0 regression tests.
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
    bool initEngineResources();
    void setupDefaultM1DemoScene();
    void resetCurrentPlaygroundParameters();

    // Asset Browser (M3) helpers
    bool selectBrowserAsset(const std::string& assetId, bool autoFitCamera = true);
    void fitCameraToActiveModel();

    // Geometry Playground (M2) helpers
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
    void renderInteractiveFrame(graphics::Framebuffer* compositeTarget = nullptr);
    void renderM1DemoSceneToFramebuffer(float faceContrast);
    void renderPlaygroundSceneToFramebuffer(float faceContrast);
    void renderAssetBrowserSceneToFramebuffer(float faceContrast);
    void renderGpuAssetToCurrentTarget(const assets::GpuAsset& asset,
                                       const glm::mat4& parentMatrix,
                                       const graphics::Camera& cam,
                                       float faceContrast);

    // Giao diện Dear ImGui (Product Menu Bar + Workspaces + Dialogs)
    void switchWorkspace(InteractiveWorkspace target);
    void applyUiScale(float scale);
    void renderProductMainMenuBar();
    void renderProductDialogs();
    void renderAssetBrowserUi();
    void renderPlaygroundControlUi();
    void renderM1ReferenceControlUi();
    void renderViewportPanel(const char* panelTitle, float posX, float posY, float width, float height, bool showTopToolbar);

    // Regression & Integration Suites (M1, M2, M3)
    bool runM1IntegrationSuiteOnce();
    bool verifyM1DepthTestingOrderIndependence();
    bool verifyM1FramebufferResizeAndZeroSafety();
    bool verifyM1RaiiMoveLifecycle();
    bool verifyM1MultiCubeRenderedPixels();

    bool runM2IntegrationSuiteOnce();
    bool verifyM2WireframeStateRestoration();
    bool verifyM2ProjectedGeometryRender(const std::string& label,
                                         const std::vector<UploadedRenderPart>& parts,
                                         const math::BoundingBox& box,
                                         const graphics::Camera& cam);

    bool runM3IntegrationSuiteOnce();
    bool verifyM3TexturedOrientationAndFallback();
    bool verifyM3MultiMaterialRendering();
    bool verifyM3CacheAndTransactionalReload();
    bool verifyM3GpuAssetRenderCoverage(const std::string& label, const assets::GpuAsset& asset);

    bool runM4IntegrationSuiteOnce();
    bool runM4BenchmarkSuiteOnce();
    bool benchmarkSingleObjectCount(std::size_t objectCount, int warmupFrames, int measuredFrames);
    bool runM5IntegrationSuiteOnce();
    bool runM6IntegrationSuiteOnce();
    bool runM8IntegrationSuiteOnce();
    bool runM9IntegrationSuiteOnce();
    bool runM9BenchmarkSuiteOnce();
    bool runM9CaptureEvidenceOnce();
    void loadUserPreferences();
    void saveUserPreferences() const;

    // Sự kiện cửa sổ
    void handleReshape(int width, int height);
    void handleKeyboard(unsigned char key, int x, int y);
    void handleClose();
    void drainAndCheckOpenGlErrors();

private:
    ApplicationConfig m_config;
    std::filesystem::path m_executableDir;

    Window m_window;
    InputManager m_inputManager;
    graphics::Renderer m_renderer;
    graphics::Camera m_camera;
    graphics::Camera m_savedRoomCamera;
    bool m_hasSavedRoomCamera = false;
    graphics::Framebuffer m_framebuffer;
    graphics::ThumbnailRenderer m_thumbnailRenderer;
    assets::AssetManager m_assetManager;
    editor::FontLoadInfo m_fontInfo;
    float m_uiScale = 1.0f;

    // Tài nguyên đồ họa dùng chung
    graphics::Shader m_meshShader;
    graphics::Shader m_gridShader;
    graphics::Shader m_shadowShader;
    graphics::Mesh m_cubeMesh;
    graphics::Mesh m_controlMarkerMesh;
    graphics::Mesh m_gridMesh;
    graphics::Mesh m_axisXMesh;
    graphics::Mesh m_axisYMesh;
    graphics::Mesh m_axisZMesh;
    std::vector<DemoCubeObject> m_demoCubes;

    // Trạng thái Room Editor & Workspace hiện hành
    editor::RoomEditor m_roomEditor;
    InteractiveWorkspace m_workspace = InteractiveWorkspace::RoomEditor;
    bool m_showPerformanceWindow = false;
    bool m_showOpenGlDiagnosticsWindow = false;
    bool m_showShortcutsModal = false;
    bool m_showUserGuideModal = false;
    bool m_showSystemInfoModal = false;
    bool m_showAboutModal = false;
    int m_browserSourceFilter = 0;   // 0=All, 1=Procedural, 2=Blender
    int m_browserCategoryFilter = 0; // 0=All, 1..7=AssetCategory
    std::array<char, 128> m_browserSearchBuf{};
    std::string m_selectedAssetId = "blender-modern-sofa";
    std::shared_ptr<assets::GpuAsset> m_activeBrowserAsset;
    std::string m_lastAssetError;
    std::string m_lastAssetStatus;

    // Trạng thái Geometry Playground (M2)
    PlaygroundCategory m_category = PlaygroundCategory::Furniture;
    int m_primitiveTypeIndex = 3;
    int m_parametricTypeIndex = 1;
    int m_furnitureTypeIndex = 0;
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

    // Tham số tương tác chung
    bool m_autoRegenerate = true;
    bool m_wireframeMode = false;
    bool m_depthTestEnabled = true;
    bool m_showGrid = true;
    bool m_showAxes = true;
    bool m_showBoundingBox = true;
    bool m_showControlOverlay = true;
    bool m_showVertexNormals = false;
    float m_normalVisualLength = 0.06f;

    // Tham số Primitives & Parametric & Furniture (M2)
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

    bool m_initialized = false;
    bool m_imguiInitialized = false;
    int m_selectedCubeIndex = 0;
    glm::vec4 m_clearColor{0.094f, 0.102f, 0.125f, 1.0f}; // #181A20

    int m_frameCount = 0;
    int m_lastFpsTimeMs = 0;
    int m_lastFrameTimeMs = 0;
    int m_framesSinceLastFps = 0;
    float m_currentFps = 0.0f;
    float m_frameTimeMs = 0.0f;
    bool m_glErrorDetected = false;

    // Trạng thái Smoke Tests
    bool m_m0ResizeTriggered = false;
    bool m_m0ResizeObserved = false;
    bool m_m0PixelVerified = false;
    std::array<unsigned char, 4> m_m0SampledPixel{0, 0, 0, 0};

    bool m_m1SuiteExecuted = false;
    bool m_m1SuitePassed = false;
    bool m_m1MultiCubePixelsVerified = false;

    bool m_m2SuiteExecuted = false;
    bool m_m2SuitePassed = false;

    bool m_m3SuiteExecuted = false;
    bool m_m3SuitePassed = false;

    bool m_m4SuiteExecuted = false;
    bool m_m4SuitePassed = false;

    bool m_m4BenchmarkExecuted = false;
    bool m_m4BenchmarkPassed = false;

    bool m_m5SuiteExecuted = false;
    bool m_m5SuitePassed = false;

    bool m_m6SuiteExecuted = false;
    bool m_m6SuitePassed = false;

    bool m_m8SuiteExecuted = false;
    bool m_m8SuitePassed = false;

    bool m_m9SuiteExecuted = false;
    bool m_m9SuitePassed = false;

    bool m_m9BenchmarkExecuted = false;
    bool m_m9BenchmarkPassed = false;

    bool m_m9CaptureExecuted = false;
    bool m_m9CapturePassed = false;
};

} // namespace roomcraft::core
