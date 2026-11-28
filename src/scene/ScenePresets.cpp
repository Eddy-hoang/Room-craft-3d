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
            // Phòng ngủ hiện đại Scandinavian Modern: 5.0 x 6.0 x 3.0 m
            // Phạm vi bên trong: X in [-2.5, 2.5], Z in [-3.0, 3.0], Y in [0, 3.0]
            roomSettings.width = 5.0f;
            roomSettings.length = 6.0f;
            roomSettings.height = 3.0f;
            roomSettings.floorColor = glm::vec4(0.71f, 0.56f, 0.40f, 1.0f); // Natural Oak (#B58D60)
            roomSettings.wallColor = glm::vec4(0.86f, 0.84f, 0.80f, 1.0f);  // Warm White (#DAD5CA)

            specs = {
                // Thảm nằm trên sàn, lệch nhẹ +3mm theo Y tránh z-fighting
                {"blender-carpet",       "Bedroom Carpet",   {-0.45f, 0.003f, -0.35f}, {0.0f,   0.0f, 0.0f}, {1.05f, 1.0f, 1.05f}},
                // Giường dựa tường Bắc (-Z)
                {"blender-modern-bed",   "Modern Bed",       {-0.70f, 0.000f, -1.78f}, {0.0f,   0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Tủ quần áo áp tường Tây (-X)
                {"blender-wardrobe",     "Wardrobe",         {-2.10f, 0.000f,  1.15f}, {0.0f,  90.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Bàn học áp tường Bắc phía Đông (mặt bàn cao 0.75m)
                {"procedural-desk",      "Study Desk",       { 1.48f, 0.000f, -2.50f}, {0.0f,   0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Đèn bàn đặt đúng mặt bàn y = 0.75m
                {"procedural-desk-lamp", "Study Desk Lamp",  { 1.92f, 0.750f, -2.58f}, {0.0f, -25.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Ghế gaming quay mặt vào bàn học
                {"blender-gaming-chair", "Study Chair",      { 1.48f, 0.000f, -1.68f}, {0.0f, 180.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Cây cảnh trang trí góc Đông-Nam (cách tường > 14 cm, không vượt biên)
                {"blender-indoor-plant", "Corner Plant",     { 1.72f, 0.000f,  1.92f}, {0.0f, -25.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}
            };
            break;
        }

        case ScenePresetType::GamingWorkspace: {
            // Không gian học tập & làm việc cá nhân hiện đại: 4.0 x 5.0 x 3.0 m
            // Phạm vi bên trong: X in [-2.0, 2.0], Z in [-2.5, 2.5], Y in [0, 3.0]
            // RC2.3: sửa toàn bộ lỗi va chạm (Desk Lamp vs Computer Setup) và vượt biên tường Tây (Indoor Plant)
            roomSettings.width = 4.0f;
            roomSettings.length = 5.0f;
            roomSettings.height = 3.0f;
            roomSettings.floorColor = glm::vec4(0.57f, 0.48f, 0.39f, 1.0f); // Smoked Oak
            roomSettings.wallColor = glm::vec4(0.81f, 0.82f, 0.84f, 1.0f);  // Cool Studio Grey-White

            specs = {
                {"blender-carpet",         "Workspace Carpet", { 0.20f, 0.003f,  0.45f}, {0.0f,   0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Bàn làm việc áp tường Bắc (-Z), mặt bàn cao 0.75m
                {"procedural-desk",        "Main Gaming Desk", { 0.22f, 0.000f, -2.02f}, {0.0f,   0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Bộ máy tính đặt cân bên trái-giữa mặt bàn y = 0.75m (x in [-0.45, 0.45])
                {"blender-computer-setup", "Computer Setup",   { 0.00f, 0.750f, -2.06f}, {0.0f,   0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Đèn bàn đặt góc phải mặt bàn y = 0.75m (x in [0.61, 0.88], cách Computer Setup > 15 cm)
                {"procedural-desk-lamp",   "Desk Lamp",        { 0.74f, 0.750f, -2.14f}, {0.0f, -35.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Ghế gaming quay vào bàn
                {"blender-gaming-chair",   "Gaming Chair",     { 0.20f, 0.000f, -1.08f}, {0.0f, 180.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Kệ sách áp tường Tây (-X)
                {"procedural-bookshelf",   "Bookshelf",        {-1.76f, 0.000f,  0.35f}, {0.0f,  90.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
                // Cây cảnh góc Tây-Bắc (đồng bộ kích thước OBJ 0.89x1.08x0.87m -> cách tường & bàn > 12 cm)
                {"blender-indoor-plant",   "Indoor Plant",     {-1.34f, 0.000f, -1.82f}, {0.0f,  15.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}
            };
            break;
        }

        case ScenePresetType::MinimalistLivingRoom: {
            // Phòng khách tối giản Scandinavian / Muji: 6.0 x 7.0 x 3.0 m
            // Phạm vi bên trong: X in [-3.0, 3.0], Z in [-3.5, 3.5], Y in [0, 3.0]
            roomSettings.width = 6.0f;
            roomSettings.length = 7.0f;
            roomSettings.height = 3.0f;
            roomSettings.floorColor = glm::vec4(0.75f, 0.62f, 0.46f, 1.0f); // Warm Light Oak
            roomSettings.wallColor = glm::vec4(0.87f, 0.85f, 0.81f, 1.0f);  // Warm Alabaster (#DAD5CA+)

            specs = {
                {"blender-carpet",       "Living Room Carpet", { 0.10f, 0.003f, -0.15f}, {0.0f,   0.0f, 0.0f}, {1.45f, 1.00f, 1.45f}},
                // Sofa dựa tường Bắc
                {"blender-modern-sofa",  "Modern Sofa",        { 0.15f, 0.000f, -2.55f}, {0.0f,   0.0f, 0.0f}, {1.05f, 1.00f, 1.00f}},
                // Bàn trà trung tâm trên thảm, cao 0.45m (0.75 * 0.60 = 0.45)
                {"procedural-desk",      "Coffee Table",       { 0.15f, 0.000f, -0.70f}, {0.0f,   0.0f, 0.0f}, {0.90f, 0.60f, 0.82f}},
                // Bình gốm đặt trên mặt bàn trà y = 0.45m
                {"procedural-vase",      "Ceramic Vase",       { 0.30f, 0.450f, -0.72f}, {0.0f,   0.0f, 0.0f}, {1.10f, 1.10f, 1.10f}},
                // Đèn đứng cạnh phải sofa
                {"blender-floor-lamp",   "Floor Lamp",         { 1.75f, 0.000f, -2.52f}, {0.0f, -20.0f, 0.0f}, {1.00f, 1.00f, 1.00f}},
                // Kệ sách áp tường Tây (-X) cân giữa phòng
                {"procedural-bookshelf", "Living Bookshelf",   {-2.76f, 0.000f,  0.15f}, {0.0f,  90.0f, 0.0f}, {1.15f, 1.00f, 1.00f}},
                // Cây cảnh góc Tây-Bắc tạo điểm xanh mềm mại
                {"blender-indoor-plant", "Living Room Plant",  {-2.15f, 0.000f, -2.55f}, {0.0f,  18.0f, 0.0f}, {1.05f, 1.05f, 1.05f}}
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
