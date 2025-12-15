cmake_minimum_required(VERSION 3.14)
project(Laba_06)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# 1. Собираем библиотеку с логикой (БЕЗ main.cpp)
# Перечисляем все .cpp файлы из папки src
set(SOURCES
    src/npc.cpp
    src/ork.cpp
    src/willian.cpp
    src/werewolf.cpp
    src/factory.cpp
    src/combat_visitor.cpp
    src/arena.cpp
  src/game.cpp
)

add_library(core_lib ${SOURCES})

# 2. Основная программа (Редактор)
add_executable(dungeon_editor main.cpp)
target_link_libraries(dungeon_editor core_lib)

# 3. Подключение GoogleTest (автоматическое скачивание)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 4. Исполняемый файл для тестов
enable_testing()

add_executable(unit_tests tests/tests.cpp)
target_link_libraries(unit_tests core_lib gtest_main)

include(GoogleTest)
gtest_discover_tests(unit_tests)