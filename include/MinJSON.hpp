#pragma once

// Проверка поддержки C++20
#if !defined(__cplusplus) || __cplusplus < 202002L
#error "MinJSON requires C++20 or newer"
#endif

#include <any>
#include <charconv>
#include <concepts>
#include <cctype>
#include <format>
#include <iostream>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <functional>
#include <memory>
#include <sstream>
#include <algorithm>

/**
 * @brief Концепт поддерживаемых типов значений
 */
template <typename T>
concept MinJSONValueType =
    std::same_as<T, int> ||
    std::same_as<T, double> ||
    std::same_as<T, bool> ||
    std::same_as<T, std::string> ||
    std::same_as<T, std::nullptr_t> ||
    requires(T vec) {
        requires std::same_as<T, std::vector<typename T::value_type>>;
        requires MinJSONValueType<typename T::value_type>;
    } ||
    requires(T opt) {
        requires std::same_as<T, std::optional<typename T::value_type>>;
        requires MinJSONValueType<typename T::value_type>;
    };

namespace MinJSON::detail {
    template <typename T> struct is_vector : std::false_type {};
    template <typename T> struct is_vector<std::vector<T>> : std::true_type {};
    
    template <typename T> struct is_optional : std::false_type {};
    template <typename T> struct is_optional<std::optional<T>> : std::true_type {};
}

/**
 * @brief Высокопроизводительная JSON-библиотека для C++20
 */
class MinJSON {
public:
    using Value = std::any;
    using Object = std::unordered_map<std::string, Value>;
    using Array = std::vector<Value>;
    using Error = std::string;
    
    template <typename T> using Result = std::variant<T, Error>;

    struct KeySegment {
        std::string value;
        KeySegment(std::string s) : value(std::move(s)) {}
    };
    
    struct IndexSegment {
        size_t value;
        IndexSegment(size_t i) : value(i) {}
    };
    
    using PathSegment = std::variant<KeySegment, IndexSegment>;
    
    struct Reflector {
        virtual ~Reflector() = default;
        virtual Value to_json(const void* object) const = 0;
        virtual void from_json(const Value& json_value, void* object) const = 0;
    };
    
    using ReflectionRegistry = std::unordered_map<std::string, std::shared_ptr<Reflector>>;

    MinJSON() {
        register_builtin_types();
    }

    // Парсинг и сериализация
    [[nodiscard]] Result<Value> parse(std::string_view input) noexcept;
    [[nodiscard]] std::string stringify(const Value& value, bool pretty = false) const noexcept;
    
    // Доступ к данным
    template <MinJSONValueType T>
    [[nodiscard]] T get(const Value& root, std::string_view path, const T& default_val = {}) const noexcept;
    
    template <MinJSONValueType T>
    [[nodiscard]] Result<T> get_checked(const Value& root, std::string_view path) const noexcept;
    
    [[nodiscard]] std::optional<Error> set(Value& root, std::string_view path, Value value) noexcept;
    
    // Рефлексия
    template <typename T>
    void register_reflector(std::shared_ptr<Reflector> reflector);
    
    template <typename T>
    [[nodiscard]] Value to_json(const T& object) const;
    
    template <typename T>
    [[nodiscard]] T from_json(const Value& json_value) const;
    
    static void clear_path_cache() noexcept {
        path_cache_.clear();
    }

private:
    std::string_view text_;
    size_t pos_ = 0;
    
    static inline thread_local std::unordered_map<
        std::string, 
        std::vector<PathSegment>
    > path_cache_;
    
    ReflectionRegistry reflection_registry_;

    // Вспомогательные методы
    void skip_whitespace() noexcept;
    char peek() const noexcept;
    char consume() noexcept;
    
    [[nodiscard]] Result<Value> parse_value();
    [[nodiscard]] Result<Value> parse_null();
    [[nodiscard]] Result<Value> parse_bool();
    [[nodiscard]] Result<Value> parse_string();
    [[nodiscard]] Result<Value> parse_number();
    [[nodiscard]] Result<Value> parse_array();
    [[nodiscard]] Result<Value> parse_object();
    
    [[nodiscard]] std::string quote_string(std::string_view str) const noexcept;
    [[nodiscard]] std::string stringify_array(const Array& arr, bool pretty) const noexcept;
    [[nodiscard]] std::string stringify_object(const Object& obj, bool pretty) const noexcept;
    
    void register_builtin_types() noexcept;
    [[nodiscard]] std::optional<Value> reflect_to_json(const Value& value) const noexcept;
    
    [[nodiscard]] std::vector<PathSegment> parse_path(std::string_view path) const;
    [[nodiscard]] const Value* find_value(const Value& root, std::string_view path) const noexcept;
    [[nodiscard]] const Value* traverse_path(const Value* current, const std::vector<PathSegment>& segments) const noexcept;
    
    template <MinJSONValueType T>
    [[nodiscard]] T extract_value(const Value& value, const T& default_val) const noexcept;
    
    void handle_segment(Value& node, const KeySegment& seg, bool is_last);
    void handle_segment(Value& node, const IndexSegment& seg, bool is_last);
    void advance(Value*& current, const KeySegment& seg);
    void advance(Value*& current, const IndexSegment& seg);
    
    // Утилиты
    [[nodiscard]] static bool is_digit(char c) noexcept;
    [[nodiscard]] static bool is_whitespace(char c) noexcept;
    [[nodiscard]] static std::string to_lower(std::string str) noexcept;
};

// Реализация методов парсинга
inline void MinJSON::skip_whitespace() noexcept {
    while (pos_ < text_.size() && is_whitespace(text_[pos_])) {
        ++pos_;
    }
}

inline char MinJSON::peek() const noexcept {
    return pos_ < text_.size() ? text_[pos_] : '\0';
}

inline char MinJSON::consume() noexcept {
    return pos_ < text_.size() ? text_[pos_++] : '\0';
}

inline bool MinJSON::is_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

inline bool MinJSON::is_whitespace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

inline std::string MinJSON::to_lower(std::string str) noexcept {
    std::transform(str.begin(), str.end(), str.begin(), 
        [](unsigned char c) { return std::tolower(c); });
    return str;
}

inline MinJSON::Result<MinJSON::Value> MinJSON::parse_value() {
    const char c = peek();
    if (c == 'n') return parse_null();
    if (c == 't' || c == 'f') return parse_bool();
    if (c == '"') return parse_string();
    if (c == '[') return parse_array();
    if (c == '{') return parse_object();
    if (is_digit(c) || c == '-' || c == '.') return parse_number();
    
    return Error("Unexpected character: " + std::string(1, c));
}

inline MinJSON::Result<MinJSON::Value> MinJSON::parse_null() {
    if (text_.substr(pos_, 4) == "null") {
        pos_ += 4;
        return nullptr;
    }
    return Error("Expected 'null'");
}

inline MinJSON::Result<MinJSON::Value> MinJSON::parse_bool() {
    if (text_.substr(pos_, 4) == "true") {
        pos_ += 4;
        return true;
    }
    if (text_.substr(pos_, 5) == "false") {
        pos_ += 5;
        return false;
    }
    return Error("Expected boolean value");
}

inline MinJSON::Result<MinJSON::Value> MinJSON::parse_string() {
    if (consume() != '"') {
        return Error("Expected '\"'");
    }
    
    std::string result;
    while (pos_ < text_.size() && peek() != '"') {
        char c = consume();
        if (c == '\\') {
            char esc = consume();
            switch (esc) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u': {
                    if (pos_ + 4 > text_.size()) {
                        return Error("Incomplete Unicode escape");
                    }
                    result += text_.substr(pos_, 4);
                    pos_ += 4;
                    break;
                }
                default: result += esc;
            }
        } else {
            result += c;
        }
    }
    
    if (consume() != '"') {
        return Error("Unterminated string");
    }
    return result;
}

inline MinJSON::Result<MinJSON::Value> MinJSON::parse_number() {
    size_t start = pos_;
    bool is_float = false;
    
    if (peek() == '-') consume();
    
    while (pos_ < text_.size()) {
        char c = peek();
        if (is_digit(c)) {
            consume();
        } else if (c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-') {
            is_float = true;
            consume();
        } else {
            break;
        }
    }
    
    std::string num_str(text_.substr(start, pos_ - start));
    try {
        if (is_float) {
            return std::stod(num_str);
        }
        return std::stoi(num_str);
    } catch (...) {
        return Error("Invalid number: " + num_str);
    }
}

inline MinJSON::Result<MinJSON::Value> MinJSON::parse_array() {
    consume(); // '['
    skip_whitespace();
    
    Array result;
    if (peek() == ']') {
        consume();
        return result;
    }
    
    while (true) {
        auto value = parse_value();
        if (auto* err = std::get_if<Error>(&value)) {
            return *err;
        }
        result.push_back(std::get<Value>(std::move(value)));
        skip_whitespace();
        
        if (peek() == ',') {
            consume();
            skip_whitespace();
        } else if (peek() == ']') {
            consume();
            break;
        } else {
            return Error("Expected ',' or ']' in array");
        }
    }
    return result;
}

inline MinJSON::Result<MinJSON::Value> MinJSON::parse_object() {
    consume(); // '{'
    skip_whitespace();
    
    Object result;
    if (peek() == '}') {
        consume();
        return result;
    }
    
    while (true) {
        auto key = parse_string();
        if (auto* err = std::get_if<Error>(&key)) {
            return *err;
        }
        skip_whitespace();
        
        if (consume() != ':') {
            return Error("Expected ':' in object");
        }
        skip_whitespace();
        
        auto value = parse_value();
        if (auto* err = std::get_if<Error>(&value)) {
            return *err;
        }
        
        result[std::any_cast<std::string>(std::get<Value>(key))] = 
            std::get<Value>(std::move(value));
        skip_whitespace();
        
        if (peek() == ',') {
            consume();
            skip_whitespace();
        } else if (peek() == '}') {
            consume();
            break;
        } else {
            return Error("Expected ',' or '}' in object");
        }
    }
    return result;
}

// Сериализация
inline std::string MinJSON::quote_string(std::string_view str) const noexcept {
    std::ostringstream oss;
    oss << '"';
    for (char c : str) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: 
                if (c < 0x20) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    oss << c;
                }
        }
    }
    oss << '"';
    return oss.str();
}

inline std::string MinJSON::stringify_array(const Array& arr, bool pretty) const noexcept {
    std::string result = "[";
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) result += ",";
        result += stringify(arr[i], pretty);
    }
    return result + "]";
}

inline std::string MinJSON::stringify_object(const Object& obj, bool pretty) const noexcept {
    std::string result = "{";
    bool first = true;
    for (const auto& [key, value] : obj) {
        if (!first) result += ",";
        result += quote_string(key) + ":" + stringify(value, pretty);
        first = false;
    }
    return result + "}";
}

inline MinJSON::Result<MinJSON::Value> MinJSON::parse(std::string_view input) noexcept {
    try {
        text_ = input;
        pos_ = 0;
        skip_whitespace();
        return parse_value();
    } catch (const std::exception& e) {
        return Error(e.what());
    }
}

inline std::string MinJSON::stringify(const Value& value, bool pretty) const noexcept {
    try {
        if (!value.has_value()) return "null";
        if (const auto* p = std::any_cast<bool>(&value)) return *p ? "true" : "false";
        if (const auto* p = std::any_cast<int>(&value)) return std::to_string(*p);
        if (const auto* p = std::any_cast<double>(&value)) return std::to_string(*p);
        if (const auto* p = std::any_cast<std::string>(&value)) return quote_string(*p);
        if (const auto* p = std::any_cast<Array>(&value)) return stringify_array(*p, pretty);
        if (const auto* p = std::any_cast<Object>(&value)) return stringify_object(*p, pretty);
        
        if (auto reflected = reflect_to_json(value); reflected) {
            return stringify(*reflected, pretty);
        }
        
        return "\"<unsupported type>\"";
    } catch (...) {
        return "\"<stringify error>\"";
    }
}

// Доступ к данным
inline std::vector<MinJSON::PathSegment> MinJSON::parse_path(std::string_view path) const {
    std::string path_str(path);
    if (auto it = path_cache_.find(path_str); it != path_cache_.end()) {
        return it->second;
    }
    
    std::vector<PathSegment> segments;
    size_t start = 0;
    const size_t length = path.size();
    
    while (start < length) {
        if (path[start] == '[') {
            // Индекс массива
            start++;
            size_t end = path.find(']', start);
            if (end == std::string::npos) {
                throw std::runtime_error("Unclosed array index");
            }
            
            size_t index = 0;
            auto result = std::from_chars(
                path.data() + start,
                path.data() + end,
                index
            );
            
            if (result.ec != std::errc()) {
                throw std::runtime_error("Invalid array index: " + std::string(path.substr(start, end - start)));
            }
            
            segments.emplace_back(IndexSegment{index});
            start = end + 1;
        } else {
            // Ключ объекта
            size_t end = path.find('[', start);
            if (end == std::string::npos) {
                end = length;
            }
            
            std::string key(path.substr(start, end - start));
            if (!key.empty()) {
                segments.emplace_back(KeySegment{std::move(key)});
            }
            start = end;
        }
        
        // Пропуск разделителя
        if (start < length && path[start] == '.') {
            start++;
        }
    }
    
    path_cache_[path_str] = segments;
    return segments;
}

inline const MinJSON::Value* MinJSON::traverse_path(
    const Value* current, 
    const std::vector<PathSegment>& segments
) const noexcept {
    for (const auto& segment : segments) {
        if (!current) return nullptr;
        
        if (auto key = std::get_if<KeySegment>(&segment)) {
            if (auto obj = std::any_cast<Object>(current)) {
                if (auto it = obj->find(key->value); it != obj->end()) {
                    current = &it->second;
                } else {
                    return nullptr;
                }
            } else {
                return nullptr;
            }
        } else if (auto index = std::get_if<IndexSegment>(&segment)) {
            if (auto arr = std::any_cast<Array>(current)) {
                if (index->value < arr->size()) {
                    current = &(*arr)[index->value];
                } else {
                    return nullptr;
                }
            } else {
                return nullptr;
            }
        }
    }
    return current;
}

inline const MinJSON::Value* MinJSON::find_value(
    const Value& root, 
    std::string_view path
) const noexcept {
    try {
        auto segments = parse_path(path);
        return traverse_path(&root, segments);
    } catch (...) {
        return nullptr;
    }
}

template <MinJSONValueType T>
T MinJSON::extract_value(const Value& value, const T& default_val) const noexcept {
    try {
        if constexpr (detail::is_optional<T>::value) {
            using ValueType = typename T::value_type;
            if (value.type() == typeid(std::nullptr_t)) {
                return std::nullopt;
            }
            return extract_value<ValueType>(value, ValueType{});
        }
        else if constexpr (detail::is_vector<T>::value) {
            using ElementType = typename T::value_type;
            if (const auto* arr = std::any_cast<Array>(&value)) {
                T result;
                result.reserve(arr->size());
                for (const auto& item : *arr) {
                    result.push_back(extract_value<ElementType>(item, ElementType{}));
                }
                return result;
            }
            return default_val;
        }
        else if constexpr (std::same_as<T, std::string>) {
            if (const auto* p = std::any_cast<std::string>(&value)) return *p;
            if (const auto* p = std::any_cast<int>(&value)) return std::to_string(*p);
            if (const auto* p = std::any_cast<double>(&value)) return std::to_string(*p);
            if (const auto* p = std::any_cast<bool>(&value)) return *p ? "true" : "false";
            return default_val;
        }
        else {
            if (const auto* p = std::any_cast<T>(&value)) return *p;
            
            // Попробуем преобразовать
            if constexpr (std::integral<T>) {
                if (const auto* p = std::any_cast<double>(&value)) return static_cast<T>(*p);
                if (const auto* p = std::any_cast<bool>(&value)) return static_cast<T>(*p);
            }
            else if constexpr (std::floating_point<T>) {
                if (const auto* p = std::any_cast<int>(&value)) return static_cast<T>(*p);
            }
            else if constexpr (std::same_as<T, bool>) {
                if (const auto* p = std::any_cast<int>(&value)) return *p != 0;
                if (const auto* p = std::any_cast<double>(&value)) return *p != 0.0;
            }
            
            return default_val;
        }
    } catch (...) {
        return default_val;
    }
}

template <MinJSONValueType T>
T MinJSON::get(const Value& root, std::string_view path, const T& default_val) const noexcept {
    if (auto value = find_value(root, path)) {
        return extract_value<T>(*value, default_val);
    }
    return default_val;
}

template <MinJSONValueType T>
MinJSON::Result<T> MinJSON::get_checked(const Value& root, std::string_view path) const noexcept {
    if (auto value = find_value(root, path)) {
        try {
            return extract_value<T>(*value, T{});
        } catch (const std::exception& e) {
            return Error(e.what());
        }
    }
    return Error("Path not found: " + std::string(path));
}

inline std::optional<MinJSON::Error> MinJSON::set(
    Value& root, 
    std::string_view path, 
    Value value
) noexcept {
    try {
        auto segments = parse_path(path);
        if (segments.empty()) {
            root = std::move(value);
            return std::nullopt;
        }

        Value* current = &root;
        for (size_t i = 0; i < segments.size(); ++i) {
            const bool last = (i == segments.size() - 1);
            
            std::visit([&](auto&& seg) {
                handle_segment(*current, seg, last);
                advance(current, seg);
            }, segments[i]);

            if (last) {
                *current = std::move(value);
            }
        }
        return std::nullopt;
    } catch (const std::exception& e) {
        return Error(e.what());
    }
}

// Рефлексия
template <typename T>
void MinJSON::register_reflector(std::shared_ptr<Reflector> reflector) {
    reflection_registry_[typeid(T).name()] = reflector;
}

template <typename T>
MinJSON::Value MinJSON::to_json(const T& object) const {
    auto it = reflection_registry_.find(typeid(T).name());
    if (it != reflection_registry_.end()) {
        return it->second->to_json(&object);
    }
    throw std::runtime_error("Reflector not registered for type");
}

template <typename T>
T MinJSON::from_json(const Value& json_value) const {
    T object;
    auto it = reflection_registry_.find(typeid(T).name());
    if (it != reflection_registry_.end()) {
        it->second->from_json(json_value, &object);
        return object;
    }
    throw std::runtime_error("Reflector not registered for type");
}

inline void MinJSON::register_builtin_types() noexcept {
    // Встроенные типы не требуют специальной регистрации
}

inline std::optional<MinJSON::Value> MinJSON::reflect_to_json(const Value& value) const noexcept {
    try {
        const std::type_info& ti = value.type();
        auto it = reflection_registry_.find(ti.name());
        if (it != reflection_registry_.end()) {
            return it->second->to_json(&value);
        }
    } catch (...) {
        // Игнорируем ошибки при рефлексии
    }
    return std::nullopt;
}

// Вспомогательные методы для установки значений
inline void MinJSON::handle_segment(Value& node, const KeySegment& seg, bool is_last) {
    if (!node.has_value() || node.type() != typeid(Object)) {
        node = Object{};
    }
    
    auto& obj = *std::any_cast<Object>(&node);
    if (obj.find(seg.value) == obj.end()) {
        obj[seg.value] = is_last ? Value{} : Object{};
    }
}

inline void MinJSON::handle_segment(Value& node, const IndexSegment& seg, bool is_last) {
    if (!node.has_value() || node.type() != typeid(Array)) {
        node = Array{};
    }
    
    auto& arr = *std::any_cast<Array>(&node);
    if (seg.value >= arr.size()) {
        arr.resize(seg.value + 1);
    }
    
    if (!is_last && !arr[seg.value].has_value()) {
        arr[seg.value] = Object{};
    }
}

inline void MinJSON::advance(Value*& current, const KeySegment& seg) {
    if (auto obj = std::any_cast<Object>(current)) {
        current = &(*obj)[seg.value];
    } else {
        throw std::runtime_error("Expected object at: " + seg.value);
    }
}

inline void MinJSON::advance(Value*& current, const IndexSegment& seg) {
    if (auto arr = std::any_cast<Array>(current)) {
        if (seg.value < arr->size()) {
            current = &(*arr)[seg.value];
        } else {
            throw std::runtime_error("Index out of range: " + std::to_string(seg.value));
        }
    } else {
        throw std::runtime_error("Expected array at index: " + std::to_string(seg.value));
    }
}

// Реализация рефлексии через макросы
#define MINJSON_REGISTER_TYPE(TYPE, ...) \
namespace MinJSON { \
    template <> \
    struct ReflectorImpl<TYPE> : Reflector { \
        Value to_json(const void* object) const override { \
            const TYPE& obj = *static_cast<const TYPE*>(object); \
            Object result; \
            __VA_ARGS__ \
            return result; \
        } \
        void from_json(const Value& json_value, void* object) const override { \
            TYPE& obj = *static_cast<TYPE*>(object); \
            const auto& obj_map = std::any_cast<Object>(json_value); \
            __VA_ARGS__ \
        } \
    }; \
} \
static void MINJSON_register_##TYPE(MinJSON& json) { \
    json.register_reflector<TYPE>(std::make_shared<MinJSON::ReflectorImpl<TYPE>>()); \
}

#define MINJSON_FIELD(FIELD) \
    result[#FIELD] = MinJSON::to_json(obj.FIELD); \
    if (auto it = obj_map.find(#FIELD); it != obj_map.end()) { \
        obj.FIELD = MinJSON::from_json<std::remove_reference_t<decltype(obj.FIELD)>>(it->second); \
    }