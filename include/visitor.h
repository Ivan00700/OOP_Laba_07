#pragma once

// Forward declarations
class Ork;
class Willian;
class Werewolf;

class Visitor {
public:
    virtual void visit(Ork& ork) = 0;
    virtual void visit(Willian& willian) = 0;
    virtual void visit(Werewolf& werewolf) = 0;
    virtual ~Visitor() = default;
};