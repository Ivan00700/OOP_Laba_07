#pragma once
#include "observer.h"
#include <fstream>
#include <iostream>

class FileObserver : public Observer {
    std::string filename;
public:
    FileObserver(const std::string& fname) : filename(fname) {
        std::ofstream fs(filename, std::ios::trunc);
        fs.close();
    }
    void update(const std::string& message) override {
        std::ofstream fs(filename, std::ios::app);
        if (fs.is_open()) {
            fs << message << std::endl;
        }
    }
};