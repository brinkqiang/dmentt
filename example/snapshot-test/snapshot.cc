#include <entt/entt.hpp>
#include <iostream>
#include <sstream>
#include <string>

// 定义组件
struct Position { float x, y; };
struct Velocity { float dx, dy; };
struct CPlayer {
    int id;
    std::string name;
    float score;

    // 用于输出组件数据
    friend std::ostream& operator<<(std::ostream& os, const CPlayer& player) {
        return os << "Player " << player.id 
                  << " - Name: " << player.name 
                  << ", Score: " << player.score;
    }
};

int main() {
    entt::registry registry;
    std::stringstream archive; // 用内存流模拟存档

    // --- 创建初始实体和组件 ---
    auto player_entity = registry.create();
    registry.emplace<Position>(player_entity, 10.0f, 20.0f);
    registry.emplace<Velocity>(player_entity, 1.0f, -0.5f);
    registry.emplace<CPlayer>(player_entity, 1, "Hero", 100.0f);

    // 打印初始状态
    std::cout << "[Initial State]\n";
    const auto& initial_player = registry.get<CPlayer>(player_entity);
    std::cout << initial_player << "\n\n";

    // --- 保存快照 ---
    entt::snapshot{registry}
        .entities(archive)                   // 保存实体列表
        .component<Position, Velocity, CPlayer>(archive); // 保存组件

    // --- 修改数据以模拟游戏中的变化 ---
    registry.patch<CPlayer>(player_entity, [](auto& player) {
        player.name = "Villain";
        player.score = 666.0f;
    });

    // 打印修改后的状态
    std::cout << "[After Modification]\n";
    const auto& modified_player = registry.get<CPlayer>(player_entity);
    std::cout << modified_player << "\n\n";

    // --- 从存档恢复状态 ---
    entt::snapshot_loader{registry}
        .entities(archive)                   // 恢复实体
        .component<Position, Velocity, CPlayer>(archive) // 恢复组件
        .orphans(); // 销毁存档中未包含的实体

    // --- 验证恢复后的数据 ---
    std::cout << "[After Restoration]\n";
    if (registry.valid(player_entity)) {
        const auto& restored_player = registry.get<CPlayer>(player_entity);
        std::cout << restored_player << "\n";

        // 验证位置是否恢复
        const auto& pos = registry.get<Position>(player_entity);
        std::cout << "Position: (" << pos.x << ", " << pos.y << ")\n";
    } else {
        std::cout << "Entity no longer exists!\n";
    }

    return 0;
}