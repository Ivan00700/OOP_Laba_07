#include "../include/game.h"

#include "../include/combat_visitor.h"
#include "../include/factory.h"
#include "../include/game_config.h"
#include "../include/output.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

namespace {
int move_distance_for(NpcType type) {
    switch (type) {
        case OrkType: return GameConfig::ORK_MOVE_DISTANCE;
        case WillianType: return GameConfig::WILLIAN_MOVE_DISTANCE;
        case WerewolfType: return GameConfig::WEREWOLF_MOVE_DISTANCE;
        default: return 0;
    }
}

int kill_distance_for(NpcType type) {
    switch (type) {
        case OrkType: return GameConfig::ORK_KILL_DISTANCE;
        case WillianType: return GameConfig::WILLIAN_KILL_DISTANCE;
        case WerewolfType: return GameConfig::WEREWOLF_KILL_DISTANCE;
        default: return 0;
    }
}

char map_symbol_for(NpcType type) {
    switch (type) {
        case OrkType: return 'O';
        case WillianType: return 'R'; // "Разбойник"
        case WerewolfType: return 'W';
        default: return '?';
    }
}

bool within_distance(const std::pair<int, int>& a, const std::pair<int, int>& b, int distance) {
    const long long dx = static_cast<long long>(a.first) - static_cast<long long>(b.first);
    const long long dy = static_cast<long long>(a.second) - static_cast<long long>(b.second);
    return dx * dx + dy * dy <= static_cast<long long>(distance) * static_cast<long long>(distance);
}

bool can_kill(const std::shared_ptr<NPC>& attacker, const std::shared_ptr<NPC>& defender) {
    CombatVisitor v(defender);
    attacker->accept(v);
    return v.is_success();
}

int roll_d6(std::mt19937& rng) {
    static std::uniform_int_distribution<int> dist(1, 6);
    return dist(rng);
}

struct FightTask {
    std::shared_ptr<NPC> attacker;
    std::shared_ptr<NPC> defender;
};

struct PtrPairHash {
    std::size_t operator()(const std::pair<const NPC*, const NPC*>& p) const noexcept {
        const auto h1 = std::hash<const void*>{}(static_cast<const void*>(p.first));
        const auto h2 = std::hash<const void*>{}(static_cast<const void*>(p.second));
        return h1 ^ (h2 << 1);
    }
};

std::string render_map(const std::vector<std::shared_ptr<NPC>>& npcs, int seconds_left) {
    std::vector<std::string> grid(GameConfig::MAP_HEIGHT, std::string(GameConfig::MAP_WIDTH, '.'));

    std::size_t alive_count = 0;
    for (const auto& npc : npcs) {
        if (!npc->is_alive()) continue;
        ++alive_count;
        auto [x, y] = npc->position();
        if (x >= 0 && x < GameConfig::MAP_WIDTH && y >= 0 && y < GameConfig::MAP_HEIGHT) {
            grid[y][x] = map_symbol_for(npc->type);
        }
    }

    std::ostringstream out;
    out << "Seconds left: " << seconds_left << " | Alive: " << alive_count << "\n";
    for (const auto& row : grid) {
        out << row << "\n";
    }
    return out.str();
}

} // namespace

Game::Game(Arena& arena, std::shared_ptr<Observer> file_observer, std::shared_ptr<Observer> console_observer)
    : arena_(arena), file_observer_(std::move(file_observer)), console_observer_(std::move(console_observer)) {}

void Game::init_random_npcs(std::size_t count) {
    std::random_device rd;
    std::mt19937 rng(rd());

    std::uniform_int_distribution<int> x_dist(0, GameConfig::MAP_WIDTH - 1);
    std::uniform_int_distribution<int> y_dist(0, GameConfig::MAP_HEIGHT - 1);
    std::uniform_int_distribution<int> type_dist(1, 3);

    for (std::size_t i = 0; i < count; ++i) {
        const int x = x_dist(rng);
        const int y = y_dist(rng);
        const int t = type_dist(rng);

        std::string type;
        switch (t) {
            case 1: type = "Ork"; break;
            case 2: type = "Willian"; break;
            case 3: type = "Werewolf"; break;
            default: type = "Ork"; break;
        }

        const std::string name = type + std::string("_") + std::to_string(i);
        auto npc = Factory::CreateNPC(type, name, x, y);
        npc->attach(file_observer_);
        npc->attach(console_observer_);
        arena_.add_npc(npc);
    }
}

void Game::run() {
    std::mutex tasks_mutex;
    std::condition_variable tasks_cv;
    std::queue<FightTask> tasks;
    std::unordered_set<std::pair<const NPC*, const NPC*>, PtrPairHash> pending;

    std::atomic<bool> stop{false};

    //  Fight thread 
    std::thread fight_thread([&]() {
        std::random_device rd;
        std::mt19937 rng(rd());

        while (true) {
            FightTask task;
            {
                std::unique_lock<std::mutex> lock(tasks_mutex);
                tasks_cv.wait(lock, [&]() { return stop.load() || !tasks.empty(); });

                if (tasks.empty()) {
                    if (stop.load()) break;
                    continue;
                }

                task = std::move(tasks.front());
                tasks.pop();

                if (task.attacker && task.defender) {
                    pending.erase({task.attacker.get(), task.defender.get()});
                }
            }

            if (!task.attacker || !task.defender) continue;
            if (!task.attacker->is_alive() || !task.defender->is_alive()) continue;

            if (!can_kill(task.attacker, task.defender)) continue;

            const int attack = roll_d6(rng);
            const int defense = roll_d6(rng);

            if (attack > defense) {
                task.defender->kill();
                task.defender->notify(task.attacker->name + " killed " + task.defender->name +
                                         " (attack=" + std::to_string(attack) +
                                         ", defense=" + std::to_string(defense) + ")",
                                     true);
            }
        }
    });

    std::thread movement_thread([&]() {
        while (!stop.load()) {
            auto snapshot = arena_.npcs_snapshot();

            for (const auto& npc : snapshot) {
                if (!npc->is_alive()) continue;

                const auto my_pos = npc->position();
                std::shared_ptr<NPC> best_target;
                long long best_dist_sq = std::numeric_limits<long long>::max();

                for (const auto& other : snapshot) {
                    if (other == npc) continue;
                    if (!other->is_alive()) continue;

                    if (!can_kill(npc, other)) continue;

                    const auto other_pos = other->position();
                    const long long dx = static_cast<long long>(my_pos.first) - static_cast<long long>(other_pos.first);
                    const long long dy = static_cast<long long>(my_pos.second) - static_cast<long long>(other_pos.second);
                    const long long dist_sq = dx * dx + dy * dy;

                    if (dist_sq < best_dist_sq) {
                        best_dist_sq = dist_sq;
                        best_target = other;
                    }
                }

                if (!best_target) {
                    for (const auto& other : snapshot) {
                        if (other == npc) continue;
                        if (!other->is_alive()) continue;

                        const auto other_pos = other->position();
                        const long long dx = static_cast<long long>(my_pos.first) - static_cast<long long>(other_pos.first);
                        const long long dy = static_cast<long long>(my_pos.second) - static_cast<long long>(other_pos.second);
                        const long long dist_sq = dx * dx + dy * dy;

                        if (dist_sq < best_dist_sq) {
                            best_dist_sq = dist_sq;
                            best_target = other;
                        }
                    }
                }

                if (!best_target) continue;

                const int step = move_distance_for(npc->type);
                if (step <= 0) continue;

                const auto target_pos = best_target->position();
                const double dx = static_cast<double>(target_pos.first - my_pos.first);
                const double dy = static_cast<double>(target_pos.second - my_pos.second);
                const double dist = std::sqrt(dx * dx + dy * dy);
                if (dist <= 0.0) continue;

                int move_x = static_cast<int>(std::lround(static_cast<double>(step) * dx / dist));
                int move_y = static_cast<int>(std::lround(static_cast<double>(step) * dy / dist));

                if (move_x == 0 && move_y == 0) {
                    if (std::abs(dx) >= std::abs(dy)) move_x = (dx > 0) ? 1 : -1;
                    else move_y = (dy > 0) ? 1 : -1;
                }

                int new_x = my_pos.first + move_x;
                int new_y = my_pos.second + move_y;

                new_x = std::clamp(new_x, 0, GameConfig::MAP_WIDTH - 1);
                new_y = std::clamp(new_y, 0, GameConfig::MAP_HEIGHT - 1);

                npc->set_position(new_x, new_y);
            }

            {
                std::lock_guard<std::mutex> lock(tasks_mutex);
                for (const auto& attacker : snapshot) {
                    if (!attacker->is_alive()) continue;

                    const int kill_dist = kill_distance_for(attacker->type);
                    if (kill_dist <= 0) continue;

                    const auto attacker_pos = attacker->position();

                    for (const auto& defender : snapshot) {
                        if (defender == attacker) continue;
                        if (!defender->is_alive()) continue;

                        const auto defender_pos = defender->position();
                        if (!within_distance(attacker_pos, defender_pos, kill_dist)) continue;

                        if (!can_kill(attacker, defender)) continue;

                        const auto key = std::make_pair(attacker.get(), defender.get());
                        if (pending.insert(key).second) {
                            tasks.push(FightTask{attacker, defender});
                        }
                    }
                }
            }
            tasks_cv.notify_one();

            std::this_thread::sleep_for(std::chrono::milliseconds(GameConfig::MOVEMENT_TICK_MS));
        }

        tasks_cv.notify_all();
    });

    const auto start = std::chrono::steady_clock::now();
    const auto end_time = start + std::chrono::seconds(GameConfig::GAME_DURATION_SECONDS);

    while (std::chrono::steady_clock::now() < end_time) {
        const auto now = std::chrono::steady_clock::now();
        const int seconds_left = static_cast<int>(
            std::chrono::duration_cast<std::chrono::seconds>(end_time - now).count());

        auto snapshot = arena_.npcs_snapshot();
        const std::string frame = render_map(snapshot, seconds_left);
        {
            std::lock_guard<std::mutex> lock(Output::cout_mutex);
            std::cout << frame << std::flush;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(GameConfig::RENDER_PERIOD_MS));
    }

    stop.store(true);
    tasks_cv.notify_all();

    movement_thread.join();
    fight_thread.join();

    {
        auto snapshot = arena_.npcs_snapshot();
        std::lock_guard<std::mutex> lock(Output::cout_mutex);
        std::cout << "\n=== Survivors ===\n";
        for (const auto& npc : snapshot) {
            if (!npc->is_alive()) continue;
            auto [x, y] = npc->position();
            std::cout << npc->name << " (";
            switch (npc->type) {
                case OrkType: std::cout << "Ork"; break;
                case WillianType: std::cout << "Willian"; break;
                case WerewolfType: std::cout << "Werewolf"; break;
                default: std::cout << "Unknown"; break;
            }
            std::cout << ") at {" << x << ", " << y << "}\n";
        }
    }
}
