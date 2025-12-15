#include <iostream>
#include <memory>
#include <mutex>
#include "include/arena.h"
#include "include/game.h"
#include "include/game_config.h"
#include "include/output.h"
#include "include/file_observer.h"
#include "include/console_observer.h"

int main() {
    Arena arena;
    
    // Создаем наблюдателей один раз
    auto file_obs = std::make_shared<FileObserver>("log.txt");
    auto console_obs = std::make_shared<ConsoleObserver>();

    {
        std::lock_guard<std::mutex> lock(Output::cout_mutex);
        std::cout << "Lab 7 - Async NPC Arena" << std::endl;
        std::cout << "Map: " << GameConfig::MAP_WIDTH << "x" << GameConfig::MAP_HEIGHT
                  << ", NPC: " << GameConfig::INITIAL_NPC_COUNT
                  << ", duration: " << GameConfig::GAME_DURATION_SECONDS << "s" << std::endl;
    }

    Game game(arena, file_obs, console_obs);
    game.init_random_npcs(GameConfig::INITIAL_NPC_COUNT);
    game.run();

    return 0;
}