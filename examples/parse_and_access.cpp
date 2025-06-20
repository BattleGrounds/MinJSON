#include "MinJSON.hpp"
#include <iostream>

int main() {
    MinJSON json;
    auto result = json.parse(R"({"items":[{"name":"apple"},{"name":"banana"}]})");

    if (std::holds_alternative<MinJSON::Error>(result)) {
        std::cerr << "Parse error\n";
        return 1;
    }

    const auto& data = std::get<MinJSON::Value>(result);
    std::string name = json.get<std::string>(data, "items[1].name");
    std::cout << name << "\n"; // banana
}
