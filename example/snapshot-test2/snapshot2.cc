#include <iostream>
#include <sstream> // Used to store serialized data in memory
#include <string>

// 1. Include Cereal and EnTT headers
#include <cereal/archives/json.hpp>
#include <entt/entt.hpp>
#include "dmfix_win_utf8.h"

// --- (2) Define our components ---
struct Position {
    float x;
    float y;
};

struct Velocity {
    float dx;
    float dy;
};

// Empty tag component
struct PlayerTag {};

// --- (3) Provide serialize functions for Cereal ("glue code") ---
// This is how we tell Cereal how to handle our custom components

template<typename Archive>
void serialize(Archive &archive, Position &pos) {
    // Serialize member variables in order
    archive(cereal::make_nvp("x", pos.x), cereal::make_nvp("y", pos.y));
}

template<typename Archive>
void serialize(Archive &archive, Velocity &vel) {
    archive(cereal::make_nvp("dx", vel.dx), cereal::make_nvp("dy", vel.dy));
}

// For empty components, an empty serialize function must be provided
template<typename Archive>
void serialize(Archive &archive, PlayerTag &tag) {
    // Empty components have no members, so the function body is empty
}

// Helper function: print the state of the Registry for verification
void printRegistryState(const std::string &title, entt::registry &registry) {
    std::cout << "========== " << title << " ==========\n";
    const auto entity_count = registry.storage<entt::entity>().size();
    std::cout << "Total entities: " << entity_count << "\n\n";
    registry.view<entt::entity>().each([&](auto entity) {
        std::cout << "Entity " << static_cast<uint32_t>(entity) << ":\n";
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
    // === Part 1: Create and populate the source Registry ===
    entt::registry sourceRegistry;

    auto player = sourceRegistry.create();
    sourceRegistry.emplace<PlayerTag>(player);
    sourceRegistry.emplace<Position>(player, 100.f, 200.f);
    sourceRegistry.emplace<Velocity>(player, 5.f, 0.f);

    auto enemy = sourceRegistry.create();
    sourceRegistry.emplace<Position>(enemy, 450.f, 300.f);

    printRegistryState("Initial State (Source Registry)", sourceRegistry);

    // === Part 2: Save the state to an in-memory string stream ===
    std::stringstream storage;
    { // Use curly braces to ensure the outputArchive completes all write operations when it goes out of scope
        cereal::JSONOutputArchive outputArchive(storage);

        entt::snapshot{sourceRegistry}
            .get<entt::entity>(outputArchive)
            .get<Position>(outputArchive)
            .get<Velocity>(outputArchive)
            .get<PlayerTag>(outputArchive);
    } // outputArchive is destroyed here, and the data is flushed to storage

    std::cout << "\nGenerated JSON Snapshot:\n"
              << storage.str() << "\n";

    // === Part 3: Load the state into a brand new Registry ===
    entt::registry destinationRegistry;
    {
        // Read from the string stream
        cereal::JSONInputArchive inputArchive(storage);

        entt::snapshot_loader{destinationRegistry}
            .get<entt::entity>(inputArchive)
            .get<Position>(inputArchive)
            .get<Velocity>(inputArchive)
            .get<PlayerTag>(inputArchive)
            .orphans(); // orphans() is still recommended to be called
    }

    printRegistryState("Loaded State (Destination Registry)", destinationRegistry);

    // === Part 4: Verification ===
    std::cout << "Verification: Are the entity counts equal? " << std::boolalpha << (sourceRegistry.storage<entt::entity>().size() == destinationRegistry.storage<entt::entity>().size()) << std::endl;
    const auto &originalPos = sourceRegistry.get<Position>(player);
    const auto &loadedPos = destinationRegistry.get<Position>(player);
    std::cout << "Verification: Is the player position consistent? " << std::boolalpha << (originalPos.x == loadedPos.x && originalPos.y == loadedPos.y) << std::endl;

    return 0;
}