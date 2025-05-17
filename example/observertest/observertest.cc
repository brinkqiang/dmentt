#include <iostream>
#include <string>
#include <entt/entt.hpp>
#include "dmfix_win_utf8.h"

// --- 组件定义 ---
struct PlayerComponent {
    std::string name;
    int health;
};

struct MonsterComponent {
    std::string type;
    int health;
};

// --- 事件定义 ---
struct PlayerJoinsGameEvent {
    entt::entity player_entity;
    std::string player_name;
};

struct MonsterAppearsEvent {
    entt::entity monster_entity;
    std::string monster_type;
    int initial_health;
};

struct PlayerAttacksMonsterEvent {
    entt::entity attacker_player_entity;
    std::string attacker_player_name;
    entt::entity target_monster_entity;
    std::string target_monster_type;
    int damage_dealt;
    int monster_remaining_health;
};

struct MonsterDefeatedEvent {
    entt::entity defeated_monster_entity;
    std::string defeated_monster_type;
    entt::entity victor_player_entity;
    std::string victor_player_name;
};

// --- 观察者类 ---

// 游戏世界记录器
class GameWorldLogger {
public:
    void onPlayerJoins(const PlayerJoinsGameEvent &event) {
        std::cout << "[世界日志] 玩家 '" << event.player_name << "' (ID: " << static_cast<uint32_t>(event.player_entity) << ") 加入了游戏。\n";
    }

    void onMonsterAppears(const MonsterAppearsEvent &event) {
        std::cout << "[世界日志] 一只 " << event.monster_type << " (ID: " << static_cast<uint32_t>(event.monster_entity) << ") 出现了，生命值: " << event.initial_health << "。\n";
    }

    void onPlayerAttack(const PlayerAttacksMonsterEvent &event) {
        std::cout << "[战斗日志] " << event.attacker_player_name << " 攻击了 " << event.target_monster_type
                  << "，造成了 " << event.damage_dealt << " 点伤害。"
                  << event.target_monster_type << " 剩余生命值: " << event.monster_remaining_health << "。\n";
    }

    void onMonsterDefeated(const MonsterDefeatedEvent &event) {
        std::cout << "[战斗日志] " << event.defeated_monster_type << " 被 " << event.victor_player_name << " 击败了！\n";
    }
};

// 成就系统
class AchievementSystem {
public:
    void onMonsterDefeated(const MonsterDefeatedEvent &event) {
        std::cout << "[成就系统] " << event.victor_player_name << " 因击败 " << event.defeated_monster_type << " 获得了成就点！\n";
        if(event.defeated_monster_type == "巨龙") { // 注意：这里原为 "巨龙", 示例中怪物名为 "奥妮克希亚"
            std::cout << "[成就系统] " << event.victor_player_name << " 完成了“屠龙伟业”成就！\n";
        } else if(event.defeated_monster_type == "奥妮克希亚") { // 为匹配示例添加此条件
            std::cout << "[成就系统] " << event.victor_player_name << " 完成了“屠龙伟业”成就！(奥妮克希亚)\n";
        }
    }
};

// --- 辅助函数和游戏逻辑 ---

// 模拟玩家攻击怪物的函数
void simulate_player_attack(entt::registry &registry, entt::dispatcher &dispatcher,
                            entt::entity player_entity, entt::entity monster_entity, int damage) {
    if(!registry.valid(player_entity) || !registry.valid(monster_entity)) {
        std::cerr << "错误：无效的玩家或怪物实体。\n";
        return;
    }

    auto *player_comp = registry.try_get<PlayerComponent>(player_entity);
    auto *monster_comp = registry.try_get<MonsterComponent>(monster_entity);

    if(!player_comp || !monster_comp) {
        std::cerr << "错误：玩家或怪物组件缺失。\n";
        return;
    }

    if(monster_comp->health <= 0) {
        // 允许鞭尸日志，但不实际扣血或触发二次死亡
        // std::cout << monster_comp->type << " 已经被击败了，无法再次对其造成伤害。\n";
        // 触发攻击事件，但生命值不变或为0
        dispatcher.trigger(PlayerAttacksMonsterEvent{
            player_entity, player_comp->name,
            monster_entity, monster_comp->type,
            0, // 伤害为0，因为已被击败
            0  // 剩余生命值为0
        });
        return;
    }

    monster_comp->health -= damage;
    int remaining_health = monster_comp->health;

    if(remaining_health < 0) {
        remaining_health = 0;
    }

    dispatcher.trigger(PlayerAttacksMonsterEvent{
        player_entity, player_comp->name,
        monster_entity, monster_comp->type,
        damage, remaining_health});

    if(remaining_health <= 0) {
        dispatcher.trigger(MonsterDefeatedEvent{
            monster_entity, monster_comp->type,
            player_entity, player_comp->name});
        // 实际游戏中，可能需要从 registry 中移除怪物实体或标记为死亡
        // registry.destroy(monster_entity); // 例如
    }
}

int main() {
    entt::registry registry;
    auto &dispatcher = registry.ctx().emplace<entt::dispatcher>();

    // 创建观察者实例
    GameWorldLogger world_logger;
    AchievementSystem achievement_tracker;

    // 连接观察者的处理函数到 dispatcher
    dispatcher.sink<PlayerJoinsGameEvent>().connect<&GameWorldLogger::onPlayerJoins>(world_logger);
    dispatcher.sink<MonsterAppearsEvent>().connect<&GameWorldLogger::onMonsterAppears>(world_logger);
    dispatcher.sink<PlayerAttacksMonsterEvent>().connect<&GameWorldLogger::onPlayerAttack>(world_logger);
    dispatcher.sink<MonsterDefeatedEvent>().connect<&GameWorldLogger::onMonsterDefeated>(world_logger);

    dispatcher.sink<MonsterDefeatedEvent>().connect<&AchievementSystem::onMonsterDefeated>(achievement_tracker);

    std::cout << "--- MMO 世界模拟开始 ---\n\n";

    // 创建玩家实体
    auto player1 = registry.create();
    registry.emplace<PlayerComponent>(player1, "阿尔萨斯", 200);
    dispatcher.trigger(PlayerJoinsGameEvent{player1, registry.get<PlayerComponent>(player1).name});

    auto player2 = registry.create();
    registry.emplace<PlayerComponent>(player2, "吉安娜", 150);
    dispatcher.trigger(PlayerJoinsGameEvent{player2, registry.get<PlayerComponent>(player2).name});

    std::cout << "\n";

    // 创建怪物实体
    auto monster_goblin = registry.create();
    registry.emplace<MonsterComponent>(monster_goblin, "哥布林工兵", 50);
    dispatcher.trigger(MonsterAppearsEvent{monster_goblin,
                                           registry.get<MonsterComponent>(monster_goblin).type,
                                           registry.get<MonsterComponent>(monster_goblin).health});

    auto monster_dragon = registry.create();
    registry.emplace<MonsterComponent>(monster_dragon, "奥妮克希亚", 1000);
    dispatcher.trigger(MonsterAppearsEvent{monster_dragon,
                                           registry.get<MonsterComponent>(monster_dragon).type,
                                           registry.get<MonsterComponent>(monster_dragon).health});
    std::cout << "\n";

    // 模拟战斗
    std::cout << "--- 战斗阶段 ---\n";
    simulate_player_attack(registry, dispatcher, player1, monster_goblin, 30);
    simulate_player_attack(registry, dispatcher, player2, monster_goblin, 25); // 哥布林应被击败
    simulate_player_attack(registry, dispatcher, player1, monster_goblin, 10); // 尝试攻击已死亡的哥布林

    std::cout << "\n";
    simulate_player_attack(registry, dispatcher, player1, monster_dragon, 70);
    simulate_player_attack(registry, dispatcher, player2, monster_dragon, 50);
    simulate_player_attack(registry, dispatcher, player1, monster_dragon, 70);

    // ... 更多攻击直到巨龙被击败
    // 确保巨龙有足够生命值被多次攻击
    if(registry.valid(monster_dragon) && registry.all_of<MonsterComponent>(monster_dragon)) {
        int dragon_health_before_final_blow = registry.get<MonsterComponent>(monster_dragon).health;
        if(dragon_health_before_final_blow > 0) {                                                                        // 仅当巨龙还活着时才进行最后一击
            simulate_player_attack(registry, dispatcher, player1, monster_dragon, dragon_health_before_final_blow + 10); // 确保击败
        }
    }

    std::cout << "\n--- MMO 世界模拟结束 ---\n";

    return 0;
}
