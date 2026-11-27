#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace roomcraft::graphics {

// Lớp Texture bao gói OpenGL 2D Texture theo mô hình RAII (non-copyable, move-only).
// Hỗ trợ nạp PNG/JPEG qua stb_image với stbi_set_flip_vertically_on_load(1) để khớp gốc UV dưới-trái của OpenGL.
class Texture {
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Nạp ảnh từ file (PNG/JPG/BMP) bằng stb_image và tạo mipmap trên GPU
    bool loadFromFile(const std::filesystem::path& filePath, std::string* outError = nullptr);

    // Tạo texture trực tiếp từ bộ nhớ CPU (dùng cho fallback texture, 1x1 white và test pattern)
    bool createFromMemory(int width,
                          int height,
                          int channels,
                          const std::uint8_t* pixels,
                          bool generateMipmaps = true,
                          std::string* outError = nullptr);

    // Tạo texture dự phòng dạng bàn cờ khi file ảnh bị thiếu hoặc hỏng
    bool createFallbackChecker(int size = 64);

    // Tạo texture trắng 1x1 (nhân với baseColor không làm đổi màu)
    bool createSolidColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255);

    void bind(unsigned int textureUnit = 0) const;
    static void unbind(unsigned int textureUnit = 0);

    void destroy();

    bool isValid() const { return m_textureId != 0; }
    bool isFallback() const { return m_isFallback; }
    unsigned int getId() const { return m_textureId; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getChannels() const { return m_channels; }
    const std::string& getSourcePath() const { return m_sourcePath; }

private:
    unsigned int m_textureId = 0;
    int m_width = 0;
    int m_height = 0;
    int m_channels = 0;
    bool m_isFallback = false;
    std::string m_sourcePath;
};

} // namespace roomcraft::graphics
