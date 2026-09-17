#pragma once
#include <iostream>
#include <variant>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <regex>
#include <experimental/random>
#include <memory>
#include "errors.h"

namespace ItmoScript {

class Function;

struct Value;
using List = std::vector<Value>;
using Dictionary = std::unordered_map<Value, Value>;
using ValueVariant = std::variant<double, std::shared_ptr<std::string>, std::shared_ptr<Function>, std::shared_ptr<List>, std::shared_ptr<Dictionary>, std::nullptr_t>;

    
    inline std::shared_ptr<std::string> new_string(std::string&& string) {
        return std::make_shared<std::string>(std::move(string));
    }
    inline std::shared_ptr<List> new_list(List&& list) {
        return std::make_shared<List>(std::move(list));
    }
    
    
    
    struct Value : public ValueVariant {
        using ValueVariant::ValueVariant;
        Value(const ValueVariant& v) : ValueVariant(v) {}
        Value(ValueVariant&& v) : ValueVariant(std::move(v)) {}
    };
    
    // Overload std::hash for Value
} // close namespace ItmoScript for std specialization

namespace std {
    template<>
    struct hash<ItmoScript::Value> {
        std::size_t operator()(ItmoScript::Value v) const {
            using namespace ItmoScript;
            struct Visitor {
                std::size_t operator()(double d) const {
                    return std::hash<double>{}(d);
                }
                std::size_t operator()(const std::shared_ptr<std::string>& s) const {
                    return s ? std::hash<std::string>{}(*s) : 0;
                }
                std::size_t operator()(std::shared_ptr<Function> f) const {
                    return std::hash<const void*>{}(f.get());
                }
                std::size_t operator()(std::shared_ptr<List> l) const {
                    if (!l) return 0;
                    std::size_t seed = l->size();
                    for (const auto& item : *l) {
                        seed ^= std::hash<Value>{}(item) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                    }
                    return seed;
                }
                std::size_t operator()(const std::shared_ptr<Dictionary>& d) const {
                    if (!d) return 0;
                    std::size_t seed = d->size();
                    for (const auto& [k, v] : *d) {
                        seed ^= std::hash<Value>{}(k) ^ (std::hash<Value>{}(v) << 1);
                    }
                    return seed;
                }
                std::size_t operator()(std::nullptr_t) const {
                    return 0xDEADBEEF;
                }
            };
            return std::visit(Visitor{}, v);
        }
    };
};

namespace ItmoScript {

namespace ValueOps {

struct Print {

    std::ostream& out;

    template<typename... Variants>
    void operator() (Variants... value) const {

        (void(out << value), ...);
    }
    
    void operator()(std::shared_ptr<std::string> value) const {
        out << *value;
    }

    void operator()(std::shared_ptr<List> value) const {
        out << "[";
        bool first = true;
        for (const auto& v : *value) {
            if (!first) out << ", ";
            std::visit(Print{out}, v);
            first = false;
        }
        out << "]";
    }

};

struct Println {

    std::ostream& out;

    template<typename... Variants>
    void operator() (Variants... value) const {

        (void(out << value << std::endl), ...);
    }
    
    void operator()(std::shared_ptr<std::string> value) const {
        out << *value << std::endl;
    }

    void operator()(std::shared_ptr<List> value) const {
        out << "[";
        bool first = true;
        for (const auto& v : *value) {
            if (!first) out << ", ";
            std::visit(Print{out}, v);
            first = false;
        }
        out << "]" << std::endl;
    }

};


struct Additive {

    Value operator() (double value1, double value2) {
        return value1 + value2;
    }

    Value operator() (std::shared_ptr<std::string> value1, std::shared_ptr<std::string> value2) {
        return new_string(*value1 + *value2);
    }

    Value operator() (std::shared_ptr<List> value1, std::shared_ptr<List> value2) {

        List result_list;

        result_list.reserve(value1->size() + value2->size());
        result_list.insert(result_list.end(), value1->begin(), value1->end());
        result_list.insert(result_list.end(), value2->begin(), value2->end());

        return new_list(std::move(result_list));
    }

    Value operator() (double value1, std::shared_ptr<std::string>  value2) {
        return new_string(std::to_string(static_cast<int>(value1)) + *value2);
    }
    Value operator() (std::shared_ptr<std::string> value1, double value2) {
        return new_string(*value1 + std::to_string(static_cast<int>(value2)));
    }
    
    template<typename T1, typename T2>
    Value operator() (T1 value1, T2 value2) {
        throw interpret_error("Unsupported types for addition: " + std::string(typeid(T1).name()) + " and " + std::string(typeid(T2).name()));
    }


};
struct Substract {

    Value operator() (double value1, double value2) {
        return value1 - value2;
    }
    
    Value operator() (std::shared_ptr<std::string> value1, std::shared_ptr<std::string> value2) {
        
        if (value1->ends_with(*value2)) {
            return new_string(value1->substr(0, value1->length() - value2->length()));
        } else {
            throw interpret_error("String1 must have string2 suffix to substract");
        }

        
    }


    Value operator() (double value1, std::shared_ptr<std::string> value2) {
        throw interpret_error("Can't substract double and string");
    }

    Value operator() (std::shared_ptr<std::string> value1, double value2) {
        throw interpret_error("Can't substract double and string");
    }

    template<typename T1, typename T2>
    Value operator() (T1 value1, T2 value2) {
        throw interpret_error("Unsupported types for substract: " + std::string(typeid(T1).name()) + " and " + std::string(typeid(T2).name()));
    }

};


struct Multiply {

    Value operator() (double value1, double value2) {
        return value1 * value2;
    }
    
    Value operator() (double value1, std::shared_ptr<std::string> value2) {
        return operator() (value2, value1);
    }
    Value operator() (std::shared_ptr<std::string> value1, double value2) {

        if (value2 < 0) {
            throw interpret_error("Can't multiply string by negative number");
        }

        std::string result;;

        for (int i = 0; i < value2; i++) {
            result += *value1;
        }

        return new_string(std::move(result));
    }

    Value operator() (std::shared_ptr<List> value1, double value2) {

        List result_list;

        result_list.reserve(value1->size() * value2);

        for (int i = 0; i < value2; i++) {
            result_list.insert(result_list.end(), value1->begin(), value1->end());
        }

        return new_list(std::move(result_list));
    }

    template<typename T1, typename T2>
    Value operator() (T1 value1, T2 value2) {
        throw interpret_error("Unsupported types for multiply: " + std::string(typeid(T1).name()) + " and " + std::string(typeid(T2).name()));
    }

};
struct Divide {

    Value operator() (double value1, double value2) {
        return value1 / value2;
    }
    
    template<typename T1, typename T2>
    Value operator() (T1 value1, T2 value2) {
        throw interpret_error("Unsupported types for divide: " + std::string(typeid(T1).name()) + " and " + std::string(typeid(T2).name()));
    }

};

struct Mod {

    Value operator() (double value1, double value2) {
        return fmod(value1, value2);
    }
    
    template<typename T1, typename T2>
    Value operator() (T1 value1, T2 value2) {
        throw interpret_error("Unsupported types for mod: " + std::string(typeid(T1).name()) + " and " + std::string(typeid(T2).name()));
    }

};
struct Power {

    Value operator() (double value1, double value2) {
        return std::pow(value1, value2);
    }
    
    template<typename T1, typename T2>
    Value operator() (T1 value1, T2 value2) {
        throw interpret_error("Unsupported types for power: " + std::string(typeid(T1).name()) + " and " + std::string(typeid(T2).name()));
    }

};
struct Revert {

    Value operator() (double value1) {
        return -value1;
    }
    
    template<typename T>
    Value operator() (T value1) {
        throw interpret_error("Unsupported types for unary minus: " + std::string(typeid(T).name()));
    }

};


struct And {

    Value operator() (double value1, double value2) {
        return static_cast<double>(static_cast<bool>(value1) && static_cast<bool>(value2));
    }

    template<typename T1, typename T2>
    Value operator() (T1 value1, T2 value2) {
        throw interpret_error("Unsupported types for and comparing: " + std::string(typeid(T1).name()) + " and " + std::string(typeid(T2).name()));
    }


};
struct Or {

    Value operator() (double value1, double value2) {
        return static_cast<double>(static_cast<bool>(value1) || static_cast<bool>(value2));
    }

    template<typename T1, typename T2>
    Value operator() (T1 value1, T2 value2) {
        throw interpret_error("Unsupported types for or comparing: " + std::string(typeid(T1).name()) + " and " + std::string(typeid(T2).name()));
    }


};
struct Not {

    Value operator() (double value) {
        return static_cast<double>(!static_cast<bool>(value));
    }

    template<typename T>
    Value operator() (T value1) {
        throw interpret_error("Unsupported type for not: " + std::string(typeid(T).name()));
    }


};

struct IsEqual {

    Value operator() (double value1, double value2) {
        return static_cast<double>(value1 == value2);
    }

    template<typename T>
    Value operator() (std::nullptr_t value1, T value2) {
        return static_cast<double>(false);
    }

    Value operator() (std::shared_ptr<std::string> value1, std::shared_ptr<std::string> value2) {
        return static_cast<double>(*value1 == *value2);
    }
    
    Value operator() (std::nullptr_t value1, std::nullptr_t value2) {
        return static_cast<double>(true);
    }

    Value operator() (std::shared_ptr<List> value1, std::shared_ptr<List> value2) {
        return static_cast<double>(*value1 == *value2);
    }

    

    template<typename T1, typename T2>
    Value operator() (T1 value1, T2 value2) {
        throw interpret_error("Unsupported types for equal comparing: " + std::string(typeid(T1).name()) + " and " + std::string(typeid(T2).name()));
    }


};

struct IsNotEqual {

    Value operator() (double value1, double value2) {
        return static_cast<double>(value1 != value2);
    }

    template<typename T>
    Value operator() (std::nullptr_t value1, T value2) {
        return static_cast<double>(true);
    }

    Value operator() (std::shared_ptr<std::string> value1, std::shared_ptr<std::string> value2) {
        return static_cast<double>(*value1 != *value2);
    }
    
    Value operator() (std::nullptr_t value1, std::nullptr_t value2) {
        return static_cast<double>(false);
    }

    template<typename T1, typename T2>
    Value operator() (T1 value1, T2 value2) {
        throw interpret_error("Unsupported types for not equal comparing: " + std::string(typeid(T1).name()) + " and " + std::string(typeid(T2).name()));
    }


};

struct IsLess {

    // null < int < string < list < function

    Value operator()(std::nullptr_t, double) { return static_cast<double>(true); }
    Value operator()(double, std::nullptr_t) { return static_cast<double>(false); }
    Value operator()(std::nullptr_t, std::shared_ptr<std::string>) { return static_cast<double>(true); }
    Value operator()(std::shared_ptr<std::string>, std::nullptr_t) { return static_cast<double>(false); }
    Value operator()(std::nullptr_t, std::shared_ptr<List>) { return static_cast<double>(true); }
    Value operator()(std::shared_ptr<List>, std::nullptr_t) { return static_cast<double>(false); }
    Value operator()(std::nullptr_t, std::shared_ptr<Function>) { return static_cast<double>(true); }
    Value operator()(std::shared_ptr<Function>, std::nullptr_t) { return static_cast<double>(false); }

    Value operator()(double, std::shared_ptr<std::string>) { return static_cast<double>(true); }
    Value operator()(std::shared_ptr<std::string>, double) { return static_cast<double>(false); }
    Value operator()(double, std::shared_ptr<List>) { return static_cast<double>(true); }
    Value operator()(std::shared_ptr<List>, double) { return static_cast<double>(false); }
    Value operator()(double, std::shared_ptr<Function>) { return static_cast<double>(true); }
    Value operator()(std::shared_ptr<Function>, double) { return static_cast<double>(false); }

    Value operator()(std::shared_ptr<std::string>, std::shared_ptr<List>) { return static_cast<double>(true); }
    Value operator()(std::shared_ptr<List>, std::shared_ptr<std::string>) { return static_cast<double>(false); }
    Value operator()(std::shared_ptr<std::string>, std::shared_ptr<Function>) { return static_cast<double>(true); }
    Value operator()(std::shared_ptr<Function>, std::shared_ptr<std::string>) { return static_cast<double>(false); }

    Value operator()(std::shared_ptr<List>, std::shared_ptr<Function>) { return static_cast<double>(true); }
    Value operator()(std::shared_ptr<Function>, std::shared_ptr<List>) { return static_cast<double>(false); }

    // value < value
    Value operator()(double value1, double value2) {
        return static_cast<double>(value1 < value2);
    }
    Value operator()(std::shared_ptr<std::string> value1, std::shared_ptr<std::string> value2) {
        return static_cast<double>(*value1 < *value2);
    }
    Value operator()(std::shared_ptr<List> value1, std::shared_ptr<List> value2) {
        return static_cast<double>(value1->size() < value2->size());
    }
    Value operator()(std::shared_ptr<Function> value1, std::shared_ptr<Function> value2) {
        return static_cast<double>(value1 < value2);
    }



    template<typename T1, typename T2>
    Value operator() (T1 value1, T2 value2) {
        throw interpret_error("Unsupported types for less comparing: " + std::string(typeid(T1).name()) + " and " + std::string(typeid(T2).name()));
    }


};


struct IsGreater {

    Value operator() (double value1, double value2) {
        return static_cast<double>(value1 > value2);
    }

    Value operator() (std::shared_ptr<std::string> value1, std::shared_ptr<std::string> value2) {
        return static_cast<double>(*value1 > *value2);
    }

    template<typename T1, typename T2>
    Value operator() (T1 value1, T2 value2) {
        throw interpret_error("Unsupported types for greater comparing: " + std::string(typeid(T1).name()) + " and " + std::string(typeid(T2).name()));
    }


};

struct IsLessEqual {

    Value operator() (double value1, double value2) {
        return static_cast<double>(value1 <= value2);
    }

    Value operator() (std::shared_ptr<std::string> value1, std::shared_ptr<std::string> value2) {
        return static_cast<double>(*value1 <= *value2);
    }

    template<typename T1, typename T2>
    Value operator() (T1 value1, T2 value2) {
        throw interpret_error("Unsupported types for less equal comparing: " + std::string(typeid(T1).name()) + " and " + std::string(typeid(T2).name()));
    }


};

struct IsGreaterEqual {

    Value operator() (double value1, double value2) {
        return static_cast<double>(value1 >= value2);
    }

    Value operator() (std::shared_ptr<std::string> value1, std::shared_ptr<std::string> value2) {
        return static_cast<double>(*value1 >= *value2);
    }

    template<typename T1, typename T2>
    Value operator() (T1 value1, T2 value2) {
        throw interpret_error("Unsupported types for greater equal comparing: " + std::string(typeid(T1).name()) + " and " + std::string(typeid(T2).name()));
    }


};

struct Index {

    Value index;

    Value operator() (std::shared_ptr<std::string> value) {

        if (!std::holds_alternative<double>(index)) {
            throw interpret_error("Index for string must be a integer");
        }

        int raw_index_i = std::get<double>(index);

        if (abs(raw_index_i) >= value->length()) {
            throw interpret_error("Index must be < string lenght");
        }

        int index_i = raw_index_i < 0 ? value->length() + raw_index_i : raw_index_i;

        return new_string(std::string(1, (*value)[index_i]));
    }
    Value operator() (std::shared_ptr<List> value) {

        if (!std::holds_alternative<double>(index)) {
            throw interpret_error("Index for list must be a integer");
        }

        int raw_index_i = std::get<double>(index);

        int index_i = raw_index_i < 0 ? value->size() + raw_index_i : raw_index_i;

        return (*value)[index_i];
    }
    Value operator() (std::shared_ptr<Dictionary> value) {

        return (*value)[index];
    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for indexing: " + std::string(typeid(T).name()));
    }


};

struct Slice {

    std::optional<int> start_opt;
    std::optional<int> end_opt;
    std::optional<int> step_opt;

    Value operator() (std::shared_ptr<std::string> value) {

        int start;
        int end;
        int step = step_opt ? step_opt.value() : 1.0;

        if (step > 0) {
            if (start_opt) {
                start = start_opt.value() < 0 ? value->length() + start_opt.value() : start_opt.value();
            } else {
                start = 0;
            }
            
            if (end_opt) {
                end = end_opt.value() < 0 ? value->length() + end_opt.value() : end_opt.value();
            } else {
                end = value->length();
            }
        } else if (step < 0) {
            if (start_opt) {
                start = start_opt.value() < 0 ? value->length() + start_opt.value() : start_opt.value();
            } else {
                start = value->length() != 0 ? value->length() - 1 : 0;
            }
            
            if (end_opt) {
                end = end_opt.value() < 0 ? value->length() + end_opt.value() : end_opt.value();
            } else {
                end = -1;
            }
        } else {
            throw interpret_error("Slice step cannot be zero");
        }

        std::string result_str;

        if (step > 0) {
            for (int i = start; i < end && i < value->length(); i += step) {
                result_str.push_back((*value)[i]);
            }
        } else {
            for (int i = start; i > end && i < value->length(); i += step) {
                result_str.push_back((*value)[i]);
            }
        }

        return new_string(std::move(result_str));
    }
    Value operator() (std::shared_ptr<List> value) {

        int start;
        int end;
        int step = step_opt ? step_opt.value() : 1;

        if (step > 0) {
            if (start_opt) {
                start = start_opt.value() < 0 ? value->size() + start_opt.value() : start_opt.value();
            } else {
                start = 0;
            }
            
            if (end_opt) {
                end = end_opt.value() < 0 ? value->size() + end_opt.value() : end_opt.value();
            } else {
                end = value->size();
            }
        } else if (step < 0) {
            if (start_opt) {
                start = start_opt.value() < 0 ? value->size() + start_opt.value() : start_opt.value();
            } else {
                start = value->size() != 0 ? value->size() - 1 : 0;
            }
            
            if (end_opt) {
                end = end_opt.value() < 0 ? value->size() + end_opt.value() : end_opt.value();
            } else {
                end = -1;
            }
        } else {
            throw interpret_error("Slice step cannot be zero");
        }

        List result_list;

        if (step > 0) {
            for (int i = start; i < end && i < value->size(); i += step) {
                result_list.push_back((*value)[i]);
            }
        } else {
            for (int i = start; i > end && i < value->size(); i += step) {
                result_list.push_back((*value)[i]);
            }
        }

        return new_list(std::move(result_list));
    }


    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for slicing: " + std::string(typeid(T).name()));
    }

};

struct Len {

    Value operator() (std::shared_ptr<std::string> value) {

        return static_cast<double>(value->length());
    }
    Value operator() (std::shared_ptr<List> value) {

        return static_cast<double>(value->size());
    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for lenght: " + std::string(typeid(T).name()));
    }


};

struct Abs {

    Value operator() (double value) {

        return std::abs(value);
    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for abs: " + std::string(typeid(T).name()));
    }

};

struct Ceil {

    Value operator() (double value) {

        return std::ceil(value);
    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for ceil: " + std::string(typeid(T).name()));
    }

};

struct Floor {

    Value operator() (double value) {

        return std::floor(value);
    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for floor: " + std::string(typeid(T).name()));
    }

};
struct Round {

    Value operator() (double value) {

        return std::round(value);
    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for round: " + std::string(typeid(T).name()));
    }

};
struct Sqrt {

    Value operator() (double value) {

        return std::sqrt(value);
    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for sqrt: " + std::string(typeid(T).name()));
    }

};

struct Rnd {

    Value operator() (double value) {

        return static_cast<double>(std::experimental::randint(0, static_cast<int>(value)));
    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for rnd: " + std::string(typeid(T).name()));
    }

};

struct ParseNum {

    Value operator() (std::shared_ptr<std::string> value) {

        try {
            return std::stod(*value);
        } catch (const std::invalid_argument&) {
            return nullptr;
        }
    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for parse_num: " + std::string(typeid(T).name()));
    }

};

struct ToString {

    Value operator() (double value) {

        return new_string(std::to_string(value));

    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for to_string: " + std::string(typeid(T).name()));
    }

};



struct Lower {

    Value operator() (std::shared_ptr<std::string> value) {

        std::transform(value->begin(), value->end(), value->begin(),
            [](unsigned char c){ return std::tolower(c); });

        return value;

    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for lower: " + std::string(typeid(T).name()));
    }

};

struct Upper {

    Value operator() (std::shared_ptr<std::string> value) {

        std::transform(value->begin(), value->end(), value->begin(),
            [](unsigned char c){ return std::toupper(c); });

        return value;

    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for upper: " + std::string(typeid(T).name()));
    }

};

struct Split {

    std::string delimiter;

    Value operator() (std::shared_ptr<std::string> value) {

        List splitted_string;

        
        std::regex reg("(" + delimiter + ")");
        
        for (auto i = std::sregex_token_iterator(value->begin(), value->end(), reg, -1); i != std::sregex_token_iterator(); ++i) {
            splitted_string.push_back(new_string(i->str()));
        }

        return new_list(std::move(splitted_string));

    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for rnd: " + std::string(typeid(T).name()));
    }

};

struct Join {

    std::string delimiter;

    Value operator() (std::shared_ptr<List> value) {

        std::string result;

        for (const auto& v : *value) {

            if (!result.empty()) {
                result += delimiter;
            }

            if (std::holds_alternative<std::shared_ptr<std::string>>(v)) {
                result += *std::get<std::shared_ptr<std::string>>(v);
            } else if (std::holds_alternative<double>(v)) {
                result += std::to_string(std::get<double>(v));
            } else {
                throw interpret_error("Unsupported type for join: " + std::string(typeid(v).name()));
            }
        }

        return new_string(std::move(result));
        

    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for join: " + std::string(typeid(T).name()));
    }

};
struct Replace {

    std::string old_substr;
    std::string new_substr;

    Value operator() (std::shared_ptr<std::string> value) {
    
        *value = std::regex_replace(*value, std::regex(old_substr), new_substr);
    
        return value;
        
    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for replace: " + std::string(typeid(T).name()));
    }

};
struct Push {

    Value to_push;

    Value operator() (std::shared_ptr<List> value) {
    
        value->push_back(to_push);
        
        return value;
        
    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for push: " + std::string(typeid(T).name()));
    }

};

struct Pop {

    Value operator() (std::shared_ptr<List> value) {
    
        if (value->empty()) {
            throw interpret_error("Can't pop empty list");
        }

        Value popped_value = value->back();

        value->pop_back();
        
        return popped_value;
        
    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for pop: " + std::string(typeid(T).name()));
    }

};

struct Insert {

    int index;
    Value to_insert;

    Value operator() (std::shared_ptr<List> value) {
    
        if (index < 0 || index > value->size()) {
            throw interpret_error("Index out of bounds for insert: " + std::to_string(index));
        }

        value->insert(value->begin() + index, to_insert);

        return value;
        
    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for insert: " + std::string(typeid(T).name()));
    }

};

struct Remove {

    int index;

    Value operator() (std::shared_ptr<List> value) {
    
        if (index < 0 || index > value->size()) {
            throw interpret_error("Index out of bounds for remove: " + std::to_string(index));
        }

        value->erase(value->begin() + index);

        return value;
        
    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for remove: " + std::string(typeid(T).name()));
    }

};

struct Sort {

    Value operator() (std::shared_ptr<List> value) {
    
        std::shared_ptr<List> sorted_list = std::make_shared<List>(*value);

        std::sort(sorted_list->begin(), sorted_list->end(), [](const Value& a, const Value& b) {
            return static_cast<bool>(std::get<double>(std::visit(IsLess{}, a, b)));
        });

        return sorted_list;
        
    }

    template<typename T>
    Value operator() (T value) {
        throw interpret_error("Unsupported type for remove: " + std::string(typeid(T).name()));
    }

};








}; // namespace ValueOps



}; // namespace ItmoScript

namespace ItmoScript {
    inline bool operator==(const Value& lhs, const Value& rhs) {
        return static_cast<bool>(std::get<double>(std::visit(ValueOps::IsEqual{}, lhs, rhs)));
    }
}

