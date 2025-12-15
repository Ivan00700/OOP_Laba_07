#pragma once
#include "visitor.h"
#include "npc.h"
#include <memory>

class CombatVisitor : public Visitor {
    std::shared_ptr<NPC> defender;
    bool success;

public:
    CombatVisitor(std::shared_ptr<NPC> def) : defender(def), success(false) {}

    bool is_success() const { return success; }

    // Логика: Attacker посещает Visitor, Visitor знает про Defender.
    
    // Орк атакует
    void visit(Ork& attacker) override;
    
    // Разбойник атакует
    void visit(Willian& attacker) override;
    
    // Оборотень атакует
    void visit(Werewolf& attacker) override;
};