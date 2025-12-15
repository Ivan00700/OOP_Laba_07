#pragma once
#include <memory>
#include <iostream>
#include "npc.h"

class Factory {
public:
    static std::shared_ptr<NPC> CreateNPC(const std::string& type, const std::string& name, int x, int y);
    static std::shared_ptr<NPC> CreateNPC(std::istream& is);
};