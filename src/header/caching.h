#ifndef CACHING_H
#define CACHING_H

#include <vector>
#include <iostream>
#include <cstdint>
#include "routes.h"

using namespace std;


class CachedServer{
public:
    CachedServer(string hostname, string ip, uint16_t port, int64_t time_cached, uint16_t duration,string status_response);
    CachedServer();
    string hostname;
    string ip;
    uint16_t port;
    int64_t time_cached;
    uint16_t duration;


    bool isExpired();
    string get_status_response();
    int update();
private:
    string status_response;

};

shared_ptr<CachedServer> findCachedServer(Route *route);

void clearExpiredServers();

#endif // CACHING_H
