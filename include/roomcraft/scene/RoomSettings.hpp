#pragma once

#include <glm/vec4.hpp>

#include <cmath>
#include <string>

namespace roomcraft::scene {

// Chế độ hiển thị tường và trần của căn phòng
enum class WallVisibilityMode {
    AllWalls = 0,     // Hiện đủ 4 tường và trần (nếu bật)
    HideCeiling = 1,  // Hiện đủ 4 tường, ẩn trần
    CutawayView = 2,  // Góc nhìn dollhouse: ẩn 2 tường phía trước (+Z South và +X East) và ẩn trần
    Manual = 3        // Người dùng tự bật/tắt từng bức tường và trần độc lập
};

inline const char* wallVisibilityModeToString(WallVisibilityMode mode)
{
    switch (mode) {
        case WallVisibilityMode::AllWalls:    return "All Walls";
        case WallVisibilityMode::HideCeiling: return "Hide Ceiling";
        case WallVisibilityMode::CutawayView: return "Cutaway View";
        case WallVisibilityMode::Manual:      return "Manual";
        default:                              return "Unknown";
    }
}

// Giới hạn hợp lệ cho kích thước phòng (đơn vị: mét)
namespace room_limits {
inline constexpr float kMinWidth = 2.0f;
inline constexpr float kMaxWidth = 20.0f;
inline constexpr float kMinLength = 2.0f;
inline constexpr float kMaxLength = 20.0f;
inline constexpr float kMinHeight = 2.0f;
inline constexpr float kMaxHeight = 6.0f;
inline constexpr float kMinWallThickness = 0.05f;
inline constexpr float kMaxWallThickness = 0.50f;
inline constexpr float kMinFloorThickness = 0.02f;
inline constexpr float kMaxFloorThickness = 0.50f;
inline constexpr float kMinGridSpacing = 0.10f;
inline constexpr float kMaxGridSpacing = 2.00f;
} // namespace room_limits

// Cấu hình tham số cho căn phòng (RoomSettings).
// Quy ước hệ tọa độ (Right-handed, Y-up, mét):
// - Gốc tọa độ (0, 0, 0) đặt tại TÂM MẶT SÀN.
// - width (trục X) và length (trục Z) là kích thước SỬ DỤNG BÊN TRONG:
//     X thuộc [-width/2, +width/2], Z thuộc [-length/2, +length/2], Y thuộc [0, height].
// - Tường được dựng ra phía NGOÀI giới hạn sử dụng bên trong; độ dày sàn kéo xuống phía âm Y.
struct RoomSettings {
    float width = 5.0f;
    float length = 6.0f;
    float height = 3.0f;
    float wallThickness = 0.15f;
    float floorThickness = 0.10f;
    float gridSpacing = 0.50f;

    glm::vec4 floorColor{0.76f, 0.67f, 0.55f, 1.0f};   // Gỗ sồi sáng
    glm::vec4 wallColor{0.89f, 0.88f, 0.85f, 1.0f};    // Tường trắng kem dịu
    glm::vec4 ceilingColor{0.93f, 0.93f, 0.91f, 1.0f}; // Trần sáng nhẹ

    WallVisibilityMode wallVisibilityMode = WallVisibilityMode::CutawayView;
    bool showNorthWall = true;  // Tường -Z
    bool showSouthWall = true;  // Tường +Z
    bool showEastWall = true;   // Tường +X
    bool showWestWall = true;   // Tường -X
    bool showCeiling = false;   // Trần Y = height
    bool showGrid = true;
    bool showWindow = true;     // Cửa sổ kiến trúc trang trí trên tường Bắc (RC2.5)

    float getFloorArea() const { return width * length; }
    float getVolume() const { return width * length * height; }

    // Kiểm tra trạng thái hiển thị thực tế của từng bức tường theo WallVisibilityMode
    // (việc ẩn tường chỉ ảnh hưởng render, KHÔNG xóa dữ liệu tường hay đổi interior bounds)
    bool isNorthWallVisible() const
    {
        if (wallVisibilityMode == WallVisibilityMode::Manual) return showNorthWall;
        return true;
    }

    bool isWestWallVisible() const
    {
        if (wallVisibilityMode == WallVisibilityMode::Manual) return showWestWall;
        return true;
    }

    bool isSouthWallVisible() const
    {
        if (wallVisibilityMode == WallVisibilityMode::CutawayView) return false;
        if (wallVisibilityMode == WallVisibilityMode::Manual) return showSouthWall;
        return true;
    }

    bool isEastWallVisible() const
    {
        if (wallVisibilityMode == WallVisibilityMode::CutawayView) return false;
        if (wallVisibilityMode == WallVisibilityMode::Manual) return showEastWall;
        return true;
    }

    bool isCeilingVisible() const
    {
        if (wallVisibilityMode == WallVisibilityMode::AllWalls) return true;
        if (wallVisibilityMode == WallVisibilityMode::HideCeiling ||
            wallVisibilityMode == WallVisibilityMode::CutawayView) {
            return false;
        }
        return showCeiling;
    }

    // So sánh phần tham số ảnh hưởng tới hình học lưới (nếu chỉ đổi màu/visibility thì KHÔNG cần rebuild mesh)
    bool geometryEquals(const RoomSettings& other) const
    {
        constexpr float kEps = 1e-5f;
        return std::abs(width - other.width) <= kEps &&
               std::abs(length - other.length) <= kEps &&
               std::abs(height - other.height) <= kEps &&
               std::abs(wallThickness - other.wallThickness) <= kEps &&
               std::abs(floorThickness - other.floorThickness) <= kEps &&
               std::abs(gridSpacing - other.gridSpacing) <= kEps;
    }

    bool validate(std::string* outError = nullptr) const
    {
        auto checkRange = [&](float val, float minV, float maxV, const char* label) -> bool {
            if (!std::isfinite(val)) {
                if (outError != nullptr) *outError = std::string(label) + " must be finite.";
                return false;
            }
            if (val < minV || val > maxV) {
                if (outError != nullptr) {
                    *outError = std::string(label) + " out of range [" +
                                std::to_string(minV) + ", " + std::to_string(maxV) + "].";
                }
                return false;
            }
            return true;
        };

        auto checkColor = [&](const glm::vec4& c, const char* label) -> bool {
            if (!std::isfinite(c.r) || !std::isfinite(c.g) || !std::isfinite(c.b) || !std::isfinite(c.a)) {
                if (outError != nullptr) *outError = std::string(label) + " contains non-finite values.";
                return false;
            }
            return true;
        };

        return checkRange(width, room_limits::kMinWidth, room_limits::kMaxWidth, "Room width") &&
               checkRange(length, room_limits::kMinLength, room_limits::kMaxLength, "Room length") &&
               checkRange(height, room_limits::kMinHeight, room_limits::kMaxHeight, "Room height") &&
               checkRange(wallThickness, room_limits::kMinWallThickness, room_limits::kMaxWallThickness, "Wall thickness") &&
               checkRange(floorThickness, room_limits::kMinFloorThickness, room_limits::kMaxFloorThickness, "Floor thickness") &&
               checkRange(gridSpacing, room_limits::kMinGridSpacing, room_limits::kMaxGridSpacing, "Grid spacing") &&
               checkColor(floorColor, "Floor color") &&
               checkColor(wallColor, "Wall color") &&
               checkColor(ceilingColor, "Ceiling color");
    }
};

} // namespace roomcraft::scene
