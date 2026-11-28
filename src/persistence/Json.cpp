#include "roomcraft/persistence/Json.hpp"

#include <charconv>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <limits>
#include <sstream>

namespace roomcraft::persistence {

double JsonValue::asDouble() const
{
    if (m_type == Type::Double) return m_double;
    if (m_type == Type::Int64) return static_cast<double>(m_int64);
    if (m_type == Type::UInt64) return static_cast<double>(m_uint64);
    return 0.0;
}

void JsonValue::pushBack(JsonValue value)
{
    if (m_type != Type::Array) {
        m_type = Type::Array;
        m_array.clear();
    }
    m_array.push_back(std::move(value));
}

void JsonValue::set(const std::string& key, JsonValue value)
{
    if (m_type != Type::Object) {
        m_type = Type::Object;
        m_object.clear();
    }
    for (auto& kv : m_object) {
        if (kv.first == key) {
            kv.second = std::move(value);
            return;
        }
    }
    m_object.emplace_back(key, std::move(value));
}

const JsonValue* JsonValue::find(const std::string& key) const
{
    if (m_type != Type::Object) {
        return nullptr;
    }
    for (const auto& kv : m_object) {
        if (kv.first == key) {
            return &kv.second;
        }
    }
    return nullptr;
}

namespace {

void writeEscapedString(std::ostringstream& oss, const std::string& s)
{
    oss << '"';
    for (unsigned char ch : s) {
        switch (ch) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b";  break;
            case '\f': oss << "\\f";  break;
            case '\n': oss << "\\n";  break;
            case '\r': oss << "\\r";  break;
            case '\t': oss << "\\t";  break;
            default:
                if (ch < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned int>(ch));
                    oss << buf;
                } else {
                    oss << static_cast<char>(ch);
                }
                break;
        }
    }
    oss << '"';
}

bool isCompactNumberArray(const JsonValue::Array& arr)
{
    if (arr.empty() || arr.size() > 4) {
        return false;
    }
    for (const auto& el : arr) {
        if (!el.isNumber()) {
            return false;
        }
    }
    return true;
}

void dumpRecursive(const JsonValue& v, std::ostringstream& oss, int indentSpaces, int depth)
{
    auto writeIndent = [&](int level) {
        if (indentSpaces > 0) {
            for (int i = 0; i < level * indentSpaces; ++i) {
                oss << ' ';
            }
        }
    };

    switch (v.type()) {
        case JsonValue::Type::Null:
            oss << "null";
            break;
        case JsonValue::Type::Bool:
            oss << (v.asBool() ? "true" : "false");
            break;
        case JsonValue::Type::Int64:
            oss << v.asInt64();
            break;
        case JsonValue::Type::UInt64:
            oss << v.asUInt64();
            break;
        case JsonValue::Type::Double: {
            const double d = v.asDouble();
            if (!std::isfinite(d)) {
                oss << "null";
            } else {
                const double clean = (std::fabs(d) < 1e-12) ? 0.0 : d;
                std::ostringstream num;
                num << std::setprecision(9) << clean;
                std::string str = num.str();
                if (str.find('.') == std::string::npos && str.find('e') == std::string::npos &&
                    str.find('E') == std::string::npos) {
                    str += ".0";
                }
                oss << str;
            }
            break;
        }
        case JsonValue::Type::String:
            writeEscapedString(oss, v.asString());
            break;
        case JsonValue::Type::Array: {
            const auto& arr = v.asArray();
            if (arr.empty()) {
                oss << "[]";
                break;
            }
            if (indentSpaces <= 0 || isCompactNumberArray(arr)) {
                oss << '[';
                for (std::size_t i = 0; i < arr.size(); ++i) {
                    if (i > 0) oss << (indentSpaces > 0 ? ", " : ",");
                    dumpRecursive(arr[i], oss, indentSpaces, depth + 1);
                }
                oss << ']';
                break;
            }
            oss << "[\n";
            for (std::size_t i = 0; i < arr.size(); ++i) {
                writeIndent(depth + 1);
                dumpRecursive(arr[i], oss, indentSpaces, depth + 1);
                if (i + 1 < arr.size()) oss << ',';
                oss << '\n';
            }
            writeIndent(depth);
            oss << ']';
            break;
        }
        case JsonValue::Type::Object: {
            const auto& obj = v.asObject();
            if (obj.empty()) {
                oss << "{}";
                break;
            }
            oss << '{';
            if (indentSpaces > 0) oss << '\n';
            for (std::size_t i = 0; i < obj.size(); ++i) {
                if (indentSpaces > 0) writeIndent(depth + 1);
                writeEscapedString(oss, obj[i].first);
                oss << (indentSpaces > 0 ? ": " : ":");
                dumpRecursive(obj[i].second, oss, indentSpaces, depth + 1);
                if (i + 1 < obj.size()) oss << ',';
                if (indentSpaces > 0) oss << '\n';
            }
            if (indentSpaces > 0) writeIndent(depth);
            oss << '}';
            break;
        }
    }
}

void appendUtf8Codepoint(std::string& out, std::uint32_t cp)
{
    if (cp <= 0x7Fu) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FFu) {
        out.push_back(static_cast<char>(0xC0u | ((cp >> 6) & 0x1Fu)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    } else if (cp <= 0xFFFFu) {
        out.push_back(static_cast<char>(0xE0u | ((cp >> 12) & 0x0Fu)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    } else {
        out.push_back(static_cast<char>(0xF0u | ((cp >> 18) & 0x07u)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    }
}

class StrictJsonParser {
public:
    StrictJsonParser(const std::string& text, int maxDepth)
        : m_text(text), m_maxDepth(maxDepth)
    {
    }

    bool parse(JsonValue& outValue, std::string& outError)
    {
        // Bỏ qua UTF-8 BOM nếu tệp được lưu bởi trình soạn thảo Windows
        if (m_text.size() >= 3 &&
            static_cast<unsigned char>(m_text[0]) == 0xEF &&
            static_cast<unsigned char>(m_text[1]) == 0xBB &&
            static_cast<unsigned char>(m_text[2]) == 0xBF) {
            m_pos = 3;
        }

        skipWhitespace();
        if (eof()) {
            return fail("Empty JSON document.");
        }
        if (!parseValue(outValue, 0)) {
            outError = m_error;
            return false;
        }
        skipWhitespace();
        if (!eof()) {
            return fail("Unexpected trailing characters after JSON document.", outError);
        }
        return true;
    }

private:
    bool eof() const { return m_pos >= m_text.size(); }
    char peek() const { return eof() ? '\0' : m_text[m_pos]; }

    char advance()
    {
        const char c = m_text[m_pos++];
        if (c == '\n') {
            ++m_line;
            m_col = 1;
        } else {
            ++m_col;
        }
        return c;
    }

    void skipWhitespace()
    {
        while (!eof()) {
            const char c = peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                advance();
            } else {
                break;
            }
        }
    }

    bool fail(const std::string& msg)
    {
        m_error = "JSON parse error at line " + std::to_string(m_line) +
                  ", col " + std::to_string(m_col) + ": " + msg;
        return false;
    }

    bool fail(const std::string& msg, std::string& outErr)
    {
        fail(msg);
        outErr = m_error;
        return false;
    }

    bool parseValue(JsonValue& out, int depth)
    {
        if (depth > m_maxDepth) {
            return fail("Maximum JSON nesting depth exceeded.");
        }
        skipWhitespace();
        if (eof()) {
            return fail("Unexpected end of JSON input.");
        }

        const char c = peek();
        if (c == '{') return parseObject(out, depth);
        if (c == '[') return parseArray(out, depth);
        if (c == '"') {
            std::string s;
            if (!parseString(s)) return false;
            out = JsonValue(std::move(s));
            return true;
        }
        if (c == 't') return parseKeyword("true", JsonValue(true), out);
        if (c == 'f') return parseKeyword("false", JsonValue(false), out);
        if (c == 'n') return parseKeyword("null", JsonValue(nullptr), out);
        if (c == '-' || (c >= '0' && c <= '9')) return parseNumber(out);

        return fail(std::string("Unexpected character '") + c + "'.");
    }

    bool parseKeyword(const char* kw, JsonValue val, JsonValue& out)
    {
        for (std::size_t i = 0; kw[i] != '\0'; ++i) {
            if (eof() || peek() != kw[i]) {
                return fail(std::string("Invalid token; expected '") + kw + "'.");
            }
            advance();
        }
        out = std::move(val);
        return true;
    }

    bool parseHex4(std::uint32_t& outHex)
    {
        outHex = 0;
        for (int i = 0; i < 4; ++i) {
            if (eof()) return fail("Incomplete \\uXXXX escape.");
            const char c = advance();
            outHex <<= 4;
            if (c >= '0' && c <= '9') outHex |= static_cast<std::uint32_t>(c - '0');
            else if (c >= 'a' && c <= 'f') outHex |= static_cast<std::uint32_t>(10 + (c - 'a'));
            else if (c >= 'A' && c <= 'F') outHex |= static_cast<std::uint32_t>(10 + (c - 'A'));
            else return fail("Invalid hex digit in \\uXXXX escape.");
        }
        return true;
    }

    bool parseString(std::string& out)
    {
        if (peek() != '"') return fail("Expected '\"'.");
        advance();
        out.clear();

        while (!eof()) {
            const unsigned char uch = static_cast<unsigned char>(advance());
            if (uch == '"') {
                return true;
            }
            if (uch < 0x20) {
                return fail("Unescaped control character inside string.");
            }
            if (uch == '\\') {
                if (eof()) return fail("Unterminated escape sequence.");
                const char esc = advance();
                switch (esc) {
                    case '"':  out.push_back('"');  break;
                    case '\\': out.push_back('\\'); break;
                    case '/':  out.push_back('/');  break;
                    case 'b':  out.push_back('\b'); break;
                    case 'f':  out.push_back('\f'); break;
                    case 'n':  out.push_back('\n'); break;
                    case 'r':  out.push_back('\r'); break;
                    case 't':  out.push_back('\t'); break;
                    case 'u': {
                        std::uint32_t cp = 0;
                        if (!parseHex4(cp)) return false;
                        if (cp >= 0xD800u && cp <= 0xDBFFu) {
                            if (eof() || advance() != '\\' || eof() || advance() != 'u') {
                                return fail("Expected low surrogate pair.");
                            }
                            std::uint32_t low = 0;
                            if (!parseHex4(low) || low < 0xDC00u || low > 0xDFFFu) {
                                return fail("Invalid low surrogate code unit.");
                            }
                            cp = 0x10000u + (((cp - 0xD800u) << 10) | (low - 0xDC00u));
                        }
                        appendUtf8Codepoint(out, cp);
                        break;
                    }
                    default:
                        return fail("Invalid escape character.");
                }
            } else {
                out.push_back(static_cast<char>(uch));
            }
        }
        return fail("Unterminated string literal.");
    }

    bool parseNumber(JsonValue& out)
    {
        const std::size_t start = m_pos;
        bool isNegative = false;
        bool isFloat = false;

        if (peek() == '-') {
            isNegative = true;
            advance();
        }
        if (eof()) return fail("Incomplete number.");

        if (peek() == '0') {
            advance();
            if (!eof() && peek() >= '0' && peek() <= '9') {
                return fail("Leading zeros are not allowed in JSON numbers.");
            }
        } else if (peek() >= '1' && peek() <= '9') {
            while (!eof() && peek() >= '0' && peek() <= '9') {
                advance();
            }
        } else {
            return fail("Invalid number syntax.");
        }

        if (!eof() && peek() == '.') {
            isFloat = true;
            advance();
            if (eof() || peek() < '0' || peek() > '9') {
                return fail("Expected digit after decimal point.");
            }
            while (!eof() && peek() >= '0' && peek() <= '9') {
                advance();
            }
        }

        if (!eof() && (peek() == 'e' || peek() == 'E')) {
            isFloat = true;
            advance();
            if (!eof() && (peek() == '+' || peek() == '-')) {
                advance();
            }
            if (eof() || peek() < '0' || peek() > '9') {
                return fail("Expected digit in exponent.");
            }
            while (!eof() && peek() >= '0' && peek() <= '9') {
                advance();
            }
        }

        const char* first = m_text.data() + start;
        const char* last = m_text.data() + m_pos;

        if (!isFloat) {
            if (!isNegative) {
                std::uint64_t uval = 0;
                const auto res = std::from_chars(first, last, uval);
                if (res.ec == std::errc() && res.ptr == last) {
                    if (uval <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                        out = JsonValue(static_cast<std::int64_t>(uval));
                    } else {
                        out = JsonValue(uval);
                    }
                    return true;
                }
            } else {
                std::int64_t ival = 0;
                const auto res = std::from_chars(first, last, ival);
                if (res.ec == std::errc() && res.ptr == last) {
                    out = JsonValue(ival);
                    return true;
                }
            }
        }

        const std::string token(first, last);
        try {
            std::size_t idx = 0;
            const double dval = std::stod(token, &idx);
            if (idx != token.size() || !std::isfinite(dval)) {
                return fail("Numeric value out of finite double range.");
            }
            out = JsonValue(dval);
            return true;
        } catch (...) {
            return fail("Failed to parse floating-point number.");
        }
    }

    bool parseArray(JsonValue& out, int depth)
    {
        advance(); // '['
        JsonValue arr = JsonValue::makeArray();
        skipWhitespace();
        if (!eof() && peek() == ']') {
            advance();
            out = std::move(arr);
            return true;
        }
        while (true) {
            JsonValue elem;
            if (!parseValue(elem, depth + 1)) return false;
            arr.pushBack(std::move(elem));
            skipWhitespace();
            if (eof()) return fail("Unterminated JSON array.");
            if (peek() == ']') {
                advance();
                out = std::move(arr);
                return true;
            }
            if (peek() != ',') {
                return fail("Expected ',' or ']' in JSON array.");
            }
            advance();
        }
    }

    bool parseObject(JsonValue& out, int depth)
    {
        advance(); // '{'
        JsonValue obj = JsonValue::makeObject();
        skipWhitespace();
        if (!eof() && peek() == '}') {
            advance();
            out = std::move(obj);
            return true;
        }
        while (true) {
            skipWhitespace();
            std::string key;
            if (!parseString(key)) return false;
            skipWhitespace();
            if (eof() || peek() != ':') {
                return fail("Expected ':' after object key.");
            }
            advance();
            JsonValue val;
            if (!parseValue(val, depth + 1)) return false;
            obj.set(key, std::move(val));
            skipWhitespace();
            if (eof()) return fail("Unterminated JSON object.");
            if (peek() == '}') {
                advance();
                out = std::move(obj);
                return true;
            }
            if (peek() != ',') {
                return fail("Expected ',' or '}' in JSON object.");
            }
            advance();
        }
    }

private:
    const std::string& m_text;
    std::size_t m_pos = 0;
    int m_line = 1;
    int m_col = 1;
    int m_maxDepth = JsonParseLimits::kMaxNestingDepth;
    std::string m_error;
};

} // namespace

std::string JsonValue::dump(int indentSpaces) const
{
    std::ostringstream oss;
    dumpRecursive(*this, oss, indentSpaces, 0);
    if (indentSpaces > 0) {
        oss << '\n';
    }
    return oss.str();
}

bool parseJson(const std::string& input,
               JsonValue& outValue,
               std::string* outError,
               std::size_t maxBytes,
               int maxDepth)
{
    if (input.size() > maxBytes) {
        if (outError != nullptr) {
            *outError = "JSON file size (" + std::to_string(input.size()) +
                        " bytes) exceeds maximum allowed limit (" + std::to_string(maxBytes) + " bytes).";
        }
        return false;
    }

    StrictJsonParser parser(input, maxDepth);
    std::string err;
    if (!parser.parse(outValue, err)) {
        if (outError != nullptr) *outError = err;
        return false;
    }
    return true;
}

} // namespace roomcraft::persistence
