#pragma once
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include "errors.h"

namespace TomatoInterpretato {

struct Json;

using JsonObject = std::map<std::string, Json>;
using JsonArray = std::vector<Json>;

struct JsonNumber {
    std::string text;  // original spelling, so big integers are not lost through double
};

struct Json {
    using Variant = std::variant<std::nullptr_t, bool, JsonNumber, std::string, JsonArray, JsonObject>;

    Variant value;
    Pos position;

    bool is_null() const { return std::holds_alternative<std::nullptr_t>(value); }
    bool is_number() const { return std::holds_alternative<JsonNumber>(value); }
    bool is_string() const { return std::holds_alternative<std::string>(value); }
    bool is_array() const { return std::holds_alternative<JsonArray>(value); }
    bool is_object() const { return std::holds_alternative<JsonObject>(value); }

    const std::string& as_string() const { return std::get<std::string>(value); }
    const JsonNumber& as_number() const { return std::get<JsonNumber>(value); }
    const JsonArray& as_array() const { return std::get<JsonArray>(value); }
    const JsonObject& as_object() const { return std::get<JsonObject>(value); }

    // nullptr if this is not an object or the key is absent
    const Json* find(const std::string& key) const;

    std::string type_name() const;
};

// Reads exactly one JSON value from the stream and stops right after it,
// so the rest of the stream stays available (e.g. for the program's `read`).
class JsonReader {
public:
    explicit JsonReader(std::istream& input)
        : input_(input)
    {}

    Json read();

private:
    std::istream& input_;
    Pos position_ {1, 1};

    int peek();
    int get();
    void skip_whitespace();
    void expect(char c);
    void expect_word(const std::string& word);

    Json read_value();
    Json read_object();
    Json read_array();
    Json read_number();
    std::string read_string();
};

};
