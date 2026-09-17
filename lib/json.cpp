#include "json.h"

namespace TomatoInterpretato {

const Json* Json::find(const std::string& key) const {
    if (!is_object()) return nullptr;

    const auto& object = as_object();
    auto it = object.find(key);
    return it == object.end() ? nullptr : &it->second;
}

std::string Json::type_name() const {
    switch (value.index()) {
        case 0: return "null";
        case 1: return "boolean";
        case 2: return "number";
        case 3: return "string";
        case 4: return "array";
        default: return "object";
    }
}

Json JsonReader::read() {
    Json result = read_value();
    return result;
}

int JsonReader::peek() {
    return input_.peek();
}

int JsonReader::get() {
    int c = input_.get();
    if (c == '\n') {
        position_.line++;
        position_.column = 1;
    } else if (c != EOF) {
        position_.column++;
    }
    return c;
}

void JsonReader::skip_whitespace() {
    while (true) {
        int c = peek();
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            get();
        } else {
            return;
        }
    }
}

void JsonReader::expect(char c) {
    Pos at = position_;
    int got = get();
    if (got != c) {
        std::string found = got == EOF ? "end of input" : std::string("'") + static_cast<char>(got) + "'";
        throw json_error(std::string("Expected '") + c + "', found " + found, at);
    }
}

void JsonReader::expect_word(const std::string& word) {
    for (char c : word) {
        expect(c);
    }
}

Json JsonReader::read_value() {
    skip_whitespace();
    Pos at = position_;
    int c = peek();

    switch (c) {
        case '{': return read_object();
        case '[': return read_array();
        case '"': return Json{read_string(), at};
        case 't': expect_word("true"); return Json{true, at};
        case 'f': expect_word("false"); return Json{false, at};
        case 'n': expect_word("null"); return Json{nullptr, at};
        case EOF: throw json_error("Unexpected end of input", at);
        default:
            if (c == '-' || (c >= '0' && c <= '9')) return read_number();
            throw json_error(std::string("Unexpected character '") + static_cast<char>(c) + "'", at);
    }
}

Json JsonReader::read_object() {
    Json result{JsonObject{}, position_};
    auto& object = std::get<JsonObject>(result.value);
    expect('{');
    skip_whitespace();

    if (peek() == '}') {
        get();
        return result;
    }

    while (true) {
        skip_whitespace();
        Pos key_pos = position_;
        if (peek() != '"') throw json_error("Expected string key", key_pos);
        std::string key = read_string();

        skip_whitespace();
        expect(':');
        Json value = read_value();

        if (object.contains(key)) throw json_error("Duplicate key \"" + key + "\"", key_pos);
        object.emplace(std::move(key), std::move(value));

        skip_whitespace();
        if (peek() == ',') {
            get();
            continue;
        }
        expect('}');
        return result;
    }
}

Json JsonReader::read_array() {
    Json result{JsonArray{}, position_};
    auto& array = std::get<JsonArray>(result.value);
    expect('[');
    skip_whitespace();

    if (peek() == ']') {
        get();
        return result;
    }

    while (true) {
        array.push_back(read_value());
        skip_whitespace();
        if (peek() == ',') {
            get();
            continue;
        }
        expect(']');
        return result;
    }
}

Json JsonReader::read_number() {
    Pos at = position_;
    std::string text;

    auto take_digits = [&]() {
        bool any = false;
        while (peek() >= '0' && peek() <= '9') {
            text.push_back(static_cast<char>(get()));
            any = true;
        }
        if (!any) throw json_error("Malformed number", at);
    };

    if (peek() == '-') text.push_back(static_cast<char>(get()));
    take_digits();
    if (peek() == '.') {
        text.push_back(static_cast<char>(get()));
        take_digits();
    }
    if (peek() == 'e' || peek() == 'E') {
        text.push_back(static_cast<char>(get()));
        if (peek() == '+' || peek() == '-') text.push_back(static_cast<char>(get()));
        take_digits();
    }

    return Json{JsonNumber{std::move(text)}, at};
}

std::string JsonReader::read_string() {
    Pos at = position_;
    expect('"');
    std::string result;

    while (true) {
        int c = get();
        if (c == EOF) throw json_error("Unterminated string", at);
        if (c == '"') return result;
        if (c != '\\') {
            result.push_back(static_cast<char>(c));
            continue;
        }

        int e = get();
        switch (e) {
            case '"': result.push_back('"'); break;
            case '\\': result.push_back('\\'); break;
            case '/': result.push_back('/'); break;
            case 'b': result.push_back('\b'); break;
            case 'f': result.push_back('\f'); break;
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            case 'u': {
                unsigned code = 0;
                for (int i = 0; i < 4; ++i) {
                    int h = get();
                    code <<= 4;
                    if (h >= '0' && h <= '9') code |= h - '0';
                    else if (h >= 'a' && h <= 'f') code |= h - 'a' + 10;
                    else if (h >= 'A' && h <= 'F') code |= h - 'A' + 10;
                    else throw json_error("Bad \\u escape", at);
                }
                // identifiers are ASCII; encode the rest as UTF-8 without surrogate handling
                if (code < 0x80) {
                    result.push_back(static_cast<char>(code));
                } else if (code < 0x800) {
                    result.push_back(static_cast<char>(0xC0 | (code >> 6)));
                    result.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                } else {
                    result.push_back(static_cast<char>(0xE0 | (code >> 12)));
                    result.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                    result.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                }
                break;
            }
            default:
                throw json_error("Bad escape sequence", at);
        }
    }
}

};
