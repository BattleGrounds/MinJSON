# 🧩 MinJSON – минималистичная JSON-библиотека для C++20

**MinJSON** — это легковесная, высокопроизводительная JSON-библиотека с поддержкой:

- ✅ C++20-концептов (`MinJSONValueType`)
- ✅ Полноценного парсинга и сериализации
- ✅ Типобезопасного доступа через пути (`"user.name[0].value"`)
- ✅ Макросной рефлексии (в стиле `nlohmann::json`)
- ✅ Кэширования путей (`thread_local`)
- ✅ Совместима с STL: `std::string`, `std::vector`, `std::optional`

## 🚀 Пример использования

### 📌 Парсинг и доступ

```cpp
#include "MinJSON.hpp"

int main() {
    MinJSON json;
    auto result = json.parse(R"({"user":{"name":"Alice","age":30}})");

    if (std::holds_alternative<MinJSON::Error>(result)) {
        std::cerr << "Error: " << std::get<MinJSON::Error>(result) << "\n";
        return 1;
    }

    const auto& data = std::get<MinJSON::Value>(result);
    std::string name = json.get<std::string>(data, "user.name");
    int age = json.get<int>(data, "user.age");

    std::cout << name << " is " << age << " years old\n";
}
```

### 📌 Макросная рефлексия

```cpp
#include "MinJSON.hpp"

struct User {
    int id;
    std::string name;
};

MINJSON_REGISTER_TYPE(User,
    MINJSON_FIELD(id)
    MINJSON_FIELD(name)
)

int main() {
    MinJSON json;
    MinJSON_register_User(json);

    User user{42, "Bob"};
    auto value = json.to_json(user);
    std::cout << json.stringify(value, true) << "\n";

    auto user2 = json.from_json<User>(value);
    std::cout << user2.name << "\n";
}
```

## 🔧 Сборка через CMake

```bash
mkdir build && cd build
cmake ..
make
```

## 📄 Лицензия

MIT License. Свободно для использования в любых целях.
