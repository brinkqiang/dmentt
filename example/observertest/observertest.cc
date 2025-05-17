#include <iostream>
#include <string>
#include <vector> // 添加 vector 头文件
#include <entt/entt.hpp>
#include "dmfix_win_utf8.h" // 用户提供的头文件

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
        if(event.defeated_monster_type == "巨龙") {
            std::cout << "[成就系统] " << event.victor_player_name << " 完成了“屠龙伟业”成就！\n";
        } else if(event.defeated_monster_type == "奥妮克希亚") {
            std::cout << "[成就系统] " << event.victor_player_name << " 完成了“屠龙伟业”成就！(奥妮克希亚)\n";
        }
    }
};

// --- 游戏模拟类 ---
class GameSimulation {
private:
    entt::registry m_registry;
    entt::dispatcher &m_dispatcher;
    GameWorldLogger m_world_logger;
    AchievementSystem m_achievement_tracker;

    // 使用 vector 管理实体
    std::vector<entt::entity> m_players;
    std::vector<entt::entity> m_monsters;

    void simulate_player_attack_internal(entt::entity player_entity, entt::entity monster_entity, int damage) {
        if(!m_registry.valid(player_entity) || !m_registry.valid(monster_entity)) {
            std::cerr << "错误：无效的玩家或怪物实体。\n";
            return;
        }

        auto *player_comp = m_registry.try_get<PlayerComponent>(player_entity);
        auto *monster_comp = m_registry.try_get<MonsterComponent>(monster_entity);

        if(!player_comp || !monster_comp) {
            std::cerr << "错误：玩家或怪物组件缺失。\n";
            return;
        }

        if(monster_comp->health <= 0) {
            m_dispatcher.trigger(PlayerAttacksMonsterEvent{
                player_entity, player_comp->name,
                monster_entity, monster_comp->type,
                0, 0});
            return;
        }

        monster_comp->health -= damage;
        int remaining_health = monster_comp->health < 0 ? 0 : monster_comp->health;

        m_dispatcher.trigger(PlayerAttacksMonsterEvent{
            player_entity, player_comp->name,
            monster_entity, monster_comp->type,
            damage, remaining_health});

        if(remaining_health <= 0) {
            m_dispatcher.trigger(MonsterDefeatedEvent{
                monster_entity, monster_comp->type,
                player_entity, player_comp->name});
        }
    }

public:
    GameSimulation()
        : m_dispatcher(m_registry.ctx().emplace<entt::dispatcher>()) {
        m_dispatcher.sink<PlayerJoinsGameEvent>().connect<&GameWorldLogger::onPlayerJoins>(m_world_logger);
        m_dispatcher.sink<MonsterAppearsEvent>().connect<&GameWorldLogger::onMonsterAppears>(m_world_logger);
        m_dispatcher.sink<PlayerAttacksMonsterEvent>().connect<&GameWorldLogger::onPlayerAttack>(m_world_logger);
        m_dispatcher.sink<MonsterDefeatedEvent>().connect<&GameWorldLogger::onMonsterDefeated>(m_world_logger);
        m_dispatcher.sink<MonsterDefeatedEvent>().connect<&AchievementSystem::onMonsterDefeated>(m_achievement_tracker);
    }

    void initialize_world() {
        std::cout << "--- MMO 世界模拟开始 ---\n\n";
        m_players.clear(); // 清空之前的实体（如果需要重复初始化）
        m_monsters.clear();

        // 创建玩家实体
        entt::entity player1_entity = m_registry.create();
        m_registry.emplace<PlayerComponent>(player1_entity, "阿尔萨斯", 200);
        m_players.push_back(player1_entity);
        m_dispatcher.trigger(PlayerJoinsGameEvent{player1_entity, m_registry.get<PlayerComponent>(player1_entity).name});

        entt::entity player2_entity = m_registry.create();
        m_registry.emplace<PlayerComponent>(player2_entity, "吉安娜", 150);
        m_players.push_back(player2_entity);
        m_dispatcher.trigger(PlayerJoinsGameEvent{player2_entity, m_registry.get<PlayerComponent>(player2_entity).name});

        std::cout << "\n";

        // 创建怪物实体
        entt::entity goblin_entity = m_registry.create();
        m_registry.emplace<MonsterComponent>(goblin_entity, "哥布林工兵", 50);
        m_monsters.push_back(goblin_entity);
        m_dispatcher.trigger(MonsterAppearsEvent{goblin_entity,
                                                 m_registry.get<MonsterComponent>(goblin_entity).type,
                                                 m_registry.get<MonsterComponent>(goblin_entity).health});

        entt::entity dragon_entity = m_registry.create();
        m_registry.emplace<MonsterComponent>(dragon_entity, "奥妮克希亚", 1000);
        m_monsters.push_back(dragon_entity);
        m_dispatcher.trigger(MonsterAppearsEvent{dragon_entity,
                                                 m_registry.get<MonsterComponent>(dragon_entity).type,
                                                 m_registry.get<MonsterComponent>(dragon_entity).health});
        std::cout << "\n";
    }

    void run_combat_scenarios() {
        std::cout << "--- 战斗阶段 ---\n";

        // 确保有足够的玩家和怪物实体用于模拟
        if(m_players.size() < 2 || m_monsters.size() < 2) {
            std::cerr << "错误：玩家或怪物数量不足，无法执行预设的战斗场景。\n";
            return;
        }

        entt::entity player1 = m_players[0];
        entt::entity player2 = m_players[1];
        entt::entity goblin = m_monsters[0];
        entt::entity dragon = m_monsters[1];

        simulate_player_attack_internal(player1, goblin, 30);
        simulate_player_attack_internal(player2, goblin, 25);
        simulate_player_attack_internal(player1, goblin, 10);

        std::cout << "\n";
        simulate_player_attack_internal(player1, dragon, 70);
        simulate_player_attack_internal(player2, dragon, 50);
        simulate_player_attack_internal(player1, dragon, 70);

        if(m_registry.valid(dragon) && m_registry.all_of<MonsterComponent>(dragon)) {
            auto &dragon_comp = m_registry.get<MonsterComponent>(dragon); // 获取引用以检查 health
            if(dragon_comp.health > 0) {
                simulate_player_attack_internal(player1, dragon, dragon_comp.health + 10);
            }
        }
        std::cout << "\n--- MMO 世界模拟结束 ---\n";
    }
};

int main() {
    GameSimulation game;
    game.initialize_world();
    game.run_combat_scenarios();

    return 0;
}
