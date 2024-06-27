#include "header/caching.h"
#include <mutex>
#include <iostream>
#include "header/routes.h"
#include "header/Packet.h"
#include "header/utils.h"
#include <thread>
#include <chrono> 


vector<shared_ptr<CachedServer>> cached_servers = {};
mutex cached_servers_mutex;


void clearExpiredServers(){
    while(true) {
        this_thread::sleep_for(chrono::seconds(120));
        cached_servers_mutex.lock();
        for (int i = 0; i < cached_servers.size(); i++){
            if (cached_servers[i]->isExpired()){
                cached_servers.erase(cached_servers.begin() + i);
            }
        }
        cached_servers_mutex.unlock();
    }
}

CachedServer::CachedServer(){
    this->hostname = "";
    this->time_cached = 0;
    this->ip = "";
    this->port = 0;
    this->status_response = "";
    this->duration = 0;
}

CachedServer::CachedServer(string hostname, string ip, uint16_t port, int64_t time_cached, uint16_t duration,string status_response){
    this->hostname = hostname;
    this->time_cached = time_cached;
    this->ip = ip;
    this->port = port;
    this->status_response = status_response;
    this->duration = duration;
    this->update();
}


shared_ptr<CachedServer> findCachedServer(Route *route){
    if (route->cache < 1) return nullptr;
    lock_guard<mutex> lock(cached_servers_mutex);
    for (int i = 0; i < cached_servers.size(); i++){
        if (cached_servers[i]->hostname == route->hostname){
            return cached_servers[i];
        }
    }

    cached_servers.push_back(make_shared<CachedServer>(route->hostname, route->address, route->port, 0, route->cache, ""));
    return nullptr;
}

bool CachedServer::isExpired(){
    return time_cached + duration < std::chrono::duration_cast<chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

string CachedServer::get_status_response(){
    lock_guard<mutex> lock(cached_servers_mutex);
    return status_response;
}

int CachedServer::update() {
    unique_ptr<Packet> packet = make_unique<Packet>();

    string resolved_addr = resolve_minecraft_srv(ip, port);
    string addr = resolved_addr.substr(0, resolved_addr.find(":"));
    int p = stoi(resolved_addr.substr(resolved_addr.find(":") + 1));
    resolveARecord(&addr);

    int server_socket = dial(addr, p);
    if (server_socket < 0) {
        return -1;
    }

    packet->ID.setValue(0x00);
    packet->data.clear();
    packet->data.writeVarInt(765);
    packet->data.writeString(ip);
    packet->data.writeShort(port);
    packet->data.writeVarInt(1);

    if (packet->WriteToSocket(server_socket)) {
        return -1;
    }

    packet->ID.setValue(0x00);
    packet->data.clear();
    if (packet->WriteToSocket(server_socket)) {
        return -1;
    }

    packet->clear();
    if (packet->ReadFromSocket(server_socket)) {
        return -1;
    }
    
    if (packet->ID.getValue() != 0x00) {
        return -1;
    }

    // lock_guard<mutex> lock(cached_servers_mutex);
    this->status_response = packet->data.readString();
    this->time_cached = std::chrono::duration_cast<chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();


    return 0;
}