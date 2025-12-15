#include "../include/willian.h"
#include "../include/output.h"
#include <mutex>

Willian::Willian(int x, int y, const std::string& name) 
    : NPC(WillianType, x, y, name) {}

void Willian::print() {
    auto [px, py] = position();
    std::lock_guard<std::mutex> lock(Output::cout_mutex);
    std::cout << "Willian: " << name << " {" << px << ", " << py << "}" << std::endl;
}

void Willian::save(std::ostream& os) {
    os << "Willian ";
    NPC::save(os);
}

void Willian::accept(Visitor& v) {
    v.visit(*this);
}