#include <iostream>
#include <vector>

#include "header/Packet.h"
#include "utils.cpp"


using namespace std;

struct Handshake_Packet {
    VarInt version;
    string hostname;
    uint16_t port;
    VarInt nextState;
    void readFromBuffer(Buffer *buf) {
        version = buf->readVarInt();
        hostname = buf->readString();
        port = buf->readShort();
        nextState = buf->readVarInt();
    }

    void writeToBuffer(Buffer *buf) {
        buf->clear();
        buf->writeVarInt(version);
        buf->writeString(hostname);
        buf->writeShort(port);
        buf->writeVarInt(nextState);
    }
};

struct Route {
    string hostname;
    string address;
    uint16_t port;
};

vector<Route> routes = {
    {"hyp.local","hypixel.net", 25565},
    {"timolia.local", "timolia.de",25565},
    {"gomme.local", "gommehd.net", 25565}
};
// todo: mutex


void handle_client(int client_socket) {
    Packet *packet = new Packet();
    if ((*packet).ReadFromSocket(client_socket)) {
        cout << "close" << endl;
        close(client_socket);
        return;
    }

    Handshake_Packet hs;
    hs.readFromBuffer(&(packet->data));

    Route *r;

    cout<< "hostname: " << hs.hostname << endl;
    for (auto &route : routes) {
        if (route.hostname == hs.hostname) {
            r = &route;
            break;
        }
    }

    if (r == nullptr) {
        cout << "Route not found" << endl;
        close(client_socket);
        return;
    }
    hs.hostname = r->address;
    hs.writeToBuffer(&(packet->data));
    
    string resolved_addr = resolve_minecraft_srv(r->address);
    string addr = resolved_addr.substr(0, resolved_addr.find(":"));
    int port = stoi(resolved_addr.substr(resolved_addr.find(":") + 1));
    resolveARecord(&addr);


    int target_sock = dial(addr, port); //dont forget to change port according to srv
    if (target_sock < 0) {
        close(client_socket);
        close(target_sock);
        return;
    }

    if ((*packet).WriteToSocket(target_sock)) {
        close(client_socket);
        close(target_sock);
        return;
    }


    proxy(client_socket,target_sock);
    close(client_socket);
    cout << "closed client socket" << endl;
    close(target_sock);
    return;
}

int main() {
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
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(25565); 
    server_address.sin_addr.s_addr = INADDR_ANY; //TODO

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

    std::vector<std::thread> threads;

    while (true) {
        struct sockaddr_in client_address;
        socklen_t client_address_size = sizeof(client_address);

        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_size);
        if (client_socket == -1) {
            perror("Failed to accept connection");
            continue;
        }

        threads.emplace_back(handle_client, client_socket);
    }

    // clean up threads before exiting
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    close(server_socket);

    return 0;
}
