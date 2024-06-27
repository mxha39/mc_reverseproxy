#include <iostream>
#include <vector>
#include "header/caching.h"
#include "header/Packet.h"
#include "header/routes.h"
#include "header/utils.h"
#include <arpa/inet.h>
#include <thread>
#include "config.cpp"
#include "packets.cpp"

using namespace std;


enum Placeholder_state {
    OFFLINE = 1,
    UNKNOWN = 2
};


void handle_ping(int socketfd) {
    unique_ptr<Packet> packet = make_unique<Packet>();
    packet->clear();
    if(packet->ReadFromSocket(socketfd)) {
        return;
    }
    if (packet->ID.getValue() != 0x01) {
        return;
    }
    packet->WriteToSocket(socketfd);
    return;
}

void handle_placeholder(int client_socket, uint32_t next_state, Placeholder_state state) {
    unique_ptr<Packet> packet = make_unique<Packet>();

    if ((*packet).ReadFromSocket(client_socket)) {
        cout << "close" << endl;
        close(client_socket);
        return;
    }
    if (packet->ID.getValue() != 0x00) {
        return;
    }

    packet->clear();
    packet->ID = 0x00;
    if(next_state == 1) {
        if (state == UNKNOWN) {
            packet->data.writeString("{\"version\":{\"name\":\"Unknown\",\"protocol\":0},\"players\":{\"max\":0,\"online\":0},\"description\":{\"text\":\"§bProxy §8- §cUnknown Route\"}}");
        } else {
            packet->data.writeString("{\"version\":{\"name\":\"Offline\",\"protocol\":0},\"players\":{\"max\":0,\"online\":0},\"description\":{\"text\":\"§bProxy §8- §cServer offline\"}}");
        }
    } else {
        if (state == UNKNOWN) {
            packet->data.writeString("{\"text\":\"§cUnknown Route\"}");
        } else {
            packet->data.writeString("{\"text\":\"§cServer offline\"}");
        }
    }
    packet->WriteToSocket(client_socket);

    handle_ping(client_socket);
    return;
}



void handle_client(int client_socket) {
    unique_ptr<Packet> packet = make_unique<Packet>();
    if ((*packet).ReadFromSocket(client_socket)) {
        cout << "close" << endl;
        close(client_socket);
        return;
    }

    unique_ptr<Handshake_Packet> hs = make_unique<Handshake_Packet>();
    hs->readFromBuffer(&(packet->data));

    Route* r = findRoute(hs->hostname);

    if (r == nullptr) {
        handle_placeholder(client_socket, hs->nextState.getValue(), UNKNOWN);
        close(client_socket);
        return;
    }

    hs->hostname = r->address;
    hs->writeToBuffer(&(packet->data));
    shared_ptr<CachedServer> cs = findCachedServer(r);
    if(hs->nextState.getValue() == 1&& cs != nullptr) {
        if (cs->isExpired()) {
            if(cs->update() < 0 ) {
                handle_placeholder(client_socket, hs->nextState.getValue(), OFFLINE);
                close(client_socket);
                return;
            }
        }

        if ((*packet).ReadFromSocket(client_socket)) {
            close(client_socket);
            return;
        }
        if (packet->ID.getValue() != 0x00) {
            return;
        }

        packet->clear();
        packet->ID = 0x00;
        packet->data.writeString(cs->get_status_response());
        packet->WriteToSocket(client_socket);
        handle_ping(client_socket);
        close(client_socket);
        return;
    }

    string resolved_addr = resolve_minecraft_srv(r->address, r->port);
    string addr = resolved_addr.substr(0, resolved_addr.find(":"));
    int port = stoi(resolved_addr.substr(resolved_addr.find(":") + 1));
    resolveARecord(&addr);

    int target_sock = dial(addr, port);
    if (target_sock < 0) {
        handle_placeholder(client_socket, hs->nextState.getValue(), OFFLINE);
        close(client_socket);
        close(target_sock);
        return;
    }
    
    if ((*packet).WriteToSocket(target_sock)) {
        close(client_socket);
        close(target_sock);
        return;
    }



    if (hs->nextState.getValue() == 2) {
        //read login packet
        packet->clear();
        if ((*packet).ReadFromSocket(client_socket)) {
            cout << "close" << endl;
            close(client_socket);
            return;
        }
        if (packet->ID.getValue() != 0x00) {
            return;
        }
        
        unique_ptr<LoginPacket> lp = make_unique<LoginPacket>();
        

        lp->readFromBuffer(&(packet->data));
        lp->writeToBuffer(&(packet->data));

        string client_IP;
        struct sockaddr_in addr;
        socklen_t addr_size = sizeof(addr);
        getpeername(client_socket, (struct sockaddr *)&addr, &addr_size);
        client_IP = inet_ntoa(addr.sin_addr);
        cout << "User " << lp->username <<  " (" << client_IP << ") connected to " << r->hostname << endl;
        if (cs != nullptr) cs->update();
        packet->WriteToSocket(target_sock);
    }

    proxy(client_socket,target_sock);
    close(client_socket);
    close(target_sock);
    if (cs != nullptr) cs->update();
    return;
}

int main() {
    signal(SIGPIPE, SIG_IGN); 
    createConfig("config.json");
    loadConfig("config.json");
    if (!load_routes()) {
        cout << "Failed to load routes" << endl;
        return 1;
    }
    thread(file_watcher, "routes.json").detach();

    cout << "Starting" << endl;
    // create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Failed to create socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("Failed to set socket options");
        close(server_socket);
        return 1;
    }

    // bind
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // TODO
    server_address.sin_port = htons((*config).port); 
    server_address.sin_addr.s_addr = INADDR_ANY;

    int bind_status = bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address));
    if (bind_status == -1) {
        perror("Failed to bind socket");
        return 1;
    }

    //listen
    int listen_status = listen(server_socket, 5); // max backlog
    if (listen_status == -1) {
        perror("Failed to listen on socket");
        return 1;
    }
    cout << "Listening." << endl;

    thread(clearExpiredServers).detach();
    while (true) {
        struct sockaddr_in client_address;
        socklen_t client_address_size = sizeof(client_address);

        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_size);
        if (client_socket == -1) {
            perror("Failed to accept connection");
            continue;
        }

        thread(handle_client, client_socket).detach();
    }

    close(server_socket);
    return 0;
}
