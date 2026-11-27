#include "roomcraft/core/Application.hpp"

#include "roomcraft/geometry/GeometryUtils.hpp"
#include "roomcraft/geometry/PrimitiveGenerator.hpp"
#include "roomcraft/geometry/RevolutionSurface.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_glut.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace roomcraft::core {

namespace {

WindowConfig makeWindowConfig(const ApplicationConfig& cfg)
{
    WindowConfig wc;
    wc.width = cfg.width;
    wc.height = cfg.height;
    wc.title = cfg.title;
    return wc;
}

} // namespace

Application::Application(ApplicationConfig config)
    : m_config(std::move(config))
    , m_window(makeWindowConfig(m_config))
{
}

Application::~Application()
{
    shutdownGraphics();
}

bool Application::initialize(int& argc, char** argv)
{
    if (argc > 0 && argv != nullptr && argv[0] != nullptr) {
        std::error_code ec;
        const auto exePath = std::filesystem::weakly_canonical(std::filesystem::path(argv[0]), ec);
        if (!ec && exePath.has_parent_path()) {
            m_executableDir = exePath.parent_path();
        } else {
            m_executableDir = std::filesystem::current_path();
        }
    } else {
        m_executableDir = std::filesystem::current_path();
    }

    if (!m_window.initialize(argc, argv)) {
        return false;
    }

    if (m_config.mode == ApplicationMode::SmokeTestM0) {
        if (!initM0Resources()) {
            shutdownGraphics();
            return false;
        }
    } else {
        if (!initM1AndM2Resources()) {
            shutdownGraphics();
            return false;
        }
    }

    if (!initImGui()) {
        shutdownGraphics();
        return false;
    }

    WindowCallbacks callbacks;
    callbacks.onDisplay = [this]() { renderFrame(); };
    callbacks.onReshape = [this](int w, int h) { handleReshape(w, h); };
    callbacks.onClose = [this]() { handleClose(); };
    callbacks.onKeyboard = [this](unsigned char key, int x, int y) { handleKeyboard(key, x, y); };
    callbacks.onKeyboardUp = [this](unsigned char key, int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_KeyboardUpFunc(key, x, y);
        }
    };
    callbacks.onSpecial = [this](int key, int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_SpecialFunc(key, x, y);
        }
    };
    callbacks.onSpecialUp = [this](int key, int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_SpecialUpFunc(key, x, y);
        }
    };
    callbacks.onMouse = [this](int button, int state, int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_MouseFunc(button, state, x, y);
        }
    };
    callbacks.onMouseWheel = [this](int wheel, int direction, int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_MouseWheelFunc(wheel, direction, x, y);
        }
    };
    callbacks.onMotion = [this](int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_MotionFunc(x, y);
        }
    };
    callbacks.onPassiveMotion = [this](int x, int y) {
        if (m_imguiInitialized) {
            ImGui_ImplGLUT_MotionFunc(x, y);
        }
    };
    m_window.setCallbacks(std::move(callbacks));

    const int nowMs = m_window.getElapsedTimeMs();
    m_lastFpsTimeMs = nowMs;
    m_lastFrameTimeMs = nowMs;
    m_initialized = true;

    std::cout << "[RoomCraft3D] Application initialized successfully." << std::endl;
    return true;
}

int Application::run()
{
    if (!m_initialized) {
        std::cerr << "[RoomCraft3D][ERROR] Cannot run Application before initialize() succeeds." << std::endl;
        return 1;
    }

    m_window.runMainLoop();
    shutdownGraphics();

    if (m_config.mode == ApplicationMode::SmokeTestM0) {
        std::cout << "[SMOKE] Frames rendered: " << m_frameCount << std::endl;
        std::cout << "[SMOKE] Triangle pixel verified: "
                  << (m_m0PixelVerified ? "YES" : "NO")
                  << " (Center RGB="
                  << static_cast<int>(m_m0SampledPixel[0]) << ","
                  << static_cast<int>(m_m0SampledPixel[1]) << ","
                  << static_cast<int>(m_m0SampledPixel[2]) << ")" << std::endl;
        std::cout << "[SMOKE] Resize verified: " << (m_m0ResizeObserved ? "YES" : "NO")
                  << " (" << m_window.getWidth() << "x" << m_window.getHeight() << ")" << std::endl;
        std::cout << "[SMOKE] OpenGL errors: " << (m_glErrorDetected ? "YES" : "NONE") << std::endl;

        const bool passed = (m_frameCount >= m_config.smokeTestFrames) &&
                            m_m0PixelVerified &&
                            m_m0ResizeObserved &&
                            !m_glErrorDetected;
        std::cout << "[SMOKE] Result: " << (passed ? "PASS" : "FAIL") << std::endl;
        return passed ? 0 : 1;
    }

    if (m_config.mode == ApplicationMode::SmokeTestM1) {
        const auto& stats = m_renderer.getLastFrameStats();
        std::cout << "[M1-SMOKE] Frames rendered       : " << m_frameCount << std::endl;
        std::cout << "[M1-SMOKE] Last frame draw calls : " << stats.drawCalls
                  << " (vertices=" << stats.vertexCount
                  << ", indices=" << stats.indexCount << ")" << std::endl;
        std::cout << "[M1-SMOKE] Integration suite     : " << (m_m1SuitePassed ? "PASS" : "FAIL") << std::endl;
        std::cout << "[M1-SMOKE] Multi-cube pixels     : " << (m_m1MultiCubePixelsVerified ? "PASS" : "FAIL") << std::endl;
        std::cout << "[M1-SMOKE] OpenGL errors         : " << (m_glErrorDetected ? "YES" : "NONE") << std::endl;

        const bool passed = (m_frameCount >= m_config.smokeTestFrames) &&
                            m_m1SuiteExecuted &&
                            m_m1SuitePassed &&
                            m_m1MultiCubePixelsVerified &&
                            !m_glErrorDetected;
        std::cout << "[M1-SMOKE] Final Result          : " << (passed ? "PASS" : "FAIL") << std::endl;
        return passed ? 0 : 1;
    }

    if (m_config.mode == ApplicationMode::SmokeTestM2) {
        const auto& stats = m_renderer.getLastFrameStats();
        std::cout << "[M2-SMOKE] Frames rendered       : " << m_frameCount << std::endl;
        std::cout << "[M2-SMOKE] Last frame draw calls : " << stats.drawCalls
                  << " (vertices=" << stats.vertexCount
                  << ", indices=" << stats.indexCount
                  << ", triangles=" << stats.triangleCount << ")" << std::endl;
        std::cout << "[M2-SMOKE] Integration suite     : " << (m_m2SuitePassed ? "PASS" : "FAIL") << std::endl;
        std::cout << "[M2-SMOKE] OpenGL errors         : " << (m_glErrorDetected ? "YES" : "NONE") << std::endl;

        const bool passed = (m_frameCount >= m_config.smokeTestFrames) &&
                            m_m2SuiteExecuted &&
                            m_m2SuitePassed &&
                            !m_glErrorDetected;
        std::cout << "[M2-SMOKE] Final Result          : " << (passed ? "PASS" : "FAIL") << std::endl;
        return passed ? 0 : 1;
    }

    return 0;
}

bool Application::initImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.133f, 0.145f, 0.176f, 0.96f);      // #22252D
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.173f, 0.188f, 0.227f, 1.0f);  // #2C303A
    style.Colors[ImGuiCol_Button] = ImVec4(0.235f, 0.275f, 0.365f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.455f, 0.584f, 0.839f, 1.0f);  // #7495D6

    if (!ImGui_ImplGLUT_Init()) {
        std::cerr << "[RoomCraft3D][ERROR] ImGui_ImplGLUT_Init() failed." << std::endl;
        ImGui::DestroyContext();
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
        std::cerr << "[RoomCraft3D][ERROR] ImGui_ImplOpenGL3_Init() failed." << std::endl;
        ImGui_ImplGLUT_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    ImGui_ImplGLUT_ReshapeFunc(m_window.getWidth(), m_window.getHeight());
    m_imguiInitialized = true;
    std::cout << "[RoomCraft3D] Dear ImGui initialized (imgui_impl_glut + imgui_impl_opengl3 #version 330 core)."
              << std::endl;
    return true;
}

bool Application::initM0Resources()
{
    const auto vertPath = resolveAssetPath("assets/shaders/basic.vert");
    const auto fragPath = resolveAssetPath("assets/shaders/basic.frag");

    if (!m_m0BasicShader.loadFromFiles(vertPath, fragPath)) {
        return false;
    }

    const float triangleVertices[] = {
        -0.55f, -0.45f, 0.0f,    0.92f, 0.34f, 0.34f,
         0.55f, -0.45f, 0.0f,    0.34f, 0.80f, 0.56f,
         0.00f,  0.55f, 0.0f,    0.45f, 0.58f, 0.84f
    };

    glGenVertexArrays(1, &m_m0Vao);
    glGenBuffers(1, &m_m0Vbo);

    glBindVertexArray(m_m0Vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_m0Vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

    constexpr GLsizei stride = static_cast<GLsizei>(6 * sizeof(float));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::cout << "[RoomCraft3D] Basic GLSL shader & triangle VAO/VBO initialized." << std::endl;
    return true;
}

bool Application::initM1AndM2Resources()
{
    if (!m_meshShader.loadFromFiles(resolveAssetPath("assets/shaders/mesh.vert"),
                                    resolveAssetPath("assets/shaders/mesh.frag"))) {
        return false;
    }

    if (!m_gridShader.loadFromFiles(resolveAssetPath("assets/shaders/grid.vert"),
                                    resolveAssetPath("assets/shaders/grid.frag"))) {
        return false;
    }

    // Cube fixture cho M1 Reference Scene & điểm đánh dấu control point cho Bézier Playground
    std::vector<graphics::Vertex> cubeVerts;
    std::vector<std::uint32_t> cubeIndices;
    graphics::Mesh::buildUnitCubeFixture(cubeVerts, cubeIndices);
    if (!m_cubeMesh.create(cubeVerts, cubeIndices, graphics::PrimitiveTopology::Triangles)) {
        return false;
    }

    geometry::MeshData markerData;
    if (!geometry::PrimitiveGenerator::createCube(0.055f, markerData) ||
        !m_controlMarkerMesh.create(markerData.vertices, markerData.indices, markerData.topology)) {
        return false;
    }

    // Grid trên mặt phẳng XZ và 3 trục tọa độ X, Y, Z bằng GL_LINES
    std::vector<graphics::Vertex> gridVerts;
    std::vector<std::uint32_t> gridIndices;
    graphics::Mesh::buildGridFixture(6, 0.5f, gridVerts, gridIndices);
    if (!m_gridMesh.create(gridVerts, gridIndices, graphics::PrimitiveTopology::Lines)) {
        return false;
    }

    std::vector<graphics::Vertex> axisVerts;
    std::vector<std::uint32_t> axisIndices;
    graphics::Mesh::buildAxisLineFixture({0.0f, 0.004f, 0.0f}, {2.2f, 0.004f, 0.0f}, axisVerts, axisIndices);
    if (!m_axisXMesh.create(axisVerts, axisIndices, graphics::PrimitiveTopology::Lines)) {
        return false;
    }
    graphics::Mesh::buildAxisLineFixture({0.0f, 0.0f, 0.0f}, {0.0f, 2.2f, 0.0f}, axisVerts, axisIndices);
    if (!m_axisYMesh.create(axisVerts, axisIndices, graphics::PrimitiveTopology::Lines)) {
        return false;
    }
    graphics::Mesh::buildAxisLineFixture({0.0f, 0.004f, 0.0f}, {0.0f, 0.004f, 2.2f}, axisVerts, axisIndices);
    if (!m_axisZMesh.create(axisVerts, axisIndices, graphics::PrimitiveTopology::Lines)) {
        return false;
    }

    if (!m_framebuffer.create(860, 640)) {
        return false;
    }

    setupDefaultM1DemoScene();

    if (m_config.mode == ApplicationMode::SmokeTestM1) {
        m_category = PlaygroundCategory::M1ReferenceScene;
        m_camera.reset();
    } else {
        // Góc nhìn gần vừa vặn để quan sát nội thất và hình học M2
        m_camera.setProjectionMode(graphics::ProjectionMode::Perspective);
        m_camera.lookAt({2.3f, 1.8f, 2.7f}, {0.0f, 0.55f, 0.0f}, {0.0f, 1.0f, 0.0f});
        if (!regenerateActivePlaygroundGeometry()) {
            return false;
        }
    }

    m_camera.setAspectRatio(static_cast<float>(m_framebuffer.getWidth()) /
                            static_cast<float>(m_framebuffer.getHeight()));

    std::cout << "[RoomCraft3D] M1/M2 Graphics & Geometry Playground resources initialized." << std::endl;
    return true;
}

void Application::setupDefaultM1DemoScene()
{
    m_demoCubes.clear();

    DemoCubeObject cubeA;
    cubeA.name = "Cube A (Front Red)";
    cubeA.transform = scene::Transform({0.25f, 0.50f, 1.10f}, {0.0f, 18.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    cubeA.defaultTransform = cubeA.transform;
    cubeA.color = {0.88f, 0.34f, 0.34f, 1.0f};
    m_demoCubes.push_back(cubeA);

    DemoCubeObject cubeB;
    cubeB.name = "Cube B (Back Blue)";
    cubeB.transform = scene::Transform({-0.45f, 0.65f, -0.55f}, {0.0f, -24.0f, 0.0f}, {1.35f, 1.30f, 1.35f});
    cubeB.defaultTransform = cubeB.transform;
    cubeB.color = {0.35f, 0.56f, 0.88f, 1.0f};
    m_demoCubes.push_back(cubeB);

    DemoCubeObject cubeC;
    cubeC.name = "Cube C (Scaled Green)";
    cubeC.transform = scene::Transform({1.95f, 0.40f, -0.35f}, {12.0f, 35.0f, 0.0f}, {0.85f, 0.80f, 1.45f});
    cubeC.defaultTransform = cubeC.transform;
    cubeC.color = {0.30f, 0.78f, 0.52f, 1.0f};
    m_demoCubes.push_back(cubeC);
}

void Application::resetCurrentPlaygroundParameters()
{
    m_planeWidth = 2.0f;
    m_planeDepth = 2.0f;
    m_planeSubX = 4;
    m_planeSubZ = 4;
    m_cubeSize = 1.0f;
    m_cuboidW = 1.4f;
    m_cuboidH = 0.8f;
    m_cuboidD = 0.9f;
    m_cylRadius = 0.45f;
    m_cylHeight = 1.2f;
    m_cylSegments = 24;
    m_cylCapped = true;
    m_sphereRadius = 0.55f;
    m_sphereLatSeg = 18;
    m_sphereLonSeg = 28;
    m_coneRadius = 0.50f;
    m_coneHeight = 1.15f;
    m_coneSegments = 24;
    m_coneCapped = true;
    m_frustumBottomR = 0.55f;
    m_frustumTopR = 0.25f;
    m_frustumHeight = 1.0f;
    m_frustumSegments = 24;
    m_frustumCapBottom = true;
    m_frustumCapTop = true;

    m_bezierCurvePts = {
        glm::vec3(-1.0f, 0.2f, 0.0f),
        glm::vec3(-0.4f, 1.5f, 0.4f),
        glm::vec3( 0.4f, 1.5f, -0.4f),
        glm::vec3( 1.0f, 0.2f, 0.0f)
    };
    m_bezierCurveSegments = 32;
    m_bezierSurfWidth = 1.8f;
    m_bezierSurfDepth = 1.8f;
    m_bezierSurfCurvature = 0.55f;
    m_bezierSurfUSeg = 16;
    m_bezierSurfVSeg = 16;
    m_revHeight = 1.1f;
    m_revBaseR = 0.22f;
    m_revBellyR = 0.48f;
    m_revNeckR = 0.16f;
    m_revRimR = 0.28f;
    m_revRadialSeg = 32;
    m_revProfileSeg = 20;
    m_revCapBottom = true;
    m_revCapTop = false;

    m_deskParams = geometry::DeskParams{};
    m_chairParams = geometry::ChairParams{};
    m_bookshelfParams = geometry::BookshelfParams{};
    m_lampParams = geometry::DeskLampParams{};
    m_vaseParams = geometry::VaseParams{};
    m_curvedChairParams = geometry::CurvedChairParams{};
    m_rootTransform.reset();
}

bool Application::uploadPlaygroundParts(std::vector<geometry::FurniturePart>& parts,
                                        const math::BoundingBox& overallBox,
                                        const geometry::MeshData* controlOverlayLines,
                                        const std::vector<glm::vec3>& controlPoints,
                                        std::string& outError)
{
    // Chuẩn bị toàn bộ GPU mesh tạm trước khi hoán đổi để nếu có lỗi sẽ KHÔNG làm mất mesh đang hiển thị
    std::vector<UploadedRenderPart> nextParts;
    nextParts.reserve(parts.size());

    std::uint32_t totalVerts = 0;
    std::uint32_t totalIndices = 0;
    std::uint32_t totalTris = 0;

    geometry::MeshData combinedNormals;
    combinedNormals.topology = graphics::PrimitiveTopology::Lines;

    for (auto& part : parts) {
        UploadedRenderPart uploaded;
        uploaded.name = part.name;
        uploaded.localTransform = part.localTransform;
        uploaded.color = part.baseColor;
        uploaded.isLinePrimitive = (part.meshData.topology == graphics::PrimitiveTopology::Lines);

        if (!uploaded.mesh.create(part.meshData.vertices, part.meshData.indices, part.meshData.topology, &outError)) {
            return false;
        }

        totalVerts += part.meshData.getVertexCount();
        totalIndices += part.meshData.getIndexCount();
        totalTris += part.meshData.getTriangleCount();

        // Thu thập các đoạn pháp tuyến đỉnh cho chế độ Normal Visualization (biến đổi về không gian root)
        if (part.meshData.topology == graphics::PrimitiveTopology::Triangles) {
            const glm::mat4 localMat = part.localTransform.getModelMatrix();
            const glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(localMat)));
            const glm::vec3 up(0.0f, 1.0f, 0.0f);
            for (const auto& v : part.meshData.vertices) {
                const glm::vec3 p = glm::vec3(localMat * glm::vec4(v.position, 1.0f));
                const glm::vec3 n = glm::normalize(normalMat * v.normal);
                const auto baseIdx = static_cast<std::uint32_t>(combinedNormals.vertices.size());
                combinedNormals.vertices.push_back({p, up, {0.0f, 0.0f}});
                combinedNormals.vertices.push_back({p + n * m_normalVisualLength, up, {1.0f, 0.0f}});
                combinedNormals.indices.push_back(baseIdx);
                combinedNormals.indices.push_back(baseIdx + 1);
            }
        }

        nextParts.push_back(std::move(uploaded));
    }

    graphics::Mesh nextBBoxLines;
    if (overallBox.isValid()) {
        const auto bboxData = geometry::GeometryUtils::buildBoundingBoxLines(overallBox);
        if (!bboxData.vertices.empty()) {
            nextBBoxLines.create(bboxData.vertices, bboxData.indices, bboxData.topology);
        }
    }

    graphics::Mesh nextControlLines;
    if (controlOverlayLines != nullptr && !controlOverlayLines->vertices.empty()) {
        nextControlLines.create(controlOverlayLines->vertices,
                                controlOverlayLines->indices,
                                controlOverlayLines->topology);
    }

    graphics::Mesh nextNormalLines;
    if (!combinedNormals.vertices.empty()) {
        nextNormalLines.create(combinedNormals.vertices, combinedNormals.indices, combinedNormals.topology);
    }

    // Hoán đổi tài nguyên GPU an toàn (RAII sẽ tự giải phóng các buffer cũ ngay khi vector cũ hết scope)
    m_activeParts = std::move(nextParts);
    m_activeBBoxLines = std::move(nextBBoxLines);
    m_activeControlLines = std::move(nextControlLines);
    m_activeNormalLines = std::move(nextNormalLines);
    m_activeControlPoints = controlPoints;
    m_activeBoundingBox = overallBox;
    m_activeVertexCount = totalVerts;
    m_activeIndexCount = totalIndices;
    m_activeTriangleCount = totalTris;
    return true;
}

bool Application::regenerateActivePlaygroundGeometry()
{
    if (m_category == PlaygroundCategory::M1ReferenceScene) {
        m_lastGeometryError.clear();
        return true;
    }

    const auto startTime = std::chrono::steady_clock::now();

    std::vector<geometry::FurniturePart> parts;
    math::BoundingBox overallBox;
    geometry::MeshData controlLines;
    bool hasControlLines = false;
    std::vector<glm::vec3> controlPoints;
    std::string error;
    bool ok = false;

    if (m_category == PlaygroundCategory::Primitives) {
        geometry::MeshData meshData;
        const glm::vec4 primColor{0.45f, 0.64f, 0.88f, 1.0f};
        switch (m_primitiveTypeIndex) {
            case 0: ok = geometry::PrimitiveGenerator::createPlane(m_planeWidth, m_planeDepth, m_planeSubX, m_planeSubZ, meshData, &error); break;
            case 1: ok = geometry::PrimitiveGenerator::createCube(m_cubeSize, meshData, &error); break;
            case 2: ok = geometry::PrimitiveGenerator::createCuboid(m_cuboidW, m_cuboidH, m_cuboidD, meshData, &error); break;
            case 3: ok = geometry::PrimitiveGenerator::createCylinder(m_cylRadius, m_cylHeight, m_cylSegments, m_cylCapped, meshData, &error); break;
            case 4: ok = geometry::PrimitiveGenerator::createSphere(m_sphereRadius, m_sphereLatSeg, m_sphereLonSeg, meshData, &error); break;
            case 5: ok = geometry::PrimitiveGenerator::createCone(m_coneRadius, m_coneHeight, m_coneSegments, m_coneCapped, meshData, &error); break;
            case 6: ok = geometry::PrimitiveGenerator::createFrustum(m_frustumBottomR, m_frustumTopR, m_frustumHeight, m_frustumSegments, m_frustumCapBottom, m_frustumCapTop, meshData, &error); break;
            default: error = "Unknown primitive type."; break;
        }
        if (ok) {
            overallBox = meshData.boundingBox;
            parts.push_back({"Primitive", std::move(meshData), scene::Transform(), primColor});
        }
    } else if (m_category == PlaygroundCategory::Parametric) {
        if (m_parametricTypeIndex == 0) {
            const geometry::BezierCurve curve(m_bezierCurvePts);
            geometry::MeshData curveLines;
            ok = curve.generateLineMesh(m_bezierCurveSegments, curveLines, &error) &&
                 curve.generateControlPolygonMesh(controlLines, &error);
            if (ok) {
                hasControlLines = true;
                controlPoints.assign(m_bezierCurvePts.begin(), m_bezierCurvePts.end());
                overallBox = curveLines.boundingBox;
                parts.push_back({"BezierCurve", std::move(curveLines), scene::Transform(), {0.35f, 0.88f, 0.60f, 1.0f}});
            }
        } else if (m_parametricTypeIndex == 1) {
            const auto surface = geometry::BezierSurface::createCurvedWavePatch(
                m_bezierSurfWidth, m_bezierSurfDepth, m_bezierSurfCurvature);
            geometry::MeshData surfMesh;
            ok = surface.generateMesh(m_bezierSurfUSeg, m_bezierSurfVSeg, surfMesh, &error) &&
                 surface.generateControlNetMesh(controlLines, &error);
            if (ok) {
                hasControlLines = true;
                const auto& grid = surface.getControlPoints();
                for (int i = 0; i < 4; ++i) {
                    for (int j = 0; j < 4; ++j) {
                        controlPoints.push_back(grid[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)]);
                    }
                }
                overallBox = surfMesh.boundingBox;
                parts.push_back({"BezierSurface", std::move(surfMesh), scene::Transform(), {0.48f, 0.70f, 0.90f, 1.0f}});
            }
        } else {
            const geometry::BezierCurve profile(
                glm::vec3(m_revBaseR,  0.0f,               0.0f),
                glm::vec3(m_revBellyR, m_revHeight * 0.35f, 0.0f),
                glm::vec3(m_revNeckR,  m_revHeight * 0.78f, 0.0f),
                glm::vec3(m_revRimR,   m_revHeight,         0.0f)
            );
            geometry::MeshData revMesh;
            ok = geometry::RevolutionSurface::generateFromBezierProfile(
                profile, m_revProfileSeg, m_revRadialSeg, m_revCapBottom, m_revCapTop, revMesh, &error);
            if (ok) {
                overallBox = revMesh.boundingBox;
                parts.push_back({"RevolutionSurface", std::move(revMesh), scene::Transform(), {0.42f, 0.72f, 0.78f, 1.0f}});
            }
        }
    } else if (m_category == PlaygroundCategory::Furniture) {
        geometry::FurnitureModel model;
        switch (m_furnitureTypeIndex) {
            case 0: ok = geometry::FurnitureGenerator::createDesk(m_deskParams, model, &error); break;
            case 1: ok = geometry::FurnitureGenerator::createChair(m_chairParams, model, &error); break;
            case 2: ok = geometry::FurnitureGenerator::createBookshelf(m_bookshelfParams, model, &error); break;
            case 3: ok = geometry::FurnitureGenerator::createDeskLamp(m_lampParams, model, &error); break;
            case 4: ok = geometry::FurnitureGenerator::createVase(m_vaseParams, model, &error); break;
            case 5: ok = geometry::FurnitureGenerator::createCurvedChair(m_curvedChairParams, model, &error); break;
            default: error = "Unknown furniture type."; break;
        }
        if (ok) {
            overallBox = model.boundingBox;
            parts = std::move(model.parts);
        }
    }

    if (!ok) {
        // Giữ nguyên mesh hợp lệ trước đó và hiển thị thông báo lỗi cho người dùng
        m_lastGeometryError = error;
        return false;
    }

    if (!uploadPlaygroundParts(parts, overallBox, hasControlLines ? &controlLines : nullptr, controlPoints, error)) {
        m_lastGeometryError = error;
        return false;
    }

    const auto endTime = std::chrono::steady_clock::now();
    m_lastGenerationTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    m_lastGeometryError.clear();
    return true;
}

void Application::shutdownGraphics()
{
    if (!m_initialized && !m_imguiInitialized &&
        !m_meshShader.isValid() && !m_gridShader.isValid() && !m_m0BasicShader.isValid() &&
        !m_cubeMesh.isValid() && !m_framebuffer.isComplete() && m_activeParts.empty() &&
        m_m0Vao == 0 && m_m0Vbo == 0) {
        return;
    }

    if (m_imguiInitialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGLUT_Shutdown();
        ImGui::DestroyContext();
        m_imguiInitialized = false;
    }

    // Giải phóng toàn bộ GPU mesh của Geometry Playground trước khi hủy OpenGL context
    m_activeParts.clear();
    m_activeBBoxLines.destroy();
    m_activeControlLines.destroy();
    m_activeNormalLines.destroy();

    m_framebuffer.destroy();
    m_cubeMesh.destroy();
    m_controlMarkerMesh.destroy();
    m_gridMesh.destroy();
    m_axisXMesh.destroy();
    m_axisYMesh.destroy();
    m_axisZMesh.destroy();
    m_meshShader.destroy();
    m_gridShader.destroy();

    m_m0BasicShader.destroy();
    if (m_m0Vao != 0) {
        glDeleteVertexArrays(1, &m_m0Vao);
        m_m0Vao = 0;
    }
    if (m_m0Vbo != 0) {
        glDeleteBuffers(1, &m_m0Vbo);
        m_m0Vbo = 0;
    }

    m_initialized = false;
    std::cout << "[RoomCraft3D] GPU & Dear ImGui resources released cleanly." << std::endl;
}

std::filesystem::path Application::resolveAssetPath(const std::filesystem::path& relativePath) const
{
    const auto candidateExe = m_executableDir / relativePath;
    if (std::filesystem::exists(candidateExe)) {
        return candidateExe;
    }
    const auto candidateCwd = std::filesystem::current_path() / relativePath;
    if (std::filesystem::exists(candidateCwd)) {
        return candidateCwd;
    }
    return candidateExe;
}

void Application::renderFrame()
{
    if (!m_initialized) {
        return;
    }

    ++m_frameCount;
    ++m_framesSinceLastFps;
    const int nowMs = m_window.getElapsedTimeMs();
    m_frameTimeMs = static_cast<float>(std::max(0, nowMs - m_lastFrameTimeMs));
    m_lastFrameTimeMs = nowMs;

    const int elapsedFpsMs = nowMs - m_lastFpsTimeMs;
    if (elapsedFpsMs >= 500) {
        m_currentFps = (static_cast<float>(m_framesSinceLastFps) * 1000.0f) / static_cast<float>(elapsedFpsMs);
        m_framesSinceLastFps = 0;
        m_lastFpsTimeMs = nowMs;
    }

    if (m_config.mode == ApplicationMode::SmokeTestM0) {
        renderM0RegressionFrame();
        return;
    }

    if (m_config.mode == ApplicationMode::SmokeTestM1 && !m_m1SuiteExecuted) {
        m_m1SuiteExecuted = true;
        m_m1SuitePassed = runM1IntegrationSuiteOnce();
        m_framebuffer.resize(800, 600);
        m_camera.reset();
        m_camera.setAspectRatio(static_cast<float>(m_framebuffer.getWidth()) /
                                static_cast<float>(m_framebuffer.getHeight()));
    }

    if (m_config.mode == ApplicationMode::SmokeTestM2 && !m_m2SuiteExecuted) {
        m_m2SuiteExecuted = true;
        m_m2SuitePassed = runM2IntegrationSuiteOnce();
        m_framebuffer.resize(800, 600);
        m_category = PlaygroundCategory::Furniture;
        m_furnitureTypeIndex = 0; // Desk
        regenerateActivePlaygroundGeometry();
        m_camera.setProjectionMode(graphics::ProjectionMode::Perspective);
        m_camera.lookAt({2.3f, 1.8f, 2.7f}, {0.0f, 0.55f, 0.0f}, {0.0f, 1.0f, 0.0f});
        m_camera.setAspectRatio(static_cast<float>(m_framebuffer.getWidth()) /
                                static_cast<float>(m_framebuffer.getHeight()));
    }

    renderInteractiveFrame();

    if (m_config.mode == ApplicationMode::SmokeTestM1) {
        if (m_frameCount == 3 && !m_m1MultiCubePixelsVerified) {
            m_m1MultiCubePixelsVerified = verifyM1MultiCubeRenderedPixels();
        }
        if (m_frameCount >= m_config.smokeTestFrames) {
            m_window.requestClose();
            return;
        }
    }

    if (m_config.mode == ApplicationMode::SmokeTestM2) {
        if (m_frameCount >= m_config.smokeTestFrames) {
            m_window.requestClose();
            return;
        }
    }

    m_window.postRedisplay();
}

void Application::renderM0RegressionFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();

    const auto& gpu = m_window.getGpuInfo();
    ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(390.0f, 250.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("RoomCraft 3D - M0 Foundation Diagnostics")) {
        ImGui::Text("Status: M0 Regression Mode");
        ImGui::Separator();
        ImGui::Text("GPU Renderer : %s", gpu.renderer.c_str());
        ImGui::Text("OpenGL       : %s (%s)", gpu.glVersion.c_str(), gpu.profile.c_str());
        ImGui::Text("GLSL         : %s", gpu.glslVersion.c_str());
        ImGui::Text("Viewport     : %d x %d", m_window.getWidth(), m_window.getHeight());
        ImGui::Checkbox("Render M0 Triangle", &m_showM0Triangle);
    }
    ImGui::End();
    ImGui::Render();

    glViewport(0, 0, m_window.getWidth(), m_window.getHeight());
    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_showM0Triangle && m_m0BasicShader.isValid()) {
        m_m0BasicShader.bind();
        glBindVertexArray(m_m0Vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        graphics::Shader::unbind();
    }

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (!m_m0PixelVerified && m_frameCount == 2) {
        glReadPixels(m_window.getWidth() / 2, m_window.getHeight() / 2, 1, 1,
                     GL_RGBA, GL_UNSIGNED_BYTE, m_m0SampledPixel.data());
        const int bgR = static_cast<int>(m_clearColor.r * 255.0f);
        const int bgG = static_cast<int>(m_clearColor.g * 255.0f);
        const int bgB = static_cast<int>(m_clearColor.b * 255.0f);
        const int diff = std::abs(static_cast<int>(m_m0SampledPixel[0]) - bgR) +
                         std::abs(static_cast<int>(m_m0SampledPixel[1]) - bgG) +
                         std::abs(static_cast<int>(m_m0SampledPixel[2]) - bgB);
        m_m0PixelVerified = (diff > 30);
    }

    drainAndCheckOpenGlErrors();
    m_window.swapBuffers();

    if (!m_m0ResizeTriggered && m_frameCount >= 3) {
        m_m0ResizeTriggered = true;
        m_window.resize(960, 540);
    }
    if ((m_frameCount >= m_config.smokeTestFrames && m_m0ResizeObserved) ||
        m_frameCount >= m_config.smokeTestFrames + 20) {
        m_window.requestClose();
        return;
    }

    m_window.postRedisplay();
}

void Application::renderM1DemoSceneToFramebuffer(float faceContrast)
{
    if (!m_framebuffer.isComplete()) {
        return;
    }

    m_framebuffer.bind();
    m_renderer.beginFrame(m_framebuffer.getWidth(),
                          m_framebuffer.getHeight(),
                          m_clearColor,
                          m_depthTestEnabled);

    const glm::mat4 identity(1.0f);
    if (m_showGrid) {
        m_renderer.drawLines(m_gridMesh, m_gridShader, identity, m_camera, {0.30f, 0.33f, 0.40f, 1.0f});
    }
    if (m_showAxes) {
        m_renderer.drawLines(m_axisXMesh, m_gridShader, identity, m_camera, {0.92f, 0.30f, 0.30f, 1.0f});
        m_renderer.drawLines(m_axisYMesh, m_gridShader, identity, m_camera, {0.30f, 0.88f, 0.42f, 1.0f});
        m_renderer.drawLines(m_axisZMesh, m_gridShader, identity, m_camera, {0.34f, 0.56f, 0.96f, 1.0f});
    }

    m_renderer.setWireframeMode(m_wireframeMode);
    for (const auto& cube : m_demoCubes) {
        m_renderer.drawMesh(m_cubeMesh, m_meshShader, cube.transform, m_camera, cube.color, faceContrast);
    }

    m_renderer.endFrame();
    graphics::Framebuffer::unbind();
}

void Application::renderPlaygroundSceneToFramebuffer(float faceContrast)
{
    if (!m_framebuffer.isComplete()) {
        return;
    }

    m_framebuffer.bind();
    m_renderer.beginFrame(m_framebuffer.getWidth(),
                          m_framebuffer.getHeight(),
                          m_clearColor,
                          m_depthTestEnabled);

    const glm::mat4 identity(1.0f);
    if (m_showGrid) {
        m_renderer.drawLines(m_gridMesh, m_gridShader, identity, m_camera, {0.28f, 0.31f, 0.38f, 1.0f});
    }
    if (m_showAxes) {
        m_renderer.drawLines(m_axisXMesh, m_gridShader, identity, m_camera, {0.92f, 0.30f, 0.30f, 1.0f});
        m_renderer.drawLines(m_axisYMesh, m_gridShader, identity, m_camera, {0.30f, 0.88f, 0.42f, 1.0f});
        m_renderer.drawLines(m_axisZMesh, m_gridShader, identity, m_camera, {0.34f, 0.56f, 0.96f, 1.0f});
    }

    // Áp dụng biến đổi phân cấp: M_world_part = M_parent * M_local_part
    const glm::mat4 parentMat = m_rootTransform.getModelMatrix();

    m_renderer.setWireframeMode(m_wireframeMode);
    for (const auto& part : m_activeParts) {
        const glm::mat4 worldMat = parentMat * part.localTransform.getModelMatrix();
        if (part.isLinePrimitive) {
            m_renderer.drawLines(part.mesh, m_gridShader, worldMat, m_camera, part.color);
        } else {
            m_renderer.drawMeshWithMatrix(part.mesh, m_meshShader, worldMat, m_camera, part.color, faceContrast);
        }
    }

    // Khôi phục GL_FILL trước khi vẽ các overlay trực quan hóa (BoundingBox, Control Points, Normals)
    m_renderer.setWireframeMode(false);

    if (m_showBoundingBox && m_activeBBoxLines.isValid()) {
        m_renderer.drawLines(m_activeBBoxLines, m_gridShader, parentMat, m_camera, {0.96f, 0.78f, 0.28f, 1.0f});
    }

    if (m_showControlOverlay) {
        if (m_activeControlLines.isValid()) {
            m_renderer.drawLines(m_activeControlLines, m_gridShader, parentMat, m_camera, {0.95f, 0.65f, 0.25f, 1.0f});
        }
        for (const auto& cp : m_activeControlPoints) {
            const glm::mat4 cpMat = parentMat * glm::translate(glm::mat4(1.0f), cp - glm::vec3(0.0f, 0.0275f, 0.0f));
            m_renderer.drawMeshWithMatrix(m_controlMarkerMesh, m_meshShader, cpMat, m_camera, {0.96f, 0.35f, 0.35f, 1.0f}, 0.25f);
        }
    }

    if (m_showVertexNormals && m_activeNormalLines.isValid()) {
        m_renderer.drawLines(m_activeNormalLines, m_gridShader, parentMat, m_camera, {0.35f, 0.92f, 0.88f, 1.0f});
    }

    m_renderer.endFrame();
    graphics::Framebuffer::unbind();
}

void Application::renderInteractiveFrame()
{
    if (m_category == PlaygroundCategory::M1ReferenceScene) {
        renderM1DemoSceneToFramebuffer(0.45f);
    } else {
        renderPlaygroundSceneToFramebuffer(0.45f);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();

    renderPlaygroundControlUi();
    renderViewportUi();

    ImGui::Render();

    glViewport(0, 0, m_window.getWidth(), m_window.getHeight());
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.070f, 0.075f, 0.095f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    drainAndCheckOpenGlErrors();
    m_window.swapBuffers();
}

void Application::renderPlaygroundControlUi()
{
    const auto& gpu = m_window.getGpuInfo();
    const auto& stats = m_renderer.getLastFrameStats();

    ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(390.0f, 696.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("RoomCraft 3D - M2 Geometry Playground")) {
        bool paramsChanged = false;
        bool selectionChanged = false;

        // 1. Chọn danh mục hình học
        const char* categories[] = {"Primitives", "Parametric", "Procedural Furniture", "M1 Reference Scene"};
        int catIdx = static_cast<int>(m_category);
        if (ImGui::Combo("Category", &catIdx, categories, 4)) {
            m_category = static_cast<PlaygroundCategory>(catIdx);
            selectionChanged = true;
        }

        // 2. Chọn loại hình học cụ thể trong danh mục
        if (m_category == PlaygroundCategory::Primitives) {
            const char* primTypes[] = {"Plane", "Cube", "Cuboid", "Cylinder", "Sphere", "Cone", "Frustum"};
            if (ImGui::Combo("Primitive Type", &m_primitiveTypeIndex, primTypes, 7)) {
                selectionChanged = true;
            }
        } else if (m_category == PlaygroundCategory::Parametric) {
            const char* paramTypes[] = {"Bezier Curve (Cubic)", "Bezier Surface (4x4)", "Surface of Revolution"};
            if (ImGui::Combo("Parametric Type", &m_parametricTypeIndex, paramTypes, 3)) {
                selectionChanged = true;
            }
        } else if (m_category == PlaygroundCategory::Furniture) {
            const char* furnTypes[] = {
                "01. Procedural Desk",
                "02. Procedural Chair",
                "03. Procedural Bookshelf",
                "04. Procedural Desk Lamp",
                "05. Procedural Vase (Revolution)",
                "06. Procedural Curved Chair (Bezier)"
            };
            if (ImGui::Combo("Furniture Type", &m_furnitureTypeIndex, furnTypes, 6)) {
                selectionChanged = true;
            }
        }

        ImGui::Separator();

        // 3. Trình chỉnh sửa tham số tương ứng
        if (m_category == PlaygroundCategory::Primitives) {
            ImGui::TextUnformatted("Primitive Parameters (meters)");
            switch (m_primitiveTypeIndex) {
                case 0:
                    paramsChanged |= ImGui::SliderFloat("Width", &m_planeWidth, 0.2f, 6.0f);
                    paramsChanged |= ImGui::SliderFloat("Depth", &m_planeDepth, 0.2f, 6.0f);
                    paramsChanged |= ImGui::SliderInt("Subdivisions X", &m_planeSubX, 1, 32);
                    paramsChanged |= ImGui::SliderInt("Subdivisions Z", &m_planeSubZ, 1, 32);
                    break;
                case 1:
                    paramsChanged |= ImGui::SliderFloat("Size", &m_cubeSize, 0.1f, 3.0f);
                    break;
                case 2:
                    paramsChanged |= ImGui::SliderFloat("Width", &m_cuboidW, 0.1f, 4.0f);
                    paramsChanged |= ImGui::SliderFloat("Height", &m_cuboidH, 0.1f, 4.0f);
                    paramsChanged |= ImGui::SliderFloat("Depth", &m_cuboidD, 0.1f, 4.0f);
                    break;
                case 3:
                    paramsChanged |= ImGui::SliderFloat("Radius", &m_cylRadius, 0.05f, 2.0f);
                    paramsChanged |= ImGui::SliderFloat("Height", &m_cylHeight, 0.1f, 3.5f);
                    paramsChanged |= ImGui::SliderInt("Radial Segments", &m_cylSegments, 3, 96);
                    paramsChanged |= ImGui::Checkbox("Capped", &m_cylCapped);
                    break;
                case 4:
                    paramsChanged |= ImGui::SliderFloat("Radius", &m_sphereRadius, 0.1f, 2.0f);
                    paramsChanged |= ImGui::SliderInt("Latitude Segments", &m_sphereLatSeg, 3, 64);
                    paramsChanged |= ImGui::SliderInt("Longitude Segments", &m_sphereLonSeg, 3, 96);
                    break;
                case 5:
                    paramsChanged |= ImGui::SliderFloat("Base Radius", &m_coneRadius, 0.05f, 2.0f);
                    paramsChanged |= ImGui::SliderFloat("Height", &m_coneHeight, 0.1f, 3.5f);
                    paramsChanged |= ImGui::SliderInt("Radial Segments", &m_coneSegments, 3, 96);
                    paramsChanged |= ImGui::Checkbox("Cap Bottom", &m_coneCapped);
                    break;
                case 6:
                    paramsChanged |= ImGui::SliderFloat("Bottom Radius", &m_frustumBottomR, 0.0f, 2.0f);
                    paramsChanged |= ImGui::SliderFloat("Top Radius", &m_frustumTopR, 0.0f, 2.0f);
                    paramsChanged |= ImGui::SliderFloat("Height", &m_frustumHeight, 0.1f, 3.5f);
                    paramsChanged |= ImGui::SliderInt("Radial Segments", &m_frustumSegments, 3, 96);
                    paramsChanged |= ImGui::Checkbox("Cap Bottom", &m_frustumCapBottom);
                    ImGui::SameLine();
                    paramsChanged |= ImGui::Checkbox("Cap Top", &m_frustumCapTop);
                    break;
                default: break;
            }
        } else if (m_category == PlaygroundCategory::Parametric) {
            ImGui::TextUnformatted("Parametric Parameters");
            if (m_parametricTypeIndex == 0) {
                paramsChanged |= ImGui::DragFloat3("P0", &m_bezierCurvePts[0].x, 0.03f, -3.0f, 3.0f);
                paramsChanged |= ImGui::DragFloat3("P1", &m_bezierCurvePts[1].x, 0.03f, -3.0f, 3.0f);
                paramsChanged |= ImGui::DragFloat3("P2", &m_bezierCurvePts[2].x, 0.03f, -3.0f, 3.0f);
                paramsChanged |= ImGui::DragFloat3("P3", &m_bezierCurvePts[3].x, 0.03f, -3.0f, 3.0f);
                paramsChanged |= ImGui::SliderInt("Curve Segments", &m_bezierCurveSegments, 2, 128);
            } else if (m_parametricTypeIndex == 1) {
                paramsChanged |= ImGui::SliderFloat("Patch Width", &m_bezierSurfWidth, 0.4f, 4.0f);
                paramsChanged |= ImGui::SliderFloat("Patch Depth", &m_bezierSurfDepth, 0.4f, 4.0f);
                paramsChanged |= ImGui::SliderFloat("Curvature Height", &m_bezierSurfCurvature, -1.0f, 1.5f);
                paramsChanged |= ImGui::SliderInt("U Segments", &m_bezierSurfUSeg, 2, 48);
                paramsChanged |= ImGui::SliderInt("V Segments", &m_bezierSurfVSeg, 2, 48);
            } else {
                paramsChanged |= ImGui::SliderFloat("Height", &m_revHeight, 0.2f, 2.5f);
                paramsChanged |= ImGui::SliderFloat("Base Radius", &m_revBaseR, 0.02f, 0.8f);
                paramsChanged |= ImGui::SliderFloat("Belly Radius", &m_revBellyR, 0.05f, 1.0f);
                paramsChanged |= ImGui::SliderFloat("Neck Radius", &m_revNeckR, 0.02f, 0.8f);
                paramsChanged |= ImGui::SliderFloat("Rim Radius", &m_revRimR, 0.02f, 0.9f);
                paramsChanged |= ImGui::SliderInt("Radial Segments", &m_revRadialSeg, 6, 96);
                paramsChanged |= ImGui::SliderInt("Profile Segments", &m_revProfileSeg, 4, 64);
                paramsChanged |= ImGui::Checkbox("Cap Bottom", &m_revCapBottom);
                ImGui::SameLine();
                paramsChanged |= ImGui::Checkbox("Cap Top", &m_revCapTop);
            }
        } else if (m_category == PlaygroundCategory::Furniture) {
            ImGui::TextUnformatted("Procedural Furniture Parameters (meters)");
            switch (m_furnitureTypeIndex) {
                case 0:
                    paramsChanged |= ImGui::SliderFloat("Width", &m_deskParams.width, 0.7f, 2.4f);
                    paramsChanged |= ImGui::SliderFloat("Depth", &m_deskParams.depth, 0.4f, 1.2f);
                    paramsChanged |= ImGui::SliderFloat("Height", &m_deskParams.height, 0.55f, 1.1f);
                    paramsChanged |= ImGui::SliderFloat("Top Thickness", &m_deskParams.topThickness, 0.02f, 0.12f);
                    paramsChanged |= ImGui::SliderFloat("Leg Thickness", &m_deskParams.legThickness, 0.025f, 0.12f);
                    break;
                case 1:
                    paramsChanged |= ImGui::SliderFloat("Width", &m_chairParams.width, 0.35f, 0.75f);
                    paramsChanged |= ImGui::SliderFloat("Depth", &m_chairParams.depth, 0.35f, 0.75f);
                    paramsChanged |= ImGui::SliderFloat("Seat Height", &m_chairParams.seatHeight, 0.30f, 0.65f);
                    paramsChanged |= ImGui::SliderFloat("Backrest Height", &m_chairParams.backrestHeight, 0.25f, 0.75f);
                    paramsChanged |= ImGui::SliderFloat("Leg Radius", &m_chairParams.legRadius, 0.012f, 0.045f);
                    paramsChanged |= ImGui::SliderInt("Leg Segments", &m_chairParams.radialSegments, 6, 48);
                    break;
                case 2:
                    paramsChanged |= ImGui::SliderFloat("Width", &m_bookshelfParams.width, 0.5f, 2.0f);
                    paramsChanged |= ImGui::SliderFloat("Height", &m_bookshelfParams.height, 0.8f, 2.6f);
                    paramsChanged |= ImGui::SliderFloat("Depth", &m_bookshelfParams.depth, 0.20f, 0.60f);
                    paramsChanged |= ImGui::SliderInt("Shelf Count", &m_bookshelfParams.shelfCount, 2, 10);
                    paramsChanged |= ImGui::SliderFloat("Panel Thickness", &m_bookshelfParams.panelThickness, 0.015f, 0.05f);
                    break;
                case 3:
                    paramsChanged |= ImGui::SliderFloat("Base Radius", &m_lampParams.baseRadius, 0.06f, 0.25f);
                    paramsChanged |= ImGui::SliderFloat("Stem Height", &m_lampParams.stemHeight, 0.18f, 0.80f);
                    paramsChanged |= ImGui::SliderFloat("Shade Bottom R", &m_lampParams.shadeBottomRadius, 0.07f, 0.30f);
                    paramsChanged |= ImGui::SliderFloat("Shade Top R", &m_lampParams.shadeTopRadius, 0.02f, 0.20f);
                    paramsChanged |= ImGui::SliderFloat("Shade Height", &m_lampParams.shadeHeight, 0.08f, 0.35f);
                    paramsChanged |= ImGui::SliderInt("Radial Segments", &m_lampParams.radialSegments, 8, 64);
                    break;
                case 4:
                    paramsChanged |= ImGui::SliderFloat("Height", &m_vaseParams.height, 0.18f, 1.0f);
                    paramsChanged |= ImGui::SliderFloat("Base Radius", &m_vaseParams.baseRadius, 0.03f, 0.20f);
                    paramsChanged |= ImGui::SliderFloat("Belly Radius", &m_vaseParams.bellyRadius, 0.05f, 0.30f);
                    paramsChanged |= ImGui::SliderFloat("Neck Radius", &m_vaseParams.neckRadius, 0.02f, 0.20f);
                    paramsChanged |= ImGui::SliderFloat("Rim Radius", &m_vaseParams.rimRadius, 0.03f, 0.25f);
                    paramsChanged |= ImGui::SliderInt("Radial Segments", &m_vaseParams.radialSegments, 8, 96);
                    paramsChanged |= ImGui::SliderInt("Profile Segments", &m_vaseParams.profileSegments, 6, 48);
                    break;
                case 5:
                    paramsChanged |= ImGui::SliderFloat("Width", &m_curvedChairParams.width, 0.38f, 0.80f);
                    paramsChanged |= ImGui::SliderFloat("Depth", &m_curvedChairParams.depth, 0.38f, 0.75f);
                    paramsChanged |= ImGui::SliderFloat("Seat Height", &m_curvedChairParams.seatHeight, 0.32f, 0.60f);
                    paramsChanged |= ImGui::SliderFloat("Backrest Height", &m_curvedChairParams.backrestHeight, 0.30f, 0.70f);
                    paramsChanged |= ImGui::SliderFloat("Curvature", &m_curvedChairParams.backrestCurvature, 0.02f, 0.25f);
                    paramsChanged |= ImGui::SliderInt("U Segments", &m_curvedChairParams.uSegments, 4, 32);
                    paramsChanged |= ImGui::SliderInt("V Segments", &m_curvedChairParams.vSegments, 4, 32);
                    break;
                default: break;
            }
        } else {
            ImGui::TextUnformatted("M1 Demo Cube Transform");
            if (!m_demoCubes.empty()) {
                m_selectedCubeIndex = std::clamp(m_selectedCubeIndex, 0, static_cast<int>(m_demoCubes.size()) - 1);
                if (ImGui::BeginCombo("Cube", m_demoCubes[static_cast<std::size_t>(m_selectedCubeIndex)].name.c_str())) {
                    for (int i = 0; i < static_cast<int>(m_demoCubes.size()); ++i) {
                        if (ImGui::Selectable(m_demoCubes[static_cast<std::size_t>(i)].name.c_str(), i == m_selectedCubeIndex)) {
                            m_selectedCubeIndex = i;
                        }
                    }
                    ImGui::EndCombo();
                }
                auto& obj = m_demoCubes[static_cast<std::size_t>(m_selectedCubeIndex)];
                ImGui::DragFloat3("Pos (m)", &obj.transform.position.x, 0.05f, -10.0f, 10.0f);
                ImGui::DragFloat3("Rot (deg)", &obj.transform.rotationDegrees.x, 1.0f, -360.0f, 360.0f);
                glm::vec3 s = obj.transform.scale;
                if (ImGui::DragFloat3("Scale", &s.x, 0.05f, 0.1f, 5.0f)) {
                    obj.transform.setScaleSafe(s, 0.05f);
                }
                if (ImGui::Button("Reset Cube")) {
                    obj.transform = obj.defaultTransform;
                }
            }
        }

        if (m_category != PlaygroundCategory::M1ReferenceScene) {
            ImGui::Checkbox("Auto-regenerate", &m_autoRegenerate);
            ImGui::SameLine();
            const bool manualRegen = ImGui::Button("Regenerate Mesh");
            ImGui::SameLine();
            if (ImGui::Button("Reset Params")) {
                resetCurrentPlaygroundParameters();
                paramsChanged = true;
            }

            if (selectionChanged || manualRegen || (paramsChanged && m_autoRegenerate)) {
                regenerateActivePlaygroundGeometry();
            }

            if (!m_lastGeometryError.empty()) {
                ImGui::TextColored(ImVec4(0.96f, 0.36f, 0.36f, 1.0f), "Error: %s", m_lastGeometryError.c_str());
            }
        }

        ImGui::Separator();

        // 4. Tùy chọn trực quan hóa
        ImGui::TextUnformatted("Visualization & Camera");
        int polyMode = m_wireframeMode ? 1 : 0;
        if (ImGui::RadioButton("Solid", &polyMode, 0)) {
            m_wireframeMode = false;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Wireframe", &polyMode, 1)) {
            m_wireframeMode = true;
        }
        ImGui::SameLine();
        ImGui::Checkbox("Depth Test", &m_depthTestEnabled);

        ImGui::Checkbox("Grid (XZ)", &m_showGrid);
        ImGui::SameLine();
        ImGui::Checkbox("XYZ Axes", &m_showAxes);
        ImGui::SameLine();
        ImGui::Checkbox("BoundingBox", &m_showBoundingBox);

        ImGui::Checkbox("Control Net/Pts", &m_showControlOverlay);
        ImGui::SameLine();
        ImGui::Checkbox("Vertex Normals", &m_showVertexNormals);

        int projIdx = (m_camera.getProjectionMode() == graphics::ProjectionMode::Perspective) ? 0 : 1;
        if (ImGui::RadioButton("Perspective", &projIdx, 0)) {
            m_camera.setProjectionMode(graphics::ProjectionMode::Perspective);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Orthographic", &projIdx, 1)) {
            m_camera.setProjectionMode(graphics::ProjectionMode::Orthographic);
        }

        if (ImGui::Button("Close 3D")) {
            m_camera.setProjectionMode(graphics::ProjectionMode::Perspective);
            m_camera.lookAt({2.3f, 1.8f, 2.7f}, {0.0f, 0.55f, 0.0f}, {0.0f, 1.0f, 0.0f});
        }
        ImGui::SameLine();
        if (ImGui::Button("Wide 3D")) {
            m_camera.setPerspectivePreset();
        }
        ImGui::SameLine();
        if (ImGui::Button("Top")) {
            m_camera.setTopOrthoPreset();
        }
        ImGui::SameLine();
        if (ImGui::Button("Front")) {
            m_camera.setFrontOrthoPreset();
        }

        if (m_category != PlaygroundCategory::M1ReferenceScene) {
            if (ImGui::TreeNode("Root Transform (Hierarchy Test)")) {
                ImGui::DragFloat3("Root Pos", &m_rootTransform.position.x, 0.05f, -5.0f, 5.0f);
                ImGui::DragFloat3("Root Rot", &m_rootTransform.rotationDegrees.x, 1.0f, -360.0f, 360.0f);
                glm::vec3 rs = m_rootTransform.scale;
                if (ImGui::DragFloat3("Root Scale", &rs.x, 0.05f, 0.1f, 3.0f)) {
                    m_rootTransform.setScaleSafe(rs, 0.05f);
                }
                if (ImGui::Button("Reset Root")) {
                    m_rootTransform.reset();
                }
                ImGui::TreePop();
            }
        }

        ImGui::Separator();

        // 5. Thống kê thật (không hardcode)
        ImGui::TextUnformatted("Real-Time Statistics");
        ImGui::Text("GPU: %s (GL %d.%d Core)", gpu.renderer.c_str(), gpu.majorVersion, gpu.minorVersion);
        ImGui::Text("FPS: %.1f | Frame: %.2f ms | Draw Calls: %u", m_currentFps, m_frameTimeMs, stats.drawCalls);
        ImGui::Text("Mesh Verts: %u | Indices: %u | Tris: %u",
                    m_activeVertexCount, m_activeIndexCount, m_activeTriangleCount);
        ImGui::Text("Gen Time: %.3f ms | FBO: %dx%d",
                    m_lastGenerationTimeMs, m_framebuffer.getWidth(), m_framebuffer.getHeight());
        if (m_activeBoundingBox.isValid()) {
            const glm::vec3 sz = m_activeBoundingBox.getSize();
            ImGui::Text("BBox Size: %.2f x %.2f x %.2f m", sz.x, sz.y, sz.z);
            ImGui::Text("BBox Min : (%.2f, %.2f, %.2f)",
                        m_activeBoundingBox.min.x, m_activeBoundingBox.min.y, m_activeBoundingBox.min.z);
        }
    }
    ImGui::End();
}

void Application::renderViewportUi()
{
    ImGui::SetNextWindowPos(ImVec2(414.0f, 12.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(854.0f, 696.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("3D Viewport (Off-screen Framebuffer)")) {
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const int targetW = static_cast<int>(avail.x);
        const int targetH = static_cast<int>(avail.y);

        if (targetW > 0 && targetH > 0 && m_config.mode == ApplicationMode::InteractivePlayground) {
            if (targetW != m_framebuffer.getWidth() || targetH != m_framebuffer.getHeight()) {
                m_framebuffer.resize(targetW, targetH);
            }
            m_camera.setAspectRatio(static_cast<float>(m_framebuffer.getWidth()) /
                                    static_cast<float>(m_framebuffer.getHeight()));
        }

        if (m_framebuffer.isComplete() && targetW > 0 && targetH > 0) {
            const auto texId = static_cast<ImTextureID>(static_cast<std::uintptr_t>(m_framebuffer.getColorTextureId()));
            ImGui::Image(texId, avail, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
        } else {
            ImGui::TextDisabled("Framebuffer inactive (viewport size is 0).");
        }
    }
    ImGui::End();
}

bool Application::runM1IntegrationSuiteOnce()
{
    bool allPassed = true;
    const auto report = [&allPassed](const char* id, const char* desc, bool ok) {
        std::cout << "[" << id << "] " << desc << ": " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok) {
            allPassed = false;
        }
    };

    report("TC-M1-02", "Valid 3D & Grid shaders compiled and linked",
           m_meshShader.isValid() && m_gridShader.isValid());

    {
        graphics::Shader badShader;
        std::string errMsg;
        const bool compiled = badShader.loadFromSource(
            "#version 330 core\nvoid main() { INVALID_SYNTAX_TOKEN }",
            "#version 330 core\nout vec4 FragColor; void main() { FragColor = vec4(1.0); }",
            "invalid_test_shader",
            &errMsg);
        report("TC-M1-03", "Invalid GLSL syntax rejected with diagnostic log",
               (!compiled) && (!badShader.isValid()) && (!errMsg.empty()));
    }

    report("TC-M1-04", "Indexed cube mesh uploaded (24 vertices, 36 indices)",
           m_cubeMesh.isValid() && m_cubeMesh.getVertexCount() == 24 && m_cubeMesh.getIndexCount() == 36);

    {
        graphics::Mesh badMesh;
        std::vector<graphics::Vertex> verts(3);
        std::vector<std::uint32_t> badIndices = {0, 1, 99};
        std::string errMsg;
        const bool created = badMesh.create(verts, badIndices, graphics::PrimitiveTopology::Triangles, &errMsg);
        report("TC-M1-05", "Out-of-bounds mesh indices rejected cleanly",
               (!created) && (!badMesh.isValid()) && (!errMsg.empty()));
    }

    report("TC-M1-13", "Off-screen Framebuffer complete with valid color texture",
           m_framebuffer.isComplete() && m_framebuffer.getColorTextureId() != 0);
    report("TC-M1-14", "Framebuffer resize, no-op caching, and 0x0 safety",
           verifyM1FramebufferResizeAndZeroSafety());
    report("TC-M1-11", "Depth testing order-independent proof + depth-off control",
           verifyM1DepthTestingOrderIndependence());
    report("TC-M1-15", "RAII move ownership & safe resource destruction",
           verifyM1RaiiMoveLifecycle());

    drainAndCheckOpenGlErrors();
    report("TC-M1-16", "Zero unexpected OpenGL errors during integration suite", !m_glErrorDetected);
    return allPassed;
}

bool Application::verifyM1FramebufferResizeAndZeroSafety()
{
    graphics::Framebuffer fbo;
    if (!fbo.create(640, 360) || !fbo.isComplete()) {
        return false;
    }
    const unsigned int initialFboId = fbo.getFboId();
    const unsigned int initialTexId = fbo.getColorTextureId();

    if (!fbo.resize(640, 360) || fbo.getFboId() != initialFboId || fbo.getColorTextureId() != initialTexId) {
        return false;
    }
    if (!fbo.resize(0, 0) || !fbo.isComplete() || fbo.getWidth() != 640 || fbo.getHeight() != 360) {
        return false;
    }
    if (!fbo.resize(800, 600) || !fbo.isComplete() || fbo.getWidth() != 800 || fbo.getHeight() != 600) {
        return false;
    }

    fbo.destroy();
    return !fbo.isComplete() && fbo.getFboId() == 0 && fbo.getColorTextureId() == 0;
}

bool Application::verifyM1DepthTestingOrderIndependence()
{
    graphics::Framebuffer fbo;
    if (!fbo.create(256, 256)) {
        return false;
    }

    graphics::Camera testCam;
    testCam.setProjectionMode(graphics::ProjectionMode::Perspective);
    testCam.setAspectRatio(1.0f);
    testCam.setFieldOfView(45.0f);
    testCam.setClipPlanes(0.1f, 50.0f);
    testCam.lookAt({0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

    const scene::Transform nearRed({0.0f, 0.0f, 1.5f}, {0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 0.4f});
    const scene::Transform farBlue({0.0f, 0.0f, -1.5f}, {0.0f, 0.0f, 0.0f}, {2.5f, 2.5f, 0.4f});
    const glm::vec4 redColor{1.0f, 0.0f, 0.0f, 1.0f};
    const glm::vec4 blueColor{0.0f, 0.0f, 1.0f, 1.0f};

    const auto readCenterPixel = [&]() -> std::array<unsigned char, 4> {
        std::array<unsigned char, 4> px{0, 0, 0, 0};
        glReadPixels(128, 128, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
        return px;
    };

    fbo.bind();
    m_renderer.beginFrame(256, 256, {0.0f, 0.0f, 0.0f, 1.0f}, true);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, farBlue, testCam, blueColor, 0.0f);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, nearRed, testCam, redColor, 0.0f);
    m_renderer.endFrame();
    const auto pxOrderFarFirst = readCenterPixel();

    m_renderer.beginFrame(256, 256, {0.0f, 0.0f, 0.0f, 1.0f}, true);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, nearRed, testCam, redColor, 0.0f);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, farBlue, testCam, blueColor, 0.0f);
    m_renderer.endFrame();
    const auto pxOrderNearFirst = readCenterPixel();

    m_renderer.beginFrame(256, 256, {0.0f, 0.0f, 0.0f, 1.0f}, false);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, nearRed, testCam, redColor, 0.0f);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, farBlue, testCam, blueColor, 0.0f);
    m_renderer.endFrame();
    const auto pxDepthDisabled = readCenterPixel();

    graphics::Framebuffer::unbind();
    fbo.destroy();

    const bool pass1IsRed = (pxOrderFarFirst[0] > 240 && pxOrderFarFirst[1] < 15 && pxOrderFarFirst[2] < 15);
    const bool pass2IsRed = (pxOrderNearFirst[0] > 240 && pxOrderNearFirst[1] < 15 && pxOrderNearFirst[2] < 15);
    const bool pass3IsBlue = (pxDepthDisabled[2] > 240 && pxDepthDisabled[0] < 15 && pxDepthDisabled[1] < 15);

    std::cout << "  [DepthTest] Far->Near (Depth ON)  RGB="
              << static_cast<int>(pxOrderFarFirst[0]) << ","
              << static_cast<int>(pxOrderFarFirst[1]) << ","
              << static_cast<int>(pxOrderFarFirst[2]) << std::endl;
    std::cout << "  [DepthTest] Near->Far (Depth ON)  RGB="
              << static_cast<int>(pxOrderNearFirst[0]) << ","
              << static_cast<int>(pxOrderNearFirst[1]) << ","
              << static_cast<int>(pxOrderNearFirst[2]) << std::endl;
    std::cout << "  [DepthTest] Near->Far (Depth OFF) RGB="
              << static_cast<int>(pxDepthDisabled[0]) << ","
              << static_cast<int>(pxDepthDisabled[1]) << ","
              << static_cast<int>(pxDepthDisabled[2]) << std::endl;

    return pass1IsRed && pass2IsRed && pass3IsBlue;
}

bool Application::verifyM1RaiiMoveLifecycle()
{
    graphics::Shader s1;
    if (!s1.loadFromFiles(resolveAssetPath("assets/shaders/grid.vert"),
                          resolveAssetPath("assets/shaders/grid.frag"))) {
        return false;
    }
    const unsigned int shaderHandle = s1.getProgramId();
    graphics::Shader s2(std::move(s1));
    if (s1.isValid() || s1.getProgramId() != 0 || s2.getProgramId() != shaderHandle) {
        return false;
    }
    s2.destroy();
    s2.destroy();

    std::vector<graphics::Vertex> verts;
    std::vector<std::uint32_t> indices;
    graphics::Mesh::buildUnitCubeFixture(verts, indices);
    graphics::Mesh m1;
    if (!m1.create(verts, indices, graphics::PrimitiveTopology::Triangles)) {
        return false;
    }
    const unsigned int vaoHandle = m1.getVaoId();
    graphics::Mesh m2(std::move(m1));
    if (m1.isValid() || m1.getVaoId() != 0 || m2.getVaoId() != vaoHandle) {
        return false;
    }
    m2.destroy();
    m2.destroy();

    graphics::Framebuffer f1;
    if (!f1.create(128, 128)) {
        return false;
    }
    const unsigned int fboHandle = f1.getFboId();
    graphics::Framebuffer f2(std::move(f1));
    if (f1.isComplete() || f1.getFboId() != 0 || f2.getFboId() != fboHandle) {
        return false;
    }
    f2.destroy();
    f2.destroy();
    return true;
}

bool Application::verifyM1MultiCubeRenderedPixels()
{
    if (!m_framebuffer.isComplete() || m_demoCubes.size() < 3) {
        return false;
    }

    const glm::mat4 vp = m_camera.getProjectionMatrix() * m_camera.getViewMatrix();
    const int w = m_framebuffer.getWidth();
    const int h = m_framebuffer.getHeight();

    m_framebuffer.bind();

    const auto sampleWorldPoint = [&](const glm::vec3& worldPos) -> std::array<unsigned char, 4> {
        const glm::vec4 clip = vp * glm::vec4(worldPos, 1.0f);
        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        const int px = std::clamp(static_cast<int>((ndc.x * 0.5f + 0.5f) * static_cast<float>(w)), 0, w - 1);
        const int py = std::clamp(static_cast<int>((ndc.y * 0.5f + 0.5f) * static_cast<float>(h)), 0, h - 1);
        std::array<unsigned char, 4> rgba{0, 0, 0, 0};
        glReadPixels(px, py, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        return rgba;
    };

    const auto pxA = sampleWorldPoint(m_demoCubes[0].transform.position);
    const auto pxB = sampleWorldPoint(m_demoCubes[1].transform.position + glm::vec3(-0.35f, 0.35f, 0.0f));
    const auto pxC = sampleWorldPoint(m_demoCubes[2].transform.position);
    graphics::Framebuffer::unbind();

    const bool cubeAIsRedDominant = (pxA[0] > pxA[1] + 40 && pxA[0] > pxA[2] + 40);
    const bool cubeBIsBlueDominant = (pxB[2] > pxB[0] + 40 && pxB[2] > pxB[1] + 20);
    const bool cubeCIsGreenDominant = (pxC[1] > pxC[0] + 40 && pxC[1] > pxC[2] + 20);

    std::cout << "[TC-M1-12] Multi-cube framebuffer samples: "
              << "CubeA=(" << static_cast<int>(pxA[0]) << "," << static_cast<int>(pxA[1]) << "," << static_cast<int>(pxA[2]) << ") "
              << "CubeB=(" << static_cast<int>(pxB[0]) << "," << static_cast<int>(pxB[1]) << "," << static_cast<int>(pxB[2]) << ") "
              << "CubeC=(" << static_cast<int>(pxC[0]) << "," << static_cast<int>(pxC[1]) << "," << static_cast<int>(pxC[2]) << ")"
              << std::endl;

    const auto& stats = m_renderer.getLastFrameStats();
    const bool statsValid = (stats.drawCalls >= 3 && stats.vertexCount >= 72 && stats.indexCount >= 108);
    return cubeAIsRedDominant && cubeBIsBlueDominant && cubeCIsGreenDominant && statsValid;
}

bool Application::verifyM2ProjectedGeometryRender(const std::string& label,
                                                  const std::vector<UploadedRenderPart>& parts,
                                                  const math::BoundingBox& box,
                                                  const graphics::Camera& cam)
{
    if (parts.empty() || !box.isValid()) {
        return false;
    }

    graphics::Framebuffer fbo;
    if (!fbo.create(320, 320) || !fbo.isComplete()) {
        return false;
    }

    const glm::vec4 clearCol(0.05f, 0.06f, 0.08f, 1.0f);
    fbo.bind();
    m_renderer.beginFrame(320, 320, clearCol, true);

    for (const auto& part : parts) {
        if (part.isLinePrimitive) {
            m_renderer.drawLines(part.mesh, m_gridShader, part.localTransform.getModelMatrix(), cam, part.color);
        } else {
            m_renderer.drawMeshWithMatrix(part.mesh, m_meshShader, part.localTransform.getModelMatrix(), cam, part.color, 0.35f);
        }
    }
    m_renderer.endFrame();

    // Đọc toàn bộ ảnh FBO 320x320 để xác minh:
    // 1. Góc ngoài cùng (4, 4) vẫn giữ nguyên màu nền (chứng minh không vẽ tràn/rác toàn màn hình)
    // 2. Trong vùng chiếu hình chiếu của BoundingBox có số lượng pixel phủ đáng kể (> 150 px) khác màu nền
    std::vector<unsigned char> pixels(320 * 320 * 4, 0);
    glReadPixels(0, 0, 320, 320, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    graphics::Framebuffer::unbind();
    fbo.destroy();

    const int bgR = static_cast<int>(clearCol.r * 255.0f);
    const int bgG = static_cast<int>(clearCol.g * 255.0f);
    const int bgB = static_cast<int>(clearCol.b * 255.0f);

    const auto pixelDiffFromBg = [&](int x, int y) -> int {
        const std::size_t idx = static_cast<std::size_t>((y * 320 + x) * 4);
        return std::abs(static_cast<int>(pixels[idx]) - bgR) +
               std::abs(static_cast<int>(pixels[idx + 1]) - bgG) +
               std::abs(static_cast<int>(pixels[idx + 2]) - bgB);
    };

    const bool cornerIsBackground = (pixelDiffFromBg(4, 4) < 12);

    // Chiếu 8 góc BoundingBox để xác định hình chữ nhật màn hình dự kiến của vật thể
    const glm::mat4 vp = cam.getProjectionMatrix() * cam.getViewMatrix();
    int minPx = 319, maxPx = 0, minPy = 319, maxPy = 0;
    const glm::vec3 corners[8] = {
        {box.min.x, box.min.y, box.min.z}, {box.max.x, box.min.y, box.min.z},
        {box.min.x, box.max.y, box.min.z}, {box.max.x, box.max.y, box.min.z},
        {box.min.x, box.min.y, box.max.z}, {box.max.x, box.min.y, box.max.z},
        {box.min.x, box.max.y, box.max.z}, {box.max.x, box.max.y, box.max.z}
    };
    for (const auto& c : corners) {
        const glm::vec4 clip = vp * glm::vec4(c, 1.0f);
        if (clip.w <= 1e-4f) {
            continue;
        }
        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        const int sx = std::clamp(static_cast<int>((ndc.x * 0.5f + 0.5f) * 320.0f), 0, 319);
        const int sy = std::clamp(static_cast<int>((ndc.y * 0.5f + 0.5f) * 320.0f), 0, 319);
        minPx = std::min(minPx, sx);
        maxPx = std::max(maxPx, sx);
        minPy = std::min(minPy, sy);
        maxPy = std::max(maxPy, sy);
    }

    int coveredPixelsInsideBBox = 0;
    for (int y = minPy; y <= maxPy; ++y) {
        for (int x = minPx; x <= maxPx; ++x) {
            if (pixelDiffFromBg(x, y) > 25) {
                ++coveredPixelsInsideBBox;
            }
        }
    }

    const auto& stats = m_renderer.getLastFrameStats();
    const bool passed = cornerIsBackground && (coveredPixelsInsideBBox > 150) && (stats.drawCalls == parts.size());

    std::cout << "  [M2-Render] " << label
              << " | drawCalls=" << stats.drawCalls
              << " | tris=" << stats.triangleCount
              << " | bboxCoveragePx=" << coveredPixelsInsideBBox
              << " | cornerBg=" << (cornerIsBackground ? "OK" : "FAIL")
              << " -> " << (passed ? "PASS" : "FAIL") << std::endl;
    return passed;
}

bool Application::verifyM2WireframeStateRestoration()
{
    graphics::Framebuffer fbo;
    if (!fbo.create(128, 128) || !fbo.isComplete()) {
        return false;
    }

    graphics::Camera cam;
    cam.setAspectRatio(1.0f);
    cam.lookAt({2.0f, 1.5f, 2.0f}, {0.0f, 0.5f, 0.0f});

    fbo.bind();
    m_renderer.beginFrame(128, 128, {0.1f, 0.1f, 0.1f, 1.0f}, true);
    m_renderer.setWireframeMode(true);

    GLint modeDuringWireframe[2] = {0, 0};
    glGetIntegerv(GL_POLYGON_MODE, modeDuringWireframe);
    m_renderer.drawMesh(m_cubeMesh, m_meshShader, scene::Transform(), cam, {0.8f, 0.8f, 0.8f, 1.0f}, 0.0f);
    m_renderer.endFrame();

    GLint modeAfterEndFrame[2] = {0, 0};
    glGetIntegerv(GL_POLYGON_MODE, modeAfterEndFrame);

    graphics::Framebuffer::unbind();
    fbo.destroy();

    return (modeDuringWireframe[0] == GL_LINE) && (modeAfterEndFrame[0] == GL_FILL);
}

bool Application::runM2IntegrationSuiteOnce()
{
    bool allPassed = true;
    const auto report = [&allPassed](const char* id, const char* desc, bool ok) {
        std::cout << "[" << id << "] " << desc << ": " << (ok ? "PASS" : "FAIL") << std::endl;
        if (!ok) {
            allPassed = false;
        }
    };

    graphics::Camera testCam;
    testCam.setProjectionMode(graphics::ProjectionMode::Perspective);
    testCam.setAspectRatio(1.0f);
    testCam.setFieldOfView(45.0f);
    testCam.lookAt({2.2f, 1.8f, 2.6f}, {0.0f, 0.55f, 0.0f}, {0.0f, 1.0f, 0.0f});

    // TC-M2-19: Kiểm thử regeneration lặp lại & bảo toàn mesh cũ khi tham số không hợp lệ
    bool regenOk = true;
    {
        m_category = PlaygroundCategory::Primitives;
        m_primitiveTypeIndex = 3; // Cylinder
        m_cylSegments = 8;
        regenOk &= regenerateActivePlaygroundGeometry();
        const std::uint32_t verts8 = m_activeVertexCount;

        m_cylSegments = 32;
        regenOk &= regenerateActivePlaygroundGeometry();
        const std::uint32_t verts32 = m_activeVertexCount;
        regenOk &= (verts32 > verts8);

        // Thử tham số lỗi (bán kính âm) -> phải trả false và GIỮ NGUYÊN mesh hợp lệ (verts32)
        const float savedRadius = m_cylRadius;
        m_cylRadius = -1.0f;
        const bool rejectedBad = !regenerateActivePlaygroundGeometry();
        regenOk &= rejectedBad && (m_activeVertexCount == verts32) && (!m_activeParts.empty()) && m_activeParts[0].mesh.isValid();
        m_cylRadius = savedRadius;
    }
    report("TC-M2-19", "Mesh regeneration (8->32 segs) and prior-mesh preservation on invalid input", regenOk);

    // TC-M2-20: Kiểm thử GPU render từng nhóm (Primitives, Bezier Surface, Revolution Surface, 6 Furniture models)
    bool categoryRenderOk = true;
    const auto renderPlaygroundItem = [&](const std::string& label, PlaygroundCategory cat, int idx, const graphics::Camera& cam) {
        m_category = cat;
        if (cat == PlaygroundCategory::Primitives) m_primitiveTypeIndex = idx;
        else if (cat == PlaygroundCategory::Parametric) m_parametricTypeIndex = idx;
        else if (cat == PlaygroundCategory::Furniture) m_furnitureTypeIndex = idx;

        if (!regenerateActivePlaygroundGeometry()) {
            categoryRenderOk = false;
            return;
        }
        categoryRenderOk &= verifyM2ProjectedGeometryRender(label, m_activeParts, m_activeBoundingBox, cam);
    };

    renderPlaygroundItem("Primitive_Cuboid", PlaygroundCategory::Primitives, 2, testCam);
    renderPlaygroundItem("Primitive_Cylinder", PlaygroundCategory::Primitives, 3, testCam);
    renderPlaygroundItem("Primitive_Sphere", PlaygroundCategory::Primitives, 4, testCam);
    renderPlaygroundItem("Primitive_Frustum", PlaygroundCategory::Primitives, 6, testCam);
    renderPlaygroundItem("Parametric_BezierSurface", PlaygroundCategory::Parametric, 1, testCam);
    renderPlaygroundItem("Parametric_RevolutionSurface", PlaygroundCategory::Parametric, 2, testCam);

    graphics::Camera closeCam;
    closeCam.setAspectRatio(1.0f);
    closeCam.lookAt({0.9f, 0.7f, 1.1f}, {0.0f, 0.25f, 0.0f}, {0.0f, 1.0f, 0.0f});

    graphics::Camera tallCam;
    tallCam.setAspectRatio(1.0f);
    tallCam.lookAt({2.6f, 1.6f, 3.0f}, {0.0f, 0.85f, 0.0f}, {0.0f, 1.0f, 0.0f});

    renderPlaygroundItem("Furniture_Desk", PlaygroundCategory::Furniture, 0, testCam);
    renderPlaygroundItem("Furniture_Chair", PlaygroundCategory::Furniture, 1, testCam);
    renderPlaygroundItem("Furniture_Bookshelf", PlaygroundCategory::Furniture, 2, tallCam);
    renderPlaygroundItem("Furniture_DeskLamp", PlaygroundCategory::Furniture, 3, closeCam);
    renderPlaygroundItem("Furniture_Vase", PlaygroundCategory::Furniture, 4, closeCam);
    renderPlaygroundItem("Furniture_CurvedChair", PlaygroundCategory::Furniture, 5, testCam);

    report("TC-M2-20", "GPU rendering & projected BBox coverage across all M2 categories", categoryRenderOk);

    // TC-M2-21: Kiểm thử khôi phục trạng thái Wireframe GL_LINE -> GL_FILL
    report("TC-M2-21", "Wireframe polygon mode restored to GL_FILL after endFrame()",
           verifyM2WireframeStateRestoration());

    // TC-M2-22: Kiểm thử vòng đời tài nguyên GPU sau nhiều chu kỳ regeneration
    bool lifecycleOk = true;
    for (int iter = 0; iter < 5; ++iter) {
        m_category = PlaygroundCategory::Furniture;
        m_furnitureTypeIndex = iter % 6;
        lifecycleOk &= regenerateActivePlaygroundGeometry();
    }
    report("TC-M2-22", "Repeated GPU upload/release lifecycle across furniture models", lifecycleOk);

    drainAndCheckOpenGlErrors();
    report("TC-M2-23", "Zero unexpected OpenGL errors during M2 integration suite", !m_glErrorDetected);
    return allPassed;
}

void Application::handleReshape(int width, int height)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    glViewport(0, 0, width, height);

    if (m_imguiInitialized) {
        ImGui_ImplGLUT_ReshapeFunc(width, height);
    }

    if (m_config.mode == ApplicationMode::SmokeTestM0 && m_m0ResizeTriggered &&
        width == 960 && height == 540) {
        m_m0ResizeObserved = true;
    }
}

void Application::handleKeyboard(unsigned char key, int x, int y)
{
    if (m_imguiInitialized) {
        ImGui_ImplGLUT_KeyboardFunc(key, x, y);
        if (ImGui::GetIO().WantCaptureKeyboard) {
            return;
        }
    }

    if (key == 27) {
        m_window.requestClose();
    }
}

void Application::handleClose()
{
    shutdownGraphics();
}

void Application::drainAndCheckOpenGlErrors()
{
    GLenum err = glGetError();
    while (err != GL_NO_ERROR) {
        m_glErrorDetected = true;
        std::cerr << "[RoomCraft3D][ERROR] OpenGL error code: 0x" << std::hex << err << std::dec << std::endl;
        err = glGetError();
    }
}

} // namespace roomcraft::core
