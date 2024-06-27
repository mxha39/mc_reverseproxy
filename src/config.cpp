#include <cstdint>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;
using namespace std;


struct Config {
    uint16_t port;
    Config() : port(25565) {};
};

unique_ptr<Config> config = make_unique<Config>();


void loadConfig(const char* path) {
    json j;
    std::ifstream file("config.json");
    if (!file.is_open()) {
        cerr << "Failed to open config.json" << endl;
        return;
    }
    file >> j;
    file.close();
    (*config).port = j["port"];    
}

void createConfig(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        json j;
        j["port"] = 25565;
        std::ofstream file(path);
        if (!file.is_open()) {
            cerr << "Failed to create config.json" << endl;
            return;
        }
        file << j.dump(4);
        file.close();
    }
}