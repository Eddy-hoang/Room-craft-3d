#include "roomcraft/scene/ScenePresets.hpp"

#include <cmath>
#include <vector>

namespace roomcraft::scene {

namespace {

struct PresetObjectSpec {
    const char* assetId;
    const char* name;
    glm::vec3 position;
    glm::vec3 rotationDegrees;
    glm::vec3 scale;
};

bool populatePresetObjects(Scene& scene,
                           const assets::FurnitureCatalog& catalog,
                           const std::vector<PresetObjectSpec>& specs,
                           std::string* outError)
{
    for (const auto& spec : specs) {
        Transform tr(spec.position, spec.rotationDegrees, spec.scale);
        const ObjectId id = scene.addObject(spec.assetId, spec.name, tr, catalog, outError);
        if (!isValidObjectId(id)) {
            return false;
        }
    }
    return true;
}

} // namespace

bool ScenePresets::applyPreset(ScenePresetType preset,
                               Scene& scene,
                               const assets::FurnitureCatalog& catalog,
                               std::string* outError)
{
    RoomSettings roomSettings;
    roomSettings.wallVisibilityMode = WallVisibilityMode::CutawayView;
    roomSettings.showGrid = true;

    std::vector<PresetObjectSpec> specs;

    switch (preset) {
        case ScenePresetType::ModernBedroom: {
            // Phòng ngủ hiện đại kết hợp góc học tập: 5.0 x 6.0 x 3.0 m
            // Phạm vi bên trong: X in [-2.5, 2.5], Z in [-3.0, 3.0], Y in [0, 3.0]
            roomSettings.width = 5.0f;
            roomSettings.length = 6.0f;
            roomSettings.height = 3.0f;
            roomSettings.floorColor = glm::vec4(0.75f, 0.65f, 0.53f, 1.0f);
            roomSettings.wallColor = glm::vec4(0.90f, 0.89f, 0.86f, 1.0f);

            specs = {
                // Thảm nằm trên sàn, lệch nhẹ +3mm theo Y tránh z-fighting
                {"blender-carpet",       "Bedroom Carpet",   {-0.40f, 0.003f, -0.50f}, {0.0f,   0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Giường dựa tường Bắc (-Z)
                {"blender-modern-bed",   "Modern Bed",       {-0.70f, 0.000f, -1.75f}, {0.0f,   0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Tủ quần áo áp tường Tây (-X)
                {"blender-wardrobe",     "Wardrobe",         {-2.10f, 0.000f,  1.20f}, {0.0f,  90.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Bàn học áp tường Bắc phía Đông (mặt bàn cao 0.75m)
                {"procedural-desk",      "Study Desk",       { 1.45f, 0.000f, -2.50f}, {0.0f,   0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Đèn bàn đặt đúng mặt bàn y = 0.75m
                {"procedural-desk-lamp", "Study Desk Lamp",  { 1.90f, 0.750f, -2.58f}, {0.0f, -25.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Ghế gaming quay mặt vào bàn học
                {"blender-gaming-chair", "Study Chair",      { 1.45f, 0.000f, -1.72f}, {0.0f, 180.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Cây cảnh trang trí góc Đông-Nam
                {"blender-indoor-plant", "Corner Plant",     { 1.85f, 0.000f,  2.15f}, {0.0f, -30.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}
            };
            break;
        }

        case ScenePresetType::GamingWorkspace: {
            // Không gian học tập & làm việc cá nhân: 4.0 x 5.0 x 3.0 m
            // Phạm vi bên trong: X in [-2.0, 2.0], Z in [-2.5, 2.5], Y in [0, 3.0]
            roomSettings.width = 4.0f;
            roomSettings.length = 5.0f;
            roomSettings.height = 3.0f;
            roomSettings.floorColor = glm::vec4(0.58f, 0.52f, 0.46f, 1.0f);
            roomSettings.wallColor = glm::vec4(0.82f, 0.84f, 0.87f, 1.0f);

            specs = {
                {"blender-carpet",         "Workspace Carpet", { 0.00f, 0.003f,  0.20f}, {0.0f,   0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Bàn làm việc áp tường Bắc (-Z), mặt bàn cao 0.75m
                {"procedural-desk",        "Main Gaming Desk", { 0.00f, 0.000f, -2.00f}, {0.0f,   0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Bộ máy tính đặt chính xác trên mặt bàn y = 0.75m
                {"blender-computer-setup", "Computer Setup",   {-0.10f, 0.750f, -2.02f}, {0.0f,   0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Đèn bàn đặt góc phải mặt bàn y = 0.75m
                {"procedural-desk-lamp",   "Desk Lamp",        { 0.52f, 0.750f, -2.10f}, {0.0f, -35.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Ghế gaming quay vào bàn
                {"blender-gaming-chair",   "Gaming Chair",     { 0.00f, 0.000f, -1.15f}, {0.0f, 180.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Kệ sách áp tường Tây (-X)
                {"procedural-bookshelf",   "Bookshelf",        {-1.78f, 0.000f,  0.40f}, {0.0f,  90.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Cây cảnh góc Tây-Bắc
                {"blender-indoor-plant",   "Indoor Plant",     {-1.45f, 0.000f, -1.85f}, {0.0f,  20.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}
            };
            break;
        }

        case ScenePresetType::MinimalistLivingRoom: {
            // Phòng khách tối giản Scandinavian: 6.0 x 7.0 x 3.0 m
            // Phạm vi bên trong: X in [-3.0, 3.0], Z in [-3.5, 3.5], Y in [0, 3.0]
            roomSettings.width = 6.0f;
            roomSettings.length = 7.0f;
            roomSettings.height = 3.0f;
            roomSettings.floorColor = glm::vec4(0.79f, 0.71f, 0.59f, 1.0f);
            roomSettings.wallColor = glm::vec4(0.93f, 0.92f, 0.89f, 1.0f);

            specs = {
                {"blender-carpet",       "Living Room Carpet", { 0.00f, 0.003f, -0.35f}, {0.0f,   0.0f, 0.0f}, {1.25f, 1.00f, 1.25f}},
                // Sofa dựa tường Bắc
                {"blender-modern-sofa",  "Modern Sofa",        { 0.00f, 0.000f, -2.55f}, {0.0f,   0.0f, 0.0f}, {1.00f, 1.00f, 1.00f}},
                // Bàn trà từ Procedural Desk thu gọn chiều cao còn 0.45m (0.75 * 0.60 = 0.45)
                {"procedural-desk",      "Coffee Table",       { 0.00f, 0.000f, -0.90f}, {0.0f,   0.0f, 0.0f}, {0.85f, 0.60f, 0.80f}},
                // Bình gốm đặt trên mặt bàn trà y = 0.45m
                {"procedural-vase",      "Ceramic Vase",       { 0.15f, 0.450f, -0.90f}, {0.0f,   0.0f, 0.0f}, {1.00f, 1.00f, 1.00f}},
                // Đèn đứng cạnh sofa
                {"blender-floor-lamp",   "Floor Lamp",         { 1.55f, 0.000f, -2.55f}, {0.0f, -20.0f, 0.0f}, {1.00f, 1.00f, 1.00f}},
                // Kệ sách áp tường Tây (-X)
                {"procedural-bookshelf", "Living Bookshelf",   {-2.78f, 0.000f, -0.50f}, {0.0f,  90.0f, 0.0f}, {1.00f, 1.00f, 1.00f}},
                // Cây cảnh góc Tây-Bắc
                {"blender-indoor-plant", "Living Room Plant",  {-2.20f, 0.000f, -2.65f}, {0.0f,  15.0f, 0.0f}, {1.00f, 1.00f, 1.00f}}
            };
            break;
        }
    }

    if (!scene.getRoom().applySettings(roomSettings, outError)) {
        return false;
    }
    scene.clearObjects();
    return populatePresetObjects(scene, catalog, specs, outError);
}

bool ScenePresets::buildBenchmarkScene(std::size_t objectCount,
                                       Scene& scene,
                                       const assets::FurnitureCatalog& catalog,
                                       std::string* outError)
{
    // Dùng tập 6 asset đại diện (kết hợp Procedural và Blender) lặp lại để kiểm chứng Shared GPU Cache
    static constexpr const char* kBenchAssets[] = {
        "blender-gaming-chair",
        "blender-modern-sofa",
        "procedural-desk",
        "blender-indoor-plant",
        "blender-floor-lamp",
        "procedural-vase"
    };
    constexpr std::size_t kPoolSize = sizeof(kBenchAssets) / sizeof(kBenchAssets[0]);

    const int cols = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(objectCount))));
    const int rows = static_cast<int>(std::ceil(static_cast<double>(objectCount) / static_cast<double>(cols)));

    RoomSettings settings;
    settings.width = std::min(room_limits::kMaxWidth, std::max(6.0f, static_cast<float>(cols) * 1.6f));
    settings.length = std::min(room_limits::kMaxLength, std::max(6.0f, static_cast<float>(rows) * 1.6f));
    settings.height = 3.5f;
    settings.wallVisibilityMode = WallVisibilityMode::CutawayView;

    if (!scene.getRoom().applySettings(settings, outError)) {
        return false;
    }
    scene.clearObjects();

    const float cellW = (settings.width - 1.0f) / static_cast<float>(std::max(1, cols));
    const float cellL = (settings.length - 1.0f) / static_cast<float>(std::max(1, rows));

    for (std::size_t i = 0; i < objectCount; ++i) {
        const int r = static_cast<int>(i) / cols;
        const int c = static_cast<int>(i) % cols;

        const float x = -settings.width * 0.5f + 0.5f + (static_cast<float>(c) + 0.5f) * cellW;
        const float z = -settings.length * 0.5f + 0.5f + (static_cast<float>(r) + 0.5f) * cellL;
        const float yaw = static_cast<float>((i * 45) % 360);

        Transform tr(glm::vec3(x, 0.0f, z), glm::vec3(0.0f, yaw, 0.0f), glm::vec3(1.0f));
        const char* assetId = kBenchAssets[i % kPoolSize];
        const std::string name = "Bench_" + std::to_string(i + 1);
        if (!isValidObjectId(scene.addObject(assetId, name, tr, catalog, outError))) {
            return false;
        }
    }

    return true;
}

} // namespace roomcraft::scene
