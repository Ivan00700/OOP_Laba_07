#pragma once
#include "npc.h"

class Werewolf : public NPC {
public:
    Werewolf(int x, int y, const std::string& name);
    void print() override;
    void save(std::ostream& os) override;
    void accept(Visitor& v) override;
};