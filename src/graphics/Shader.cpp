#include "roomcraft/graphics/Shader.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

namespace roomcraft::graphics {

Shader::~Shader()
{
    destroy();
}

Shader::Shader(Shader&& other) noexcept
    : m_programId(std::exchange(other.m_programId, 0))
    , m_uniformCache(std::move(other.m_uniformCache))
{
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_programId = std::exchange(other.m_programId, 0);
        m_uniformCache = std::move(other.m_uniformCache);
    }
    return *this;
}

bool Shader::loadFromFiles(const std::filesystem::path& vertexPath,
                           const std::filesystem::path& fragmentPath,
                           std::string* outError)
{
    std::string vertexSource;
    std::string fragmentSource;

    if (!readTextFile(vertexPath, vertexSource, outError)) {
        return false;
    }
    if (!readTextFile(fragmentPath, fragmentSource, outError)) {
        return false;
    }

    const std::string label = vertexPath.filename().string() + " + " + fragmentPath.filename().string();
    return loadFromSource(vertexSource, fragmentSource, label, outError);
}

bool Shader::loadFromSource(const std::string& vertexSource,
                            const std::string& fragmentSource,
                            const std::string& debugLabel,
                            std::string* outError)
{
    const unsigned int vertexShader = compileStage(
        GL_VERTEX_SHADER, vertexSource, debugLabel + " [VERTEX]", outError);
    if (vertexShader == 0) {
        return false;
    }

    const unsigned int fragmentShader = compileStage(
        GL_FRAGMENT_SHADER, fragmentSource, debugLabel + " [FRAGMENT]", outError);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    const unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    // Sau bước link, đối tượng shader trung gian luôn có thể giải phóng ngay
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (linked != GL_TRUE) {
        GLint logLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
        std::string infoLog(static_cast<size_t>(logLen > 0 ? logLen : 1), '\0');
        glGetProgramInfoLog(program, logLen, nullptr, infoLog.data());

        const std::string message = "Shader program link failed (" + debugLabel + "): " + infoLog;
        if (outError != nullptr) {
            *outError = message;
        }
        std::cerr << "[RoomCraft3D][ERROR] " << message << std::endl;

        glDeleteProgram(program);
        return false;
    }

    // Chỉ thay thế chương trình cũ sau khi biên dịch và liên kết thành công hoàn toàn
    destroy();
    m_programId = program;
    m_uniformCache.clear();
    return true;
}

void Shader::destroy()
{
    if (m_programId != 0) {
        glDeleteProgram(m_programId);
        m_programId = 0;
    }
    m_uniformCache.clear();
}

void Shader::bind() const
{
    glUseProgram(m_programId);
}

void Shader::unbind()
{
    glUseProgram(0);
}

void Shader::setInt(const std::string& name, int value)
{
    const int location = getUniformLocation(name);
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void Shader::setFloat(const std::string& name, float value)
{
    const int location = getUniformLocation(name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void Shader::setVec3(const std::string& name, const glm::vec3& value)
{
    const int location = getUniformLocation(name);
    if (location != -1) {
        glUniform3fv(location, 1, glm::value_ptr(value));
    }
}

void Shader::setVec4(const std::string& name, const glm::vec4& value)
{
    const int location = getUniformLocation(name);
    if (location != -1) {
        glUniform4fv(location, 1, glm::value_ptr(value));
    }
}

void Shader::setMat4(const std::string& name, const glm::mat4& value)
{
    const int location = getUniformLocation(name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }
}

bool Shader::readTextFile(const std::filesystem::path& path,
                          std::string& outContent,
                          std::string* outError)
{
    std::ifstream file(path, std::ios::in);
    if (!file.is_open()) {
        const std::string message = "Failed to open shader file: " + path.string();
        if (outError != nullptr) {
            *outError = message;
        }
        std::cerr << "[RoomCraft3D][ERROR] " << message << std::endl;
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    outContent = buffer.str();
    return true;
}

unsigned int Shader::compileStage(unsigned int shaderType,
                                  const std::string& source,
                                  const std::string& stageLabel,
                                  std::string* outError)
{
    const unsigned int shader = glCreateShader(shaderType);
    const char* srcPtr = source.c_str();
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string infoLog(static_cast<size_t>(logLen > 0 ? logLen : 1), '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, infoLog.data());

        const std::string message = "Shader compilation failed (" + stageLabel + "): " + infoLog;
        if (outError != nullptr) {
            *outError = message;
        }
        std::cerr << "[RoomCraft3D][ERROR] " << message << std::endl;

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

int Shader::getUniformLocation(const std::string& name)
{
    if (m_programId == 0) {
        return -1;
    }

    const auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end()) {
        return it->second;
    }

    const int location = glGetUniformLocation(m_programId, name.c_str());
    m_uniformCache.emplace(name, location);
    return location;
}

} // namespace roomcraft::graphics
