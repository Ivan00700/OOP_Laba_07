#include "../include/npc.h"
#include <cmath>

NPC::NPC(NpcType t, int _x, int _y, const std::string& _name) 
    : type(t), x(_x), y(_y), name(_name) {}

void NPC::attach(std::shared_ptr<Observer> observer) {
    observers.push_back(observer);
}

void NPC::notify(const std::string& message, bool is_kill_event) {
    for (auto& o : observers) {
        o->update(message);
    }
}

void NPC::save(std::ostream& os) {
    std::lock_guard<std::mutex> lock(state_mutex);
    os << x << " " << y << " " << name << std::endl;
}

bool NPC::is_close(const std::shared_ptr<NPC>& other, size_t distance) {
    if (this == other.get()) return false;
    int x1, y1, x2, y2;
    {
        std::lock_guard<std::mutex> lock(state_mutex);
        x1 = x;
        y1 = y;
    }
    {
        std::lock_guard<std::mutex> lock(other->state_mutex);
        x2 = other->x;
        y2 = other->y;
    }
    auto dist_sq = std::pow(x1 - x2, 2) + std::pow(y1 - y2, 2);
    return dist_sq <= std::pow(static_cast<double>(distance), 2);
}

bool NPC::is_alive() const {
    std::lock_guard<std::mutex> lock(state_mutex);
    return alive;
}

void NPC::kill() {
    std::lock_guard<std::mutex> lock(state_mutex);
    alive = false;
}

std::pair<int, int> NPC::position() const {
    std::lock_guard<std::mutex> lock(state_mutex);
    return {x, y};
}

void NPC::set_position(int new_x, int new_y) {
    std::lock_guard<std::mutex> lock(state_mutex);
    x = new_x;
    y = new_y;
}