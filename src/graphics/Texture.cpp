#include "roomcraft/graphics/Texture.hpp"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

namespace roomcraft::graphics {

namespace {
constexpr int kMaxTextureDimension = 8192;
} // namespace

Texture::~Texture()
{
    destroy();
}

Texture::Texture(Texture&& other) noexcept
    : m_textureId(std::exchange(other.m_textureId, 0))
    , m_width(std::exchange(other.m_width, 0))
    , m_height(std::exchange(other.m_height, 0))
    , m_channels(std::exchange(other.m_channels, 0))
    , m_isFallback(std::exchange(other.m_isFallback, false))
    , m_sourcePath(std::move(other.m_sourcePath))
{
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_textureId = std::exchange(other.m_textureId, 0);
        m_width = std::exchange(other.m_width, 0);
        m_height = std::exchange(other.m_height, 0);
        m_channels = std::exchange(other.m_channels, 0);
        m_isFallback = std::exchange(other.m_isFallback, false);
        m_sourcePath = std::move(other.m_sourcePath);
    }
    return *this;
}

bool Texture::loadFromFile(const std::filesystem::path& filePath, std::string* outError)
{
    const auto pathU8 = filePath.u8string();
    const std::string displayPath(pathU8.begin(), pathU8.end());

    std::error_code ec;
    if (filePath.empty() || !std::filesystem::exists(filePath, ec)) {
        if (outError != nullptr) {
            *outError = "Texture file does not exist: " + displayPath;
        }
        return false;
    }

    // Đọc byte bằng std::ifstream(std::filesystem::path) để giải mã ổn định trên đường dẫn Unicode (Windows)
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        if (outError != nullptr) {
            *outError = "Unable to open texture file: " + displayPath;
        }
        return false;
    }
    const std::streamsize fileSize = file.tellg();
    if (fileSize <= 0 || fileSize > 64 * 1024 * 1024) {
        if (outError != nullptr) {
            *outError = "Invalid texture file size: " + displayPath;
        }
        return false;
    }
    std::vector<unsigned char> encodedBytes(static_cast<std::size_t>(fileSize));
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(encodedBytes.data()), fileSize)) {
        if (outError != nullptr) {
            *outError = "Failed to read texture bytes: " + displayPath;
        }
        return false;
    }

    // Lật ảnh theo trục dọc để hàng dưới cùng của ảnh tương ứng v = 0 của OpenGL
    // (nhất quán với việc KHÔNG dùng aiProcess_FlipUVs trong ModelLoader)
    stbi_set_flip_vertically_on_load(1);

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load_from_memory(
        encodedBytes.data(), static_cast<int>(encodedBytes.size()), &width, &height, &channels, 0);
    if (data == nullptr) {
        if (outError != nullptr) {
            *outError = std::string("stb_image failed to decode '") + displayPath +
                        "': " + (stbi_failure_reason() ? stbi_failure_reason() : "unknown error");
        }
        return false;
    }

    const bool ok = createFromMemory(width, height, channels, data, true, outError);
    stbi_image_free(data);

    if (ok) {
        m_isFallback = false;
        m_sourcePath = displayPath;
    }
    return ok;
}

bool Texture::createFromMemory(int width,
                               int height,
                               int channels,
                               const std::uint8_t* pixels,
                               bool generateMipmaps,
                               std::string* outError)
{
    if (pixels == nullptr || width <= 0 || height <= 0 ||
        width > kMaxTextureDimension || height > kMaxTextureDimension) {
        if (outError != nullptr) {
            *outError = "Invalid texture dimensions or null pixel buffer.";
        }
        return false;
    }

    GLenum internalFormat = GL_SRGB8_ALPHA8;
    GLenum dataFormat = GL_RGBA;
    std::vector<std::uint8_t> convertedRgb;
    const std::uint8_t* uploadPtr = pixels;

    if (channels == 1) {
        // Mở rộng ảnh xám 1 kênh sang RGB để shader diffuse đọc đồng nhất
        convertedRgb.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3);
        for (std::size_t i = 0; i < static_cast<std::size_t>(width) * static_cast<std::size_t>(height); ++i) {
            convertedRgb[i * 3 + 0] = pixels[i];
            convertedRgb[i * 3 + 1] = pixels[i];
            convertedRgb[i * 3 + 2] = pixels[i];
        }
        internalFormat = GL_SRGB8;
        dataFormat = GL_RGB;
        uploadPtr = convertedRgb.data();
    } else if (channels == 3) {
        internalFormat = GL_SRGB8;
        dataFormat = GL_RGB;
    } else if (channels == 4) {
        internalFormat = GL_SRGB8_ALPHA8;
        dataFormat = GL_RGBA;
    } else {
        if (outError != nullptr) {
            *outError = "Unsupported texture channel count: " + std::to_string(channels);
        }
        return false;
    }

    destroy();

    glGenTextures(1, &m_textureId);
    glBindTexture(GL_TEXTURE_2D, m_textureId);

    // Đặt GL_UNPACK_ALIGNMENT = 1 để tránh lệch hàng hoặc crash với ảnh RGB có bề rộng không chia hết cho 4
    GLint prevAlignment = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat),
                 width, height, 0, dataFormat, GL_UNSIGNED_BYTE, uploadPtr);

    glPixelStorei(GL_UNPACK_ALIGNMENT, prevAlignment);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (generateMipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        // Bật Anisotropic Filtering an toàn theo chuẩn OpenGL 3.3 Core nếu phần cứng hỗ trợ (RC2.4)
        static const float kAnisoLevel = []() -> float {
            GLint extCount = 0;
            glGetIntegerv(GL_NUM_EXTENSIONS, &extCount);
            bool supported = false;
            for (GLint i = 0; i < extCount; ++i) {
                const auto* ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
                if (ext != nullptr &&
                    (std::string(ext) == "GL_EXT_texture_filter_anisotropic" ||
                     std::string(ext) == "GL_ARB_texture_filter_anisotropic")) {
                    supported = true;
                    break;
                }
            }
            if (!supported) return 1.0f;
            GLfloat maxAniso = 1.0f;
            constexpr GLenum kMaxTextureMaxAnisotropyExt = 0x84FF;
            glGetFloatv(kMaxTextureMaxAnisotropyExt, &maxAniso);
            return std::clamp(static_cast<float>(maxAniso), 1.0f, 8.0f);
        }();
        if (kAnisoLevel > 1.0f) {
            constexpr GLenum kTextureMaxAnisotropyExt = 0x84FE;
            glTexParameterf(GL_TEXTURE_2D, kTextureMaxAnisotropyExt, kAnisoLevel);
        }
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    m_width = width;
    m_height = height;
    m_channels = channels;
    return true;
}

bool Texture::createFallbackChecker(int size)
{
    const int dim = (size >= 8 && size <= 512) ? size : 64;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(dim * dim * 4), 255);
    const int cell = dim / 8;

    for (int y = 0; y < dim; ++y) {
        for (int x = 0; x < dim; ++x) {
            const bool dark = (((x / cell) + (y / cell)) % 2) == 0;
            const std::size_t idx = static_cast<std::size_t>((y * dim + x) * 4);
            // Mẫu bàn cờ hồng-xám dễ nhận biết khi thiếu texture nhưng không gây chói
            pixels[idx + 0] = dark ? 95 : 210;
            pixels[idx + 1] = dark ? 70 : 120;
            pixels[idx + 2] = dark ? 105 : 195;
            pixels[idx + 3] = 255;
        }
    }

    if (!createFromMemory(dim, dim, 4, pixels.data(), true)) {
        return false;
    }
    m_isFallback = true;
    m_sourcePath = "<fallback-checker>";
    return true;
}

bool Texture::createSolidColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
{
    const std::uint8_t pixel[4] = {r, g, b, a};
    if (!createFromMemory(1, 1, 4, pixel, false)) {
        return false;
    }
    m_isFallback = false;
    m_sourcePath = "<solid-1x1>";
    return true;
}

void Texture::bind(unsigned int textureUnit) const
{
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, m_textureId);
}

void Texture::unbind(unsigned int textureUnit)
{
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::destroy()
{
    if (m_textureId != 0) {
        glDeleteTextures(1, &m_textureId);
        m_textureId = 0;
    }
    m_width = 0;
    m_height = 0;
    m_channels = 0;
    m_isFallback = false;
    m_sourcePath.clear();
}

} // namespace roomcraft::graphics
