#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <vector>
#include <filesystem>
#include <fstream>
#include "../include/factory.h"
#include "../include/ork.h"
#include "../include/willian.h"
#include "../include/werewolf.h"
#include "../include/combat_visitor.h"
#include "../include/file_observer.h"
#include "../include/observer.h"
#include "../include/arena.h"

// ==========================================
// 1. Тесты Фабрики (Factory Tests) - 8 тестов
// ==========================================

TEST(FactoryTest, CreateOrkCorrectly) {
    auto npc = Factory::CreateNPC("Ork", "Thrall", 10, 20);
    ASSERT_TRUE(npc != nullptr);
    EXPECT_EQ(npc->type, OrkType);
    EXPECT_EQ(npc->name, "Thrall");
    auto [x, y] = npc->position();
    EXPECT_EQ(x, 10);
    EXPECT_EQ(y, 20);
}

TEST(FactoryTest, CreateWillianCorrectly) {
    auto npc = Factory::CreateNPC("Willian", "Robin", 0, 99);
    ASSERT_TRUE(npc != nullptr);
    EXPECT_EQ(npc->type, WillianType);
}

TEST(FactoryTest, CreateWerewolfCorrectly) {
    auto npc = Factory::CreateNPC("Werewolf", "Lucian", 50, 50);
    ASSERT_TRUE(npc != nullptr);
    EXPECT_EQ(npc->type, WerewolfType);
}

TEST(FactoryTest, CaseInsensitiveCreation) {
    // Проверка, что "ork", "ORK", "Ork" работают (если реализовано, 
    // в коде было сравнение ork/Ork. Проверим то, что точно есть)
    auto npc = Factory::CreateNPC("ork", "Small", 1, 1);
    EXPECT_EQ(npc->type, OrkType);
}

TEST(FactoryTest, ThrowOnUnknownType) {
    EXPECT_THROW(Factory::CreateNPC("Dragon", "Smaug", 10, 10), std::runtime_error);
}

TEST(FactoryTest, ThrowOnNegativeCoordinates) {
    EXPECT_THROW(Factory::CreateNPC("Ork", "Bad", -1, 50), std::runtime_error);
    EXPECT_THROW(Factory::CreateNPC("Ork", "Bad", 50, -1), std::runtime_error);
}

TEST(FactoryTest, ThrowOnOutOfBoundsCoordinates) {
    EXPECT_THROW(Factory::CreateNPC("Ork", "Bad", 100, 50), std::runtime_error);
    EXPECT_THROW(Factory::CreateNPC("Ork", "Bad", 50, 100), std::runtime_error);
}

TEST(FactoryTest, CreateFromStream) {
    std::stringstream ss;
    ss << "Ork 10 20 Guldan";
    auto npc = Factory::CreateNPC(ss);
    ASSERT_TRUE(npc != nullptr);
    EXPECT_EQ(npc->type, OrkType);
    auto [x, y] = npc->position();
    EXPECT_EQ(x, 10);
    EXPECT_EQ(y, 20);
    EXPECT_EQ(npc->name, "Guldan");
}

// ==========================================
// 2. Тесты NPC Логики (NPC Logic) - 6 тестов
// ==========================================

TEST(NPCTest, SaveFormatOrk) {
    auto npc = std::make_shared<Ork>(10, 20, "Name");
    std::stringstream ss;
    npc->save(ss);
    // Ожидаем: "Ork 10 20 Name" (плюс перевод строки)
    std::string output;
    std::getline(ss, output);
    EXPECT_EQ(output, "Ork 10 20 Name");
}

TEST(NPCTest, SaveFormatWillian) {
    auto npc = std::make_shared<Willian>(5, 5, "Rob");
    std::stringstream ss;
    npc->save(ss);
    std::string output;
    std::getline(ss, output);
    EXPECT_EQ(output, "Willian 5 5 Rob");
}

TEST(NPCTest, DistanceCalculationSamePoint) {
    auto n1 = std::make_shared<Ork>(10, 10, "A");
    auto n2 = std::make_shared<Ork>(10, 10, "B");
    EXPECT_TRUE(n1->is_close(n2, 0));
    EXPECT_TRUE(n1->is_close(n2, 1));
}

TEST(NPCTest, DistanceCalculationExact) {
    // Треугольник 3-4-5. Расстояние между (0,0) и (3,4) равно 5.
    auto n1 = std::make_shared<Ork>(0, 0, "A");
    auto n2 = std::make_shared<Ork>(3, 4, "B");
    EXPECT_TRUE(n1->is_close(n2, 5));
    EXPECT_FALSE(n1->is_close(n2, 4));
}

TEST(NPCTest, DistanceCalculationFar) {
    auto n1 = std::make_shared<Ork>(0, 0, "A");
    auto n2 = std::make_shared<Ork>(99, 99, "B");
    EXPECT_FALSE(n1->is_close(n2, 50));
}

TEST(NPCTest, SelfDistanceIgnored) {
    // is_close должен возвращать false, если сравниваем объект сам с собой
    // (реализация: if (this == other.get()) return false;)
    auto n1 = std::make_shared<Ork>(0, 0, "A");
    EXPECT_FALSE(n1->is_close(n1, 100));
}

TEST(NPCTest, AliveDefaultTrue) {
    auto n1 = std::make_shared<Ork>(0, 0, "A");
    EXPECT_TRUE(n1->is_alive());
}

TEST(NPCTest, KillMakesDead) {
    auto n1 = std::make_shared<Ork>(0, 0, "A");
    n1->kill();
    EXPECT_FALSE(n1->is_alive());
}

TEST(NPCTest, PositionSetGet) {
    auto n1 = std::make_shared<Ork>(0, 0, "A");
    n1->set_position(12, 34);
    auto [x, y] = n1->position();
    EXPECT_EQ(x, 12);
    EXPECT_EQ(y, 34);
}

// ==========================================
// 3. Тесты Боевой Системы (Combat Matrix) - 9 тестов
// ==========================================
// Матрица 3x3: Кто кого атакует.
// Helper function
bool fight(std::shared_ptr<NPC> attacker, std::shared_ptr<NPC> defender) {
    CombatVisitor v(defender);
    attacker->accept(v);
    return v.is_success();
}

TEST(CombatTest, Ork_Kills_Willian) {
    auto att = std::make_shared<Ork>(0,0,"A");
    auto def = std::make_shared<Willian>(0,0,"D");
    EXPECT_TRUE(fight(att, def));
}

TEST(CombatTest, Ork_DoesNotKill_Ork) {
    auto att = std::make_shared<Ork>(0,0,"A");
    auto def = std::make_shared<Ork>(0,0,"D");
    EXPECT_FALSE(fight(att, def));
}

TEST(CombatTest, Ork_DoesNotKill_Werewolf) {
    auto att = std::make_shared<Ork>(0,0,"A");
    auto def = std::make_shared<Werewolf>(0,0,"D");
    EXPECT_FALSE(fight(att, def));
}

TEST(CombatTest, Willian_Kills_Werewolf) {
    auto att = std::make_shared<Willian>(0,0,"A");
    auto def = std::make_shared<Werewolf>(0,0,"D");
    EXPECT_TRUE(fight(att, def));
}

TEST(CombatTest, Willian_DoesNotKill_Willian) {
    auto att = std::make_shared<Willian>(0,0,"A");
    auto def = std::make_shared<Willian>(0,0,"D");
    EXPECT_FALSE(fight(att, def));
}

TEST(CombatTest, Willian_DoesNotKill_Ork) {
    auto att = std::make_shared<Willian>(0,0,"A");
    auto def = std::make_shared<Ork>(0,0,"D");
    EXPECT_FALSE(fight(att, def));
}

TEST(CombatTest, Werewolf_Kills_Willian) {
    auto att = std::make_shared<Werewolf>(0,0,"A");
    auto def = std::make_shared<Willian>(0,0,"D");
    EXPECT_TRUE(fight(att, def)); // Взаимное уничтожение по условию задачи (Оборотень убивает разбойника)
}

TEST(CombatTest, Werewolf_DoesNotKill_Werewolf) {
    auto att = std::make_shared<Werewolf>(0,0,"A");
    auto def = std::make_shared<Werewolf>(0,0,"D");
    EXPECT_FALSE(fight(att, def));
}

TEST(CombatTest, Werewolf_DoesNotKill_Ork) {
    auto att = std::make_shared<Werewolf>(0,0,"A");
    auto def = std::make_shared<Ork>(0,0,"D");
    EXPECT_FALSE(fight(att, def));
}

// ==========================================
// 4. Тесты Наблюдателя (Observer) - 5 тестов
// ==========================================

class TestObserver : public Observer {
public:
    std::vector<std::string> messages;
    void update(const std::string& message) override {
        messages.push_back(message);
    }
};

TEST(ObserverTest, SingleObserverNotify) {
    auto npc = std::make_shared<Ork>(0,0,"O");
    auto obs = std::make_shared<TestObserver>();
    npc->attach(obs);
    npc->notify("Event 1");
    
    ASSERT_EQ(obs->messages.size(), 1);
    EXPECT_EQ(obs->messages[0], "Event 1");
}

TEST(ObserverTest, MultipleObservers) {
    auto npc = std::make_shared<Ork>(0,0,"O");
    auto obs1 = std::make_shared<TestObserver>();
    auto obs2 = std::make_shared<TestObserver>();
    
    npc->attach(obs1);
    npc->attach(obs2);
    npc->notify("Event 2");

    EXPECT_EQ(obs1->messages.size(), 1);
    EXPECT_EQ(obs2->messages.size(), 1);
}

TEST(ObserverTest, MultipleNotifications) {
    auto npc = std::make_shared<Ork>(0,0,"O");
    auto obs = std::make_shared<TestObserver>();
    npc->attach(obs);
    npc->notify("A");
    npc->notify("B");
    
    ASSERT_EQ(obs->messages.size(), 2);
    EXPECT_EQ(obs->messages[1], "B");
}

TEST(ObserverTest, FileObserverIntegration) {
    // Тест создания реального файла
    std::string filename = "test_log.txt";
    // Очистка перед тестом
    if (std::filesystem::exists(filename)) std::filesystem::remove(filename);

    {
        auto file_obs = std::make_shared<FileObserver>(filename);
        file_obs->update("Test Log Entry");
        // Деструктор закроет файл (или flush при записи)
    }

    // Проверка
    std::ifstream fs(filename);
    ASSERT_TRUE(fs.is_open());
    std::string line;
    std::getline(fs, line);
    EXPECT_EQ(line, "Test Log Entry");
    
    // Чистка
    fs.close();
    std::filesystem::remove(filename);
}

// ==========================================
// 5. Тесты Арены (Integration) - 5 тестов
// ==========================================

TEST(ArenaTest, AddAndPrintDoesNotCrash) {
    Arena arena;
    auto npc = Factory::CreateNPC("Ork", "T", 0, 0);
    arena.add_npc(npc);
    
    // Захват stdout (продвинутая техника, но здесь просто проверим, что не падает)
    testing::internal::CaptureStdout();
    arena.print();
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_NE(output.find("Ork: T"), std::string::npos);
}

TEST(ArenaTest, SaveAndLoadEmpty) {
    Arena arena;
    std::string fname = "test_empty_arena.txt";
    arena.save(fname);
    
    Arena arena2;
    // Нужно передать observer'ы, так как сигнатура требует
    auto obs = std::make_shared<TestObserver>();
    arena2.load(fname, obs, obs);
    
    // Проверяем, что файл создался и он маленький
    EXPECT_TRUE(std::filesystem::exists(fname));
    std::filesystem::remove(fname);
}

TEST(ArenaTest, FullCycleSaveLoad) {
    Arena arena;
    arena.add_npc(Factory::CreateNPC("Ork", "O1", 10, 10));
    arena.add_npc(Factory::CreateNPC("Willian", "W1", 20, 20));
    
    std::string fname = "test_full_arena.txt";
    arena.save(fname);
    
    Arena arena2;
    auto obs = std::make_shared<TestObserver>();
    arena2.load(fname, obs, obs);
    
    // При загрузке мы не можем легко проверить внутренний вектор (он private),
    // но можем проверить вывод print
    testing::internal::CaptureStdout();
    arena2.print();
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_NE(output.find("O1"), std::string::npos);
    EXPECT_NE(output.find("W1"), std::string::npos);
    
    std::filesystem::remove(fname);
}

TEST(ArenaTest, FightLogicIntegration) {
    // Интеграционный тест: добавить врагов, запустить бой, проверить уведомление
    Arena arena;
    auto attacker = std::make_shared<Ork>(0, 0, "Killer");
    auto victim = std::make_shared<Willian>(0, 0, "Victim");
    
    // Прикрепляем шпиона
    auto spy = std::make_shared<TestObserver>();
    attacker->attach(spy);
    victim->attach(spy);
    
    arena.add_npc(attacker);
    arena.add_npc(victim);
    
    // Бой на дистанции 10 (достаточно)
    // Захватываем вывод, чтобы не мусорить в консоль теста
    testing::internal::CaptureStdout();
    arena.fight(10); 
    testing::internal::GetCapturedStdout();
    
    // Проверяем, что Observer получил сообщение об убийстве
    ASSERT_FALSE(spy->messages.empty());
    bool kill_msg_found = false;
    for(const auto& msg : spy->messages) {
        if(msg.find("killed") != std::string::npos) {
            kill_msg_found = true;
            break;
        }
    }
    EXPECT_TRUE(kill_msg_found);
}

TEST(ArenaTest, FightNoRange) {
    Arena arena;
    auto n1 = std::make_shared<Ork>(0, 0, "A");
    auto n2 = std::make_shared<Willian>(99, 99, "B");
    
    auto spy = std::make_shared<TestObserver>();
    n1->attach(spy);
    n2->attach(spy);
    
    arena.add_npc(n1);
    arena.add_npc(n2);
    
    testing::internal::CaptureStdout();
    arena.fight(10);
    testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(spy->messages.empty());
}

TEST(ArenaTest, SnapshotReturnsCopy) {
    Arena arena;
    arena.add_npc(std::make_shared<Ork>(0, 0, "A"));
    arena.add_npc(std::make_shared<Willian>(1, 1, "B"));

    auto snap1 = arena.npcs_snapshot();
    EXPECT_EQ(snap1.size(), 2u);

    // Modifying snapshot should not affect arena
    snap1.clear();
    auto snap2 = arena.npcs_snapshot();
    EXPECT_EQ(snap2.size(), 2u);
}