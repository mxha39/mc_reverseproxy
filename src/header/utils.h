#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <resolv.h>

using namespace std;

#define BUFFER_SIZE 4096

void handleSRVRecord(const ns_rr &rr, const ns_msg &nsMsg, std::string &host);
bool resolveARecord(string *hostname);
string resolve_minecraft_srv(const string &host, uint16_t port);
int dial(const std::string &target_ip, int target_port);
void proxy(int client_sock, int target_sock);

#endif // UTILS_HPP
