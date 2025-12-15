#include "../include/combat_visitor.h"
#include "../include/ork.h"
#include "../include/willian.h"
#include "../include/werewolf.h"

// Разбойник убивает оборотней
// Оборотень убивает разбойника
// Орк убивает разбойника

void CombatVisitor::visit(Ork& attacker) {
    // Орк убивает разбойника
    if (defender->type == WillianType) {
        success = true;
    }
}

void CombatVisitor::visit(Willian& attacker) {
    // Разбойник убивает оборотней
    if (defender->type == WerewolfType) {
        success = true;
    }
}

void CombatVisitor::visit(Werewolf& attacker) {
    // Оборотень убивает разбойника
    if (defender->type == WillianType) {
        success = true;
    }
}