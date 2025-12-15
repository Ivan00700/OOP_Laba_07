#pragma once

#include "arena.h"
#include "observer.h"
#include <cstddef>
#include <memory>

class Game {
public:
    Game(Arena& arena, std::shared_ptr<Observer> file_observer, std::shared_ptr<Observer> console_observer);

    void init_random_npcs(std::size_t count);
    void run();

private:
    Arena& arena_;
    std::shared_ptr<Observer> file_observer_;
    std::shared_ptr<Observer> console_observer_;
};
