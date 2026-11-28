#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace roomcraft::persistence {

// Kiểu dữ liệu JSON thuần C++17 tuân thủ RFC 8259 (không cần tải thư viện ngoài).
// Hỗ trợ bảo vệ giới hạn kích thước file, độ sâu lồng nhau tối đa, phân biệt nguyên dương 64-bit
// và từ chối NaN/Infinity hoặc ký tự thừa sau tài liệu JSON.
class JsonValue {
public:
    enum class Type {
        Null = 0,
        Bool,
        Int64,
        UInt64,
        Double,
        String,
        Array,
        Object
    };

    using Array = std::vector<JsonValue>;
    using ObjectMember = std::pair<std::string, JsonValue>;
    using Object = std::vector<ObjectMember>; // Giữ nguyên thứ tự khóa để đầu ra JSON nhất quán

    JsonValue() = default;
    JsonValue(std::nullptr_t) : m_type(Type::Null) {}
    JsonValue(bool value) : m_type(Type::Bool), m_bool(value) {}
    JsonValue(int value) : m_type(Type::Int64), m_int64(static_cast<std::int64_t>(value)) {}
    JsonValue(std::int64_t value) : m_type(Type::Int64), m_int64(value) {}
    JsonValue(std::uint64_t value) : m_type(Type::UInt64), m_uint64(value) {}
    JsonValue(float value) : m_type(Type::Double), m_double(static_cast<double>(value)) {}
    JsonValue(double value) : m_type(Type::Double), m_double(value) {}
    JsonValue(const char* value) : m_type(Type::String), m_string(value != nullptr ? value : "") {}
    JsonValue(std::string value) : m_type(Type::String), m_string(std::move(value)) {}
    JsonValue(Array value) : m_type(Type::Array), m_array(std::move(value)) {}
    JsonValue(Object value) : m_type(Type::Object), m_object(std::move(value)) {}

    static JsonValue makeArray() { return JsonValue(Array{}); }
    static JsonValue makeObject() { return JsonValue(Object{}); }

    Type type() const { return m_type; }
    bool isNull() const { return m_type == Type::Null; }
    bool isBool() const { return m_type == Type::Bool; }
    bool isInt64() const { return m_type == Type::Int64; }
    bool isUInt64() const { return m_type == Type::UInt64; }
    bool isNumber() const { return m_type == Type::Int64 || m_type == Type::UInt64 || m_type == Type::Double; }
    bool isString() const { return m_type == Type::String; }
    bool isArray() const { return m_type == Type::Array; }
    bool isObject() const { return m_type == Type::Object; }

    bool asBool() const { return m_bool; }
    std::int64_t asInt64() const { return m_int64; }
    std::uint64_t asUInt64() const { return m_uint64; }
    double asDouble() const;
    const std::string& asString() const { return m_string; }
    const Array& asArray() const { return m_array; }
    Array& asArray() { return m_array; }
    const Object& asObject() const { return m_object; }
    Object& asObject() { return m_object; }

    void pushBack(JsonValue value);
    void set(const std::string& key, JsonValue value);
    const JsonValue* find(const std::string& key) const;
    bool contains(const std::string& key) const { return find(key) != nullptr; }

    std::string dump(int indentSpaces = 2) const;

private:
    Type m_type = Type::Null;
    bool m_bool = false;
    std::int64_t m_int64 = 0;
    std::uint64_t m_uint64 = 0;
    double m_double = 0.0;
    std::string m_string;
    Array m_array;
    Object m_object;
};

struct JsonParseLimits {
    static constexpr std::size_t kMaxInputBytes = 16u * 1024u * 1024u; // 16 MB
    static constexpr int kMaxNestingDepth = 64;
    static constexpr std::size_t kMaxTotalNodes = 250000u;
};

// Phân tích chuỗi JSON chặt chẽ theo RFC 8259 (từ chối UTF-8 lỗi, surrogate lẻ, khóa trùng, NaN/Inf, rác cuối)
bool parseJson(const std::string& input,
               JsonValue& outValue,
               std::string* outError = nullptr,
               std::size_t maxBytes = JsonParseLimits::kMaxInputBytes,
               int maxDepth = JsonParseLimits::kMaxNestingDepth,
               std::size_t maxNodes = JsonParseLimits::kMaxTotalNodes);

} // namespace roomcraft::persistence
