#pragma once
#include "npc.h"

class Willian : public NPC {
public:
    Willian(int x, int y, const std::string& name);
    void print() override;
    void save(std::ostream& os) override;
    void accept(Visitor& v) override;
};