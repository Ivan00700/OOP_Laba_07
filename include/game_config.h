#pragma once

#include <cstddef>

namespace GameConfig {
// Map size (0..WIDTH-1, 0..HEIGHT-1)
inline constexpr int MAP_WIDTH = 100;
inline constexpr int MAP_HEIGHT = 100;

inline constexpr std::size_t INITIAL_NPC_COUNT = 50;
inline constexpr int GAME_DURATION_SECONDS = 30;
inline constexpr int RENDER_PERIOD_MS = 1000;
inline constexpr int MOVEMENT_TICK_MS = 200;

// Movement & kill distances by type (from assignment table)
inline constexpr int ORK_MOVE_DISTANCE = 20;
inline constexpr int ORK_KILL_DISTANCE = 10;

inline constexpr int WILLIAN_MOVE_DISTANCE = 10; // "Разбойник"
inline constexpr int WILLIAN_KILL_DISTANCE = 10;

inline constexpr int WEREWOLF_MOVE_DISTANCE = 40;
inline constexpr int WEREWOLF_KILL_DISTANCE = 5;
} // namespace GameConfig
