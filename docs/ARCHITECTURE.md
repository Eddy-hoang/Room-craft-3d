# Kiến trúc phần mềm — RoomCraft 3D (Architecture)

## 1. Kiến trúc tổng thể theo tầng (Layered Modular Architecture)

```mermaid
flowchart TD
    UI["Presentation Layer\n(Dear ImGui: Diagnostics + 3D Viewport)"] --> APP["Application Layer\n(Application + Window + Demo Scene)"]
    APP --> GFX["Core Graphics Engine\n(Renderer, Shader, Mesh, Vertex, Camera, Framebuffer)"]
    APP --> SCENE["Scene Math Layer\n(Transform)"]
    GFX --> GL["OpenGL 3.3 Core · GLSL 330 · GLAD · GLM"]
    APP --> GLUT["FreeGLUT Window & Event Loop"]
```

## 2. Sơ đồ lớp Core Graphics Engine (Milestone M1)

```mermaid
classDiagram
    class Application {
        -Window m_window
        -Renderer m_renderer
        -Camera m_camera
        -Framebuffer m_framebuffer
        -Shader m_meshShader
        -Shader m_gridShader
        -Mesh m_cubeMesh
        +initialize(argc, argv) bool
        +run() int
        -shutdownGraphics() void
    }

    class Window {
        -GpuInfo m_gpuInfo
        -int m_width
        -int m_height
        +initialize(argc, argv) bool
        +setCallbacks(callbacks) void
        +runMainLoop() void
        +requestClose() void
        +swapBuffers() void
    }

    class Renderer {
        -RenderStats m_lastFrameStats
        +beginFrame(w, h, clearColor, depthTest) void
        +drawMesh(mesh, shader, transform, camera, color) void
        +drawLines(lineMesh, shader, model, camera, color) void
        +endFrame() void
        +getLastFrameStats() RenderStats
    }

    class Shader {
        -unsigned int m_programId
        +loadFromFiles(vertPath, fragPath) bool
        +loadFromSource(vertSrc, fragSrc) bool
        +bind() void
        +setMat4(name, value) void
        +setVec4(name, value) void
        +destroy() void
    }

    class Mesh {
        -unsigned int m_vao
        -unsigned int m_vbo
        -unsigned int m_ebo
        -uint32_t m_vertexCount
        -uint32_t m_indexCount
        +validateData(vertices, indices, topology)$ bool
        +create(vertices, indices, topology) bool
        +draw() void
        +destroy() void
    }

    class Vertex {
        +glm::vec3 position
        +glm::vec3 normal
        +glm::vec2 texCoord
    }

    class Transform {
        +glm::vec3 position
        +glm::vec3 rotationDegrees
        +glm::vec3 scale
        +getModelMatrix() glm::mat4
        +reset() void
    }

    class Camera {
        -glm::vec3 m_position
        -glm::vec3 m_target
        -ProjectionMode m_projectionMode
        +lookAt(pos, target, up) bool
        +getViewMatrix() glm::mat4
        +getProjectionMatrix() glm::mat4
        +reset() void
    }

    class Framebuffer {
        -unsigned int m_fbo
        -unsigned int m_colorTexture
        -unsigned int m_depthStencilRbo
        +create(w, h) bool
        +resize(w, h) bool
        +bind() void
        +unbind()$ void
        +isComplete() bool
        +destroy() void
    }

    Application *-- Window
    Application *-- Renderer
    Application *-- Camera
    Application *-- Framebuffer
    Application *-- Shader
    Application *-- Mesh
    Application *-- Transform
    Mesh ..> Vertex : uploads
    Renderer ..> Mesh : draws
    Renderer ..> Shader : binds
    Renderer ..> Transform : reads Model
    Renderer ..> Camera : reads View/Proj
```

## 3. Luồng Graphics Pipeline (Off-screen Framebuffer -> Dear ImGui Viewport)

```mermaid
flowchart LR
    CPU["CPU Geometry\n(Vertex + Indices)"] --> VAL["Mesh::validateData()"]
    VAL --> GPU["VAO + VBO + EBO\n(GPU Static Buffers)"]
    GPU --> VS["Vertex Shader\n(mesh.vert / grid.vert)\np_clip = P * V * M * p_local"]
    VS --> RAST["Primitive Assembly\n& Rasterization"]
    RAST --> FS["Fragment Shader\n(mesh.frag / grid.frag)"]
    FS --> DEPTH["Depth Testing\n(GL_DEPTH_TEST, GL_LESS)"]
    DEPTH --> FBO["Off-screen Framebuffer\n(RGBA8 Texture + DEPTH24_STENCIL8)"]
    FBO --> IMGUI["ImGui::Image\n(UV flipped: 0,1 -> 1,0)"]
    IMGUI --> SCREEN["Default Framebuffer\n& glutSwapBuffers()"]
```

## 4. Tách lớp thư viện trong CMake (M1)

Để hỗ trợ kiểm thử đơn vị (Unit Testing) độc lập với GPU/OpenGL:
- `roomcraft_math`: Chỉ chứa `Transform`, `Camera` và `MeshValidation` (chỉ phụ thuộc `GLM`). Được liên kết trực tiếp bởi `RoomCraft3D_UnitTests` để chạy nhanh, không cần tạo cửa sổ hay OpenGL context.
- `roomcraft_engine`: Chứa toàn bộ lớp điều khiển OpenGL/FreeGLUT/ImGui (`Window`, `Shader`, `Mesh`, `Framebuffer`, `Renderer`, `Application`).
- `RoomCraft3D`: Executable chính liên kết `roomcraft_engine`, hỗ trợ chế độ tương tác mặc định, `--smoke-test` (M0 regression) và `--m1-smoke-test` (M1 GPU integration).

## 5. Nguyên tắc quản lý vòng đời tài nguyên GPU

- Toàn bộ lớp sở hữu GPU handle (`Shader`, `Mesh`, `Framebuffer`) áp dụng **RAII**: cấm copy (`= delete`), hỗ trợ move (`std::exchange(..., 0)`) và có phương thức `destroy()` idempotent (chống double-delete).
- Callback `Window::onClose` gọi `Application::shutdownGraphics()` **trước khi** FreeGLUT hủy OpenGL context để mọi lệnh `glDelete*` và `ImGui_ImplOpenGL3_Shutdown()` luôn thực thi trên context hợp lệ.
