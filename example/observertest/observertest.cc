#include <iostream>
#include <string>
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
    GameWorldLogger m_world_logger; // 将观察者作为成员，确保其生命周期
    AchievementSystem m_achievement_tracker;

    // 玩家和怪物实体的句柄，方便在不同方法中使用
    entt::entity m_player1;
    entt::entity m_player2;
    entt::entity m_monster_goblin;
    entt::entity m_monster_dragon;

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
        // 连接观察者的处理函数到 dispatcher
        m_dispatcher.sink<PlayerJoinsGameEvent>().connect<&GameWorldLogger::onPlayerJoins>(m_world_logger);
        m_dispatcher.sink<MonsterAppearsEvent>().connect<&GameWorldLogger::onMonsterAppears>(m_world_logger);
        m_dispatcher.sink<PlayerAttacksMonsterEvent>().connect<&GameWorldLogger::onPlayerAttack>(m_world_logger);
        m_dispatcher.sink<MonsterDefeatedEvent>().connect<&GameWorldLogger::onMonsterDefeated>(m_world_logger);
        m_dispatcher.sink<MonsterDefeatedEvent>().connect<&AchievementSystem::onMonsterDefeated>(m_achievement_tracker);
    }

    void initialize_world() {
        std::cout << "--- MMO 世界模拟开始 ---\n\n";

        // 创建玩家实体
        m_player1 = m_registry.create();
        m_registry.emplace<PlayerComponent>(m_player1, "阿尔萨斯", 200);
        m_dispatcher.trigger(PlayerJoinsGameEvent{m_player1, m_registry.get<PlayerComponent>(m_player1).name});

        m_player2 = m_registry.create();
        m_registry.emplace<PlayerComponent>(m_player2, "吉安娜", 150);
        m_dispatcher.trigger(PlayerJoinsGameEvent{m_player2, m_registry.get<PlayerComponent>(m_player2).name});

        std::cout << "\n";

        // 创建怪物实体
        m_monster_goblin = m_registry.create();
        m_registry.emplace<MonsterComponent>(m_monster_goblin, "哥布林工兵", 50);
        m_dispatcher.trigger(MonsterAppearsEvent{m_monster_goblin,
                                                 m_registry.get<MonsterComponent>(m_monster_goblin).type,
                                                 m_registry.get<MonsterComponent>(m_monster_goblin).health});

        m_monster_dragon = m_registry.create();
        m_registry.emplace<MonsterComponent>(m_monster_dragon, "奥妮克希亚", 1000);
        m_dispatcher.trigger(MonsterAppearsEvent{m_monster_dragon,
                                                 m_registry.get<MonsterComponent>(m_monster_dragon).type,
                                                 m_registry.get<MonsterComponent>(m_monster_dragon).health});
        std::cout << "\n";
    }

    void run_combat_scenarios() {
        std::cout << "--- 战斗阶段 ---\n";
        simulate_player_attack_internal(m_player1, m_monster_goblin, 30);
        simulate_player_attack_internal(m_player2, m_monster_goblin, 25); // 哥布林应被击败
        simulate_player_attack_internal(m_player1, m_monster_goblin, 10); // 尝试攻击已死亡的哥布林

        std::cout << "\n";
        simulate_player_attack_internal(m_player1, m_monster_dragon, 70);
        simulate_player_attack_internal(m_player2, m_monster_dragon, 50);
        simulate_player_attack_internal(m_player1, m_monster_dragon, 70);

        if(m_registry.valid(m_monster_dragon) && m_registry.all_of<MonsterComponent>(m_monster_dragon)) {
            int dragon_health_before_final_blow = m_registry.get<MonsterComponent>(m_monster_dragon).health;
            if(dragon_health_before_final_blow > 0) {
                simulate_player_attack_internal(m_player1, m_monster_dragon, dragon_health_before_final_blow + 10);
            }
        }
        std::cout << "\n--- MMO 世界模拟结束 ---\n";
    }
};

int main() {
    // 调用 dmfix_win_utf8.h 中的函数 (如果需要)
    // 例如: dmfix_win_utf8::init();

    GameSimulation game;
    game.initialize_world();
    game.run_combat_scenarios();

    // 调用 dmfix_win_utf8.h 中的清理函数 (如果需要)
    // 例如: dmfix_win_utf8::cleanup();
    return 0;
}
