#include "../include/ork.h"
#include "../include/output.h"
#include <mutex>

Ork::Ork(int x, int y, const std::string& name) 
    : NPC(OrkType, x, y, name) {}

void Ork::print() {
    auto [px, py] = position();
    std::lock_guard<std::mutex> lock(Output::cout_mutex);
    std::cout << "Ork: " << name << " {" << px << ", " << py << "}" << std::endl;
}

void Ork::save(std::ostream& os) {
    os << "Ork ";
    NPC::save(os);
}

void Ork::accept(Visitor& v) {
    v.visit(*this);
}