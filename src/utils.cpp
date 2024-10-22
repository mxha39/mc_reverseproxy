#include "header/utils.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <arpa/inet.h>
#include <netdb.h>

#include <arpa/nameser.h>
#include <functional>
#include <iostream>
#include <netinet/in.h>
#include <resolv.h>
#include <string.h>
#include <map>
#include <iomanip>
#include <sstream>



void handleSRVRecord(const ns_rr &rr, const ns_msg &nsMsg, string &host) {
    char name[1024];
    dn_expand(ns_msg_base(nsMsg), ns_msg_end(nsMsg), ns_rr_rdata(rr) + 6, name, sizeof(name));
    host = string(name) + ":" + to_string(ntohs(*((unsigned short*)ns_rr_rdata(rr) + 2)));
}


bool resolveARecord(string *hostname) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int status;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;     // IPv4, TODO: support IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;         // Any protocol

    status = getaddrinfo(hostname->c_str(), nullptr, &hints, &result);
    if (status != 0) {
        cerr << "getaddrinfo error: " << gai_strerror(status) << endl;
        return false;
    }

    // Find the first IPv4 address and update the hostname pointer
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in *)rp->ai_addr;
            char ipAddr[INET_ADDRSTRLEN];
            const char *ip = inet_ntop(AF_INET, &(addr->sin_addr), ipAddr, INET_ADDRSTRLEN);
            if (ip != nullptr) {
                *hostname = ip; // Update the hostname pointer to the first IPv4 address
                break;
            } else {
                cerr << "Failed to convert address to string" << endl;
            }
        }
    }

    freeaddrinfo(result);
    return true;
}




string resolve_minecraft_srv(const string &host, uint16_t port) {
    string full_host = "_minecraft._tcp." + host;
    res_init();

    int response;
    ns_msg nsMsg;

    unsigned char query_buffer[1024];
    {
        ns_type type = ns_t_srv;
        response = res_query(full_host.data(), C_IN, type, query_buffer, sizeof(query_buffer));
        if (response < 0) {
            return host+":" +to_string(port);
        }
    }

    ns_initparse(query_buffer, response, &nsMsg);

    for (int x = 0; x < ns_msg_count(nsMsg, ns_s_an); x++) {
        ns_rr rr;
        ns_parserr(&nsMsg, ns_s_an, x, &rr);
        if (ns_rr_type(rr) == ns_t_srv) {
            unsigned short port = ntohs(*((unsigned short*)ns_rr_rdata(rr) + 2)); // Port
            char name[1024];
            dn_expand(ns_msg_base(nsMsg), ns_msg_end(nsMsg), ns_rr_rdata(rr) + 6, name, sizeof(name)); // Name
            string host_info = string(name) + ":" + to_string(port);
            return host_info;
        }
    }

    return host+":"+to_string(port);
}


int dial(const std::string &target_ip, int target_port) {
    int target_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (target_sock < 0) {
        std::cerr << "Failed to create socket for target server." << std::endl;
        return -1;
    }

    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    if (setsockopt(target_sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
        std::cerr << "Failed to set socket send timeout." << std::endl;
        close(target_sock);
        return -1;
    }
    if (setsockopt(target_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
        std::cerr << "Failed to set socket receive timeout." << std::endl;
        close(target_sock);
        return -1;
    }

    struct sockaddr_in target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(target_port);

    if (inet_pton(AF_INET, target_ip.c_str(), &target_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address for target server." << std::endl;
        close(target_sock);
        return -1;
    }

    if (connect(target_sock, (struct sockaddr*)&target_addr, sizeof(target_addr)) < 0) {
        close(target_sock);
        return -1;
    }

    return target_sock;
}

void proxy(int client_sock, int target_sock) {
    auto forward = [](int from, int to) {
        char buffer[BUFFER_SIZE];
        ssize_t bytes;
        while ((bytes = read(from, buffer, BUFFER_SIZE)) > 0) {
            if (write(to, buffer, bytes) < 0) {
                // std::cerr << "Write error." << std::endl; // TODO fix
                // break;
                continue;
            }
        }
        close(from);
        close(to);
    };

    std::thread(forward, client_sock, target_sock).detach();
    forward(target_sock, client_sock); // blocking
}