#include "../include/werewolf.h"
#include "../include/output.h"
#include <mutex>

Werewolf::Werewolf(int x, int y, const std::string& name) 
    : NPC(WerewolfType, x, y, name) {}

void Werewolf::print() {
    auto [px, py] = position();
    std::lock_guard<std::mutex> lock(Output::cout_mutex);
    std::cout << "Werewolf: " << name << " {" << px << ", " << py << "}" << std::endl;
}

void Werewolf::save(std::ostream& os) {
    os << "Werewolf ";
    NPC::save(os);
}

void Werewolf::accept(Visitor& v) {
    v.visit(*this);
}