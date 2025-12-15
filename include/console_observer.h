#pragma once
#include "observer.h"
#include <iostream>
#include <mutex>
#include "output.h"

class ConsoleObserver : public Observer {
public:
    void update(const std::string& message) override {
        std::lock_guard<std::mutex> lock(Output::cout_mutex);
        std::cout << "[Console Log]: " << message << std::endl;
    }
};