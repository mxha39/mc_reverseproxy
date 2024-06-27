#ifndef ROUTES_H
#define ROUTES_H
#include <iostream>
#include <cstdint>
#include <nlohmann/json.hpp>
using namespace std;

struct Route {
    string hostname;
    string address;
    uint16_t port;
    uint16_t cache;
};

bool save_routes();
bool load_routes();
Route* findRoute(const string& hostname);
void file_watcher(string filename);

#endif // ROUTES_H
