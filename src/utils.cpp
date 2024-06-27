#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096

using namespace std;

int dial(const std::string &target_ip, int target_port) {
    int target_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (target_sock < 0) {
        std::cerr << "Failed to create socket for target server." << std::endl;
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
        std::cerr << "Connection to target server failed." << std::endl;
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
                std::cerr << "Write error." << std::endl;
                break;
            }
        }
        close(from);
        close(to);
    };

    std::thread(forward, client_sock, target_sock).detach();
    forward(target_sock, client_sock); // blocking
}