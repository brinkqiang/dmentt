#include <any>
#include <iostream>
#include <vector>
#include <entt/entt.hpp>

int main() {
    using namespace entt::literals;
    using traits_type = entt::entt_traits<entt::entity>;

    entt::registry registry;
    entt::basic_snapshot snapshot{registry};
    auto &storage = registry.storage<entt::entity>();

    std::vector<entt::any> data{};
    auto archive = [&data](auto &&elem) { data.emplace_back(std::forward<decltype(elem)>(elem)); };

    snapshot.get<entt::entity>(archive);

    return 0;
}