cmake_minimum_required(VERSION 3.20)

project(RoomCraft3D
    VERSION 0.6.0
    DESCRIPTION "RoomCraft 3D - Desktop 3D Interior Design and Room Planner"
    LANGUAGES C CXX
)

# Bắt buộc chuẩn C++17 trên toàn bộ project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Cho phép các thư viện CMake cũ tương thích khi dùng CMake 4.x
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

# Tự động phát hiện nếu đang cấu hình qua vcpkg toolchain
set(_default_use_vcpkg OFF)
if(DEFINED VCPKG_TOOLCHAIN OR (DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg"))
    set(_default_use_vcpkg ON)
endif()
option(ROOMCRAFT_USE_VCPKG "Use dependencies resolved via vcpkg manifest" ${_default_use_vcpkg})

find_package(OpenGL REQUIRED)

if(ROOMCRAFT_USE_VCPKG)
    message(STATUS "[RoomCraft3D] Dependency mode: vcpkg")
    find_package(FreeGLUT CONFIG REQUIRED)
    find_package(glad CONFIG REQUIRED)
    find_package(imgui CONFIG REQUIRED)
    find_package(glm CONFIG REQUIRED)
    find_package(assimp CONFIG REQUIRED)
    find_path(STB_INCLUDE_DIRS "stb_image.h" REQUIRED)

    if(TARGET FreeGLUT::freeglut)
        set(ROOMCRAFT_FREEGLUT_TARGET FreeGLUT::freeglut)
    elseif(TARGET FreeGLUT::freeglut_static)
        set(ROOMCRAFT_FREEGLUT_TARGET FreeGLUT::freeglut_static)
    else()
        message(FATAL_ERROR "FreeGLUT target not found via vcpkg")
    endif()

    set(ROOMCRAFT_GLAD_TARGET glad::glad)
    set(ROOMCRAFT_IMGUI_TARGET imgui::imgui)
    set(ROOMCRAFT_GLM_TARGET glm::glm)
    set(ROOMCRAFT_ASSIMP_TARGET assimp::assimp)

    add_library(roomcraft_stb INTERFACE)
    target_include_directories(roomcraft_stb INTERFACE "${STB_INCLUDE_DIRS}")
    set(ROOMCRAFT_STB_TARGET roomcraft_stb)
else()
    message(STATUS "[RoomCraft3D] Dependency mode: Pinned FetchContent (locked commits/tags)")
    include(FetchContent)

    # 1. FreeGLUT v3.8.0 (khóa commit SHA)
    set(FREEGLUT_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    set(FREEGLUT_BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
    set(FREEGLUT_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(FREEGLUT_GLES OFF CACHE BOOL "" FORCE)
    set(FREEGLUT_REPLACE_GLUT OFF CACHE BOOL "" FORCE)
    set(INSTALL_PDB OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        freeglut
        GIT_REPOSITORY https://github.com/FreeGLUTProject/freeglut.git
        GIT_TAG        3db1649ce1f5e42f1338b51e3fa14849be547d5d # v3.8.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(freeglut)

    if(TARGET freeglut_static)
        add_library(roomcraft_freeglut INTERFACE)
        target_link_libraries(roomcraft_freeglut INTERFACE freeglut_static)
        target_compile_definitions(roomcraft_freeglut INTERFACE FREEGLUT_STATIC)
        target_include_directories(roomcraft_freeglut INTERFACE
            "${freeglut_SOURCE_DIR}/include"
            "${freeglut_BINARY_DIR}/include"
        )
        set(ROOMCRAFT_FREEGLUT_TARGET roomcraft_freeglut)
    else()
        message(FATAL_ERROR "Expected freeglut_static target from FreeGLUT FetchContent")
    endif()

    # 2. GLAD v0.1.36 (OpenGL 3.3 Core Profile, khóa commit SHA)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    if(POLICY CMP0169)
        cmake_policy(SET CMP0169 OLD)
    endif()

    FetchContent_Declare(
        glad_src
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG        1ecd45775d96f35170458e6b148eb0708967e402 # v0.1.36
        GIT_SHALLOW    TRUE
    )
    FetchContent_GetProperties(glad_src)
    if(NOT glad_src_POPULATED)
        FetchContent_Populate(glad_src)
    endif()

    set(GLAD_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/glad")
    if(NOT EXISTS "${GLAD_OUT_DIR}/src/glad.c" OR NOT EXISTS "${GLAD_OUT_DIR}/include/glad/glad.h")
        message(STATUS "[RoomCraft3D] Generating GLAD OpenGL 3.3 Core loader...")
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" -m glad
                --profile core
                --api gl=3.3
                --generator c
                --spec gl
                --reproducible
                --out-path "${GLAD_OUT_DIR}"
            WORKING_DIRECTORY "${glad_src_SOURCE_DIR}"
            RESULT_VARIABLE GLAD_GEN_RESULT
            OUTPUT_VARIABLE GLAD_GEN_OUT
            ERROR_VARIABLE  GLAD_GEN_ERR
        )
        if(NOT GLAD_GEN_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to generate GLAD OpenGL 3.3 Core loader:\n${GLAD_GEN_OUT}\n${GLAD_GEN_ERR}")
        endif()
    endif()

    add_library(glad STATIC "${GLAD_OUT_DIR}/src/glad.c")
    target_include_directories(glad PUBLIC "${GLAD_OUT_DIR}/include")
    set(ROOMCRAFT_GLAD_TARGET glad)

    # 3. Dear ImGui v1.93.0-docking (khóa commit SHA, backend FreeGLUT + OpenGL3)
    FetchContent_Declare(
        imgui_src
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        b916186e66aa3c02b72c6ca17455516512ed8049 # v1.93.0-docking
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(imgui_src)

    add_library(imgui STATIC
        "${imgui_src_SOURCE_DIR}/imgui.cpp"
        "${imgui_src_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_src_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_src_SOURCE_DIR}/imgui_widgets.cpp"
        "${imgui_src_SOURCE_DIR}/backends/imgui_impl_glut.cpp"
        "${imgui_src_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
    )
    target_include_directories(imgui PUBLIC
        "${imgui_src_SOURCE_DIR}"
        "${imgui_src_SOURCE_DIR}/backends"
    )
    target_link_libraries(imgui PUBLIC
        ${ROOMCRAFT_FREEGLUT_TARGET}
        OpenGL::GL
    )
    set(ROOMCRAFT_IMGUI_TARGET imgui)

    # 4. GLM 1.0.1 (khóa commit SHA)
    set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
    set(GLM_ENABLE_CXX_17 ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        0af55ccecd98d4e5a8d1fad7de25ba429d60e863 # 1.0.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(glm)

    if(TARGET glm::glm-header-only)
        set(ROOMCRAFT_GLM_TARGET glm::glm-header-only)
    elseif(TARGET glm::glm)
        set(ROOMCRAFT_GLM_TARGET glm::glm)
    else()
        message(FATAL_ERROR "Expected glm target from GLM FetchContent")
    endif()

    # 5. Assimp v6.0.6 (khóa commit SHA, liên kết tĩnh, CHỈ bật OBJ importer để build nhanh & gọn)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)

    # Trên Windows với WinLibs CMake (GnuTLS không kèm CAfile mặc định), tự động trỏ tới CA bundle của Git
    if(WIN32 AND NOT DEFINED CMAKE_TLS_CAINFO)
        if(EXISTS "C:/Program Files/Git/mingw64/etc/ssl/certs/ca-bundle.crt")
            set(CMAKE_TLS_CAINFO "C:/Program Files/Git/mingw64/etc/ssl/certs/ca-bundle.crt" CACHE FILEPATH "" FORCE)
        endif()
    endif()

    # 5. Assimp v6.0.6 (tag commit a79ff6bbce54d3e01186a918dc6374ed55317426, khóa SHA256 archive)
    FetchContent_Declare(
        assimp
        URL https://github.com/assimp/assimp/archive/refs/tags/v6.0.6.tar.gz
        URL_HASH SHA256=5f7ed412b76575621df0525a762b31da5874afff352b3a890c6308db4ac7204c
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(assimp)

    if(TARGET assimp::assimp)
        set(ROOMCRAFT_ASSIMP_TARGET assimp::assimp)
    elseif(TARGET assimp)
        set(ROOMCRAFT_ASSIMP_TARGET assimp)
    else()
        message(FATAL_ERROR "Expected assimp target from Assimp FetchContent")
    endif()

    # 6. stb (stb_image.h v2.34, khóa commit SHA + SHA256 archive)
    FetchContent_Declare(
        stb_src
        URL https://github.com/nothings/stb/archive/6b5799bae8a1fe3792691ef4b72ae26434c3e799.tar.gz
        URL_HASH SHA256=87e9ecd2d2e0cb8fa35bf5a93ea6ca3999595ea4f0a4738ed1aac0a34cbd4da9
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(stb_src)

    add_library(roomcraft_stb INTERFACE)
    target_include_directories(roomcraft_stb INTERFACE "${stb_src_SOURCE_DIR}")
    set(ROOMCRAFT_STB_TARGET roomcraft_stb)
endif()

# ==============================================================================
# 1. roomcraft_math (CPU-only): Transform, Camera (+Auto-Fit), BoundingBox, Ray/Plane/Intersection, MeshValidation
# ==============================================================================
add_library(roomcraft_math STATIC
    src/scene/Transform.cpp
    src/graphics/Camera.cpp
    src/graphics/MeshValidation.cpp
    src/math/BoundingBox.cpp
    src/math/Intersection.cpp
)
target_include_directories(roomcraft_math PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(roomcraft_math PUBLIC
    ${ROOMCRAFT_GLM_TARGET}
)

# ==============================================================================
# 2. roomcraft_geometry (CPU-only, KHÔNG phụ thuộc OpenGL/FreeGLUT/ImGui)
# ==============================================================================
add_library(roomcraft_geometry STATIC
    src/geometry/MeshData.cpp
    src/geometry/GeometryUtils.cpp
    src/geometry/PrimitiveGenerator.cpp
    src/geometry/BezierCurve.cpp
    src/geometry/BezierSurface.cpp
    src/geometry/RevolutionSurface.cpp
    src/geometry/FurnitureGenerator.cpp
)
target_include_directories(roomcraft_geometry PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(roomcraft_geometry PUBLIC
    roomcraft_math
)

# ==============================================================================
# 3. roomcraft_assets (CPU-only, liên kết Assimp, KHÔNG phụ thuộc OpenGL/GLUT/ImGui):
#    AssetData, AssetNormalizer, ModelLoader, FurnitureCatalog
# ==============================================================================
add_library(roomcraft_assets STATIC
    src/assets/AssetNormalizer.cpp
    src/assets/ModelLoader.cpp
    src/assets/FurnitureCatalog.cpp
)
target_include_directories(roomcraft_assets PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(roomcraft_assets PUBLIC
    roomcraft_geometry
    ${ROOMCRAFT_ASSIMP_TARGET}
)

# ==============================================================================
# 4. roomcraft_scene (CPU-only, KHÔNG phụ thuộc OpenGL/FreeGLUT/ImGui):
#    Scene/Room/Presets + M5 CameraController/Picking/Transform + M6 TransformGizmo/EditorLayout
# ==============================================================================
add_library(roomcraft_scene STATIC
    src/scene/Room.cpp
    src/scene/Scene.cpp
    src/scene/ScenePresets.cpp
    src/systems/PlacementSystem.cpp
    src/systems/CameraController.cpp
    src/systems/PickingSystem.cpp
    src/systems/TransformController.cpp
    src/systems/TransformGizmo.cpp
    src/core/InputManager.cpp
    src/editor/EditorLayout.cpp
    src/editor/ViewportInteraction.cpp
)
target_include_directories(roomcraft_scene PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(roomcraft_scene PUBLIC
    roomcraft_assets
)

# ==============================================================================
# 5. roomcraft_engine (GPU + Window + ImGui): Core Engine, Thumbnails, Design System, RoomEditor, App
# ==============================================================================
add_library(roomcraft_engine STATIC
    src/core/Window.cpp
    src/core/Application.cpp
    src/graphics/Shader.cpp
    src/graphics/Mesh.cpp
    src/graphics/Framebuffer.cpp
    src/graphics/Renderer.cpp
    src/graphics/Texture.cpp
    src/graphics/TextureManager.cpp
    src/graphics/ThumbnailRenderer.cpp
    src/graphics/SceneRenderer.cpp
    src/assets/AssetManager.cpp
    src/editor/EditorTheme.cpp
    src/editor/EditorIcons.cpp
    src/editor/RoomEditor.cpp
)
target_include_directories(roomcraft_engine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(roomcraft_engine PUBLIC
    roomcraft_scene
    ${ROOMCRAFT_STB_TARGET}
    ${ROOMCRAFT_GLAD_TARGET}
    ${ROOMCRAFT_FREEGLUT_TARGET}
    ${ROOMCRAFT_IMGUI_TARGET}
    OpenGL::GL
)

if(WIN32)
    target_link_libraries(roomcraft_engine PUBLIC winmm gdi32 user32)
endif()

# Cảnh báo biên dịch chặt cho toàn bộ mã nguồn dự án
if(MSVC)
    target_compile_options(roomcraft_math PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(roomcraft_geometry PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(roomcraft_assets PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(roomcraft_scene PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(roomcraft_engine PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(roomcraft_math PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(roomcraft_geometry PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(roomcraft_assets PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(roomcraft_scene PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(roomcraft_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Executable chính RoomCraft3D
add_executable(RoomCraft3D
    src/main.cpp
)
target_link_libraries(RoomCraft3D PRIVATE roomcraft_engine)

if(MSVC)
    target_compile_options(RoomCraft3D PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(RoomCraft3D PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(MINGW)
    target_link_options(RoomCraft3D PRIVATE -static)
endif()

add_custom_command(TARGET RoomCraft3D POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:RoomCraft3D>/assets"
    COMMENT "Copying RoomCraft 3D assets (shaders, models, textures) next to executable"
)

# ==============================================================================
# Kiểm thử tự động (CTest): M0/M1/M2 Regression + M3 Asset Unit & GPU Tests
# ==============================================================================
enable_testing()

# 1. M1 Unit Tests (CPU-only)
add_executable(RoomCraft3D_UnitTests
    tests/unit/TestMain.cpp
    tests/unit/TransformTests.cpp
    tests/unit/CameraTests.cpp
    tests/unit/MeshValidationTests.cpp
)
target_link_libraries(RoomCraft3D_UnitTests PRIVATE roomcraft_math)

# 2. M2 Geometry Unit Tests (CPU-only)
add_executable(RoomCraft3D_GeometryUnitTests
    tests/unit/GeometryTestMain.cpp
    tests/unit/PrimitiveTests.cpp
    tests/unit/BezierCurveTests.cpp
    tests/unit/BezierSurfaceTests.cpp
    tests/unit/RevolutionSurfaceTests.cpp
    tests/unit/BoundingBoxTests.cpp
    tests/unit/FurnitureTests.cpp
)
target_link_libraries(RoomCraft3D_GeometryUnitTests PRIVATE roomcraft_geometry)

# 3. M3 Asset Pipeline Unit Tests (CPU-only, không cần OpenGL context)
add_executable(RoomCraft3D_AssetUnitTests
    tests/unit/AssetTestMain.cpp
    tests/unit/ModelLoaderTests.cpp
    tests/unit/AssetNormalizerTests.cpp
    tests/unit/AssetCatalogTests.cpp
)
target_link_libraries(RoomCraft3D_AssetUnitTests PRIVATE roomcraft_assets)
target_compile_definitions(RoomCraft3D_AssetUnitTests PRIVATE
    ROOMCRAFT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# 4. M4 Scene Management Unit Tests (CPU-only, không cần OpenGL context)
add_executable(RoomCraft3D_SceneUnitTests
    tests/unit/SceneTestMain.cpp
    tests/unit/RoomTests.cpp
    tests/unit/SceneTests.cpp
    tests/unit/PlacementTests.cpp
    tests/unit/ScenePresetTests.cpp
)
target_link_libraries(RoomCraft3D_SceneUnitTests PRIVATE roomcraft_scene)

# 5. M5 Camera, Ray Casting, Object Picking & Interaction Unit Tests (CPU-only, không cần OpenGL context)
add_executable(RoomCraft3D_InteractionUnitTests
    tests/unit/InteractionTestMain.cpp
    tests/unit/RayTests.cpp
    tests/unit/IntersectionTests.cpp
    tests/unit/CameraControllerTests.cpp
    tests/unit/PickingTests.cpp
    tests/unit/InteractionTests.cpp
)
target_link_libraries(RoomCraft3D_InteractionUnitTests PRIVATE roomcraft_scene)

# 6. M6 Editor Layout, Notifications & Move Gizmo Unit Tests (CPU-only, không cần OpenGL context)
add_executable(RoomCraft3D_EditorUiUnitTests
    tests/unit/EditorUiTests.cpp
)
target_link_libraries(RoomCraft3D_EditorUiUnitTests PRIVATE roomcraft_scene)

if(MSVC)
    target_compile_options(RoomCraft3D_UnitTests PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(RoomCraft3D_GeometryUnitTests PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(RoomCraft3D_AssetUnitTests PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(RoomCraft3D_SceneUnitTests PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(RoomCraft3D_InteractionUnitTests PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(RoomCraft3D_EditorUiUnitTests PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(RoomCraft3D_UnitTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(RoomCraft3D_GeometryUnitTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(RoomCraft3D_AssetUnitTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(RoomCraft3D_SceneUnitTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(RoomCraft3D_InteractionUnitTests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(RoomCraft3D_EditorUiUnitTests PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(MINGW)
    target_link_options(RoomCraft3D_UnitTests PRIVATE -static)
    target_link_options(RoomCraft3D_GeometryUnitTests PRIVATE -static)
    target_link_options(RoomCraft3D_AssetUnitTests PRIVATE -static)
    target_link_options(RoomCraft3D_SceneUnitTests PRIVATE -static)
    target_link_options(RoomCraft3D_InteractionUnitTests PRIVATE -static)
    target_link_options(RoomCraft3D_EditorUiUnitTests PRIVATE -static)
endif()

add_test(NAME M0_SmokeTest             COMMAND $<TARGET_FILE:RoomCraft3D> --smoke-test)
add_test(NAME M1_UnitTests             COMMAND $<TARGET_FILE:RoomCraft3D_UnitTests>)
add_test(NAME M1_IntegrationSmokeTest  COMMAND $<TARGET_FILE:RoomCraft3D> --m1-smoke-test)
add_test(NAME M2_GeometryUnitTests     COMMAND $<TARGET_FILE:RoomCraft3D_GeometryUnitTests>)
add_test(NAME M2_IntegrationSmokeTest  COMMAND $<TARGET_FILE:RoomCraft3D> --m2-smoke-test)
add_test(NAME M3_AssetUnitTests        COMMAND $<TARGET_FILE:RoomCraft3D_AssetUnitTests>)
add_test(NAME M3_IntegrationSmokeTest  COMMAND $<TARGET_FILE:RoomCraft3D> --m3-smoke-test)
add_test(NAME M4_SceneUnitTests        COMMAND $<TARGET_FILE:RoomCraft3D_SceneUnitTests>)
add_test(NAME M4_IntegrationSmokeTest  COMMAND $<TARGET_FILE:RoomCraft3D> --m4-smoke-test)
add_test(NAME M5_InteractionUnitTests  COMMAND $<TARGET_FILE:RoomCraft3D_InteractionUnitTests>)
add_test(NAME M5_IntegrationSmokeTest  COMMAND $<TARGET_FILE:RoomCraft3D> --m5-smoke-test)
add_test(NAME M6_EditorUiUnitTests     COMMAND $<TARGET_FILE:RoomCraft3D_EditorUiUnitTests>)
add_test(NAME M6_IntegrationSmokeTest  COMMAND $<TARGET_FILE:RoomCraft3D> --m6-smoke-test)
