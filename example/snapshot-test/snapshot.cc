#include <iostream>
#include <sstream> // 用于在内存中存储序列化数据
#include <string>

// 1. 包含 Cereal 和 EnTT 的头文件
#include <cereal/archives/json.hpp>
#include <entt/entt.hpp>
#include "dmfix_win_utf8.h"
// --- (2) 定义我们的组件 ---
struct Position {
    float x;
    float y;
};

struct Velocity {
    float dx;
    float dy;
};

// 空的标签组件
struct PlayerTag {};

// --- (3) 为 Cereal 提供 serialize 函数 ("粘合代码") ---
// 这是告诉 Cereal 如何处理我们自定义组件的方式

template<typename Archive>
void serialize(Archive &archive, Position &pos) {
    // 按顺序序列化成员变量
    archive(cereal::make_nvp("x", pos.x), cereal::make_nvp("y", pos.y));
}

template<typename Archive>
void serialize(Archive &archive, Velocity &vel) {
    archive(cereal::make_nvp("dx", vel.dx), cereal::make_nvp("dy", vel.dy));
}

// 对于空的组件，需要提供一个空的 serialize 函数
template<typename Archive>
void serialize(Archive &archive, PlayerTag &tag) {
    // 空组件没有成员，所以函数体是空的
}

// 辅助函数：打印 Registry 的状态用于验证
void printRegistryState(const std::string &title, entt::registry &registry) {
    std::cout << "========== " << title << " ==========\n";
    const auto entity_count = registry.storage<entt::entity>().size();
    std::cout << "总实体数: " << entity_count << "\n\n";
    registry.view<entt::entity>().each([&](auto entity) {
        std::cout << "实体 " << static_cast<uint32_t>(entity) << ":\n";
        if(registry.all_of<PlayerTag>(entity)) {
            std::cout << "  - PlayerTag\n";
        }
        if(registry.all_of<Position>(entity)) {
            const auto &pos = registry.get<Position>(entity);
            std::cout << "  - Position{ x: " << pos.x << ", y: " << pos.y << " }\n";
        }
        if(registry.all_of<Velocity>(entity)) {
            const auto &vel = registry.get<Velocity>(entity);
            std::cout << "  - Velocity{ dx: " << vel.dx << ", dy: " << vel.dy << " }\n";
        }
    });
    std::cout << "=======================================\n\n";
}

int main() {
    // === Part 1: 创建并填充源 Registry ===
    entt::registry sourceRegistry;

    auto player = sourceRegistry.create();
    sourceRegistry.emplace<PlayerTag>(player);
    sourceRegistry.emplace<Position>(player, 100.f, 200.f);
    sourceRegistry.emplace<Velocity>(player, 5.f, 0.f);

    auto enemy = sourceRegistry.create();
    sourceRegistry.emplace<Position>(enemy, 450.f, 300.f);

    printRegistryState("初始状态 (Source Registry)", sourceRegistry);

    // === Part 2: 保存状态到内存字符串流 ===
    std::stringstream storage;
    { // 使用花括号确保 outputArchive 在离开作用域时完成所有写入操作
        cereal::JSONOutputArchive outputArchive(storage);

        entt::snapshot{sourceRegistry}
            .get<entt::entity>(outputArchive)
            .get<Position>(outputArchive)
            .get<Velocity>(outputArchive)
            .get<PlayerTag>(outputArchive);
    } // outputArchive 在这里被销毁，数据被刷新到 storage

    std::cout << "\n生成的 JSON 快照:\n"
              << storage.str() << "\n";

    // === Part 3: 加载状态到一个全新的 Registry ===
    entt::registry destinationRegistry;
    {
        // 从字符串流中读取
        cereal::JSONInputArchive inputArchive(storage);

        entt::snapshot_loader{destinationRegistry}
            .get<entt::entity>(inputArchive)
            .get<Position>(inputArchive)
            .get<Velocity>(inputArchive)
            .get<PlayerTag>(inputArchive)
            .orphans(); // orphans() 依然建议调用
    }

    printRegistryState("加载后的状态 (Destination Registry)", destinationRegistry);

    // === Part 4: 验证 ===
    std::cout << "验证: 实体数量是否相等? " << std::boolalpha << (sourceRegistry.storage<entt::entity>().size() == destinationRegistry.storage<entt::entity>().size()) << std::endl;
    const auto &originalPos = sourceRegistry.get<Position>(player);
    const auto &loadedPos = destinationRegistry.get<Position>(player);
    std::cout << "验证: 玩家位置是否一致? " << std::boolalpha << (originalPos.x == loadedPos.x && originalPos.y == loadedPos.y) << std::endl;

    return 0;
}