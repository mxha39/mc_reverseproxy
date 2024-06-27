#include "header/routes.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <mutex>
#include <vector>
#include <fstream>

#include <sys/inotify.h>
#include <unistd.h>
#include <limits.h>

using namespace std;
using json = nlohmann::json;

mutex routesMux;
vector<Route> routes = {};

Route* findRoute(const string& hostname) {
    lock_guard<mutex> lock(routesMux);
    for (auto& route : routes) {
        if (route.hostname == hostname) {
            return &route;
        }
    }
    return nullptr;
}

bool save_routes() {
    json j;
    {
        lock_guard<mutex> lock(routesMux);
        for (auto& route : routes) {
            json routeJson = {
                {"address", route.address},
                {"port", route.port},
                {"cache", route.cache}
            };
            j[route.hostname] = routeJson;
        }
    }

    ofstream file("routes.json");
    if (!file.is_open()) {
        cerr << "Failed to open routes.json" << endl;
        return false;
    }
    file << j.dump(4);
    file.close();
    return true;
}

bool load_routes() {
    json j;
    ifstream file("routes.json");
    if (!file.is_open()) {
        cerr << "Failed to open routes.json" << endl;
        save_routes();
        return false;
    }
    try {
        file >> j;
    } catch (const std::exception& e) {
        cerr << "Failed to parse JSON: " << e.what() << endl;
        file.close();
        return false;
    }
    file.close();

    // lock_guard<mutex> lock(routesMux);
    routesMux.lock();
    routes.clear();
    
    for (auto it = j.begin(); it != j.end(); ++it) {
        try {
            Route route;
            route.hostname = it.key();
            route.address = it.value()["address"];
            route.port = it.value()["port"];
            route.cache = it.value()["cache"];
            routes.push_back(route);
            cout << "Loaded route: " << route.hostname << " -> " << route.address << ":" << route.port << endl;
        } catch (const std::exception& e) {
            cerr << "Error while processing JSON: " << e.what() << endl;
            routesMux.unlock();
            return false;
        }
    }
    routesMux.unlock();
    return true;
}



// linux only, todo: add windows support, clear cachedservers
void file_watcher(string filename) {
    int inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd < 0) {
        perror("inotify_init1");
        return;
    }

    int watch_fd = inotify_add_watch(inotify_fd, filename.data(), IN_MODIFY);
    if (watch_fd < 0) {
        perror("inotify_add_watch");
        close(inotify_fd);
        return;
    }

    const size_t event_size = sizeof(struct inotify_event);
    const size_t buf_len = 1024 * (event_size + NAME_MAX + 1);
    char buf[buf_len];

    while (true) {
        int length = read(inotify_fd, buf, buf_len);
        if (length < 0 && errno != EAGAIN) {
            perror("read");
            break;
        }

        if (length <= 0) {
            // No events available, sleep for a short time to avoid busy-waiting
            usleep(500000); // Sleep for 500ms
            continue;
        }

        for (int i = 0; i < length; i += event_size + buf[i]) {
            struct inotify_event* event = reinterpret_cast<struct inotify_event*>(&buf[i]);

            if (event->mask & IN_MODIFY) {
                cout << endl;
                load_routes();
                cout << "Routes reloaded" << endl;
            }
        }
    }

    inotify_rm_watch(inotify_fd, watch_fd);
    close(inotify_fd);

    return;
}
