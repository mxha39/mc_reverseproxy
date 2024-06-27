#include <iostream>
#include <vector>
#include <mutex>
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

struct LoginPacket {
    string username;
    UUID uuid;
    void readFromBuffer(Buffer *buf) {
        username = buf->readString();
        char uid[16];
        for (int i = 0; i < 16; i++) {
            uid[i] = buf->readByte();
        }
        uuid = uid;        
    }
    void writeToBuffer(Buffer *buf) {
        buf->writeString(username);
        for (int i = 0; i < 16; i++) {
            buf->writeByte(uuid.get()[i]);
        }
    }
};
struct Route {
    string hostname;
    string address;
    uint16_t port;
};
mutex routesMux;

vector<Route> routes = {
    {"hyp.local","hypixel.net", 25565},
    {"timolia.local", "timolia.de",25565},
    {"gomme.local", "gommehd.net", 25565}
};

Route* findRoute(const string& hostname) {
    lock_guard<mutex> lock(routesMux);
    for (auto& route : routes) {
        if (route.hostname == hostname) {
            return &route;
        }
    }
    return nullptr;
}



enum Placeholder_state {
    OFFLINE = 1,
    UNKNOWN = 2
};

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

    packet->clear();
    packet->ReadFromSocket(client_socket);
    if (packet->ID.getValue() != 0x01) {
        return;
    }
    packet->WriteToSocket(client_socket);

    return;
}


void handle_client(int client_socket) {
    unique_ptr<Packet> packet = make_unique<Packet>();
    if ((*packet).ReadFromSocket(client_socket)) {
        cout << "close" << endl;
        close(client_socket);
        return;
    }

    Handshake_Packet hs;
    hs.readFromBuffer(&(packet->data));

    Route* r = findRoute(hs.hostname);

    if (r == nullptr) {
        handle_placeholder(client_socket, hs.nextState.getValue(), UNKNOWN);
        close(client_socket);
        return;
    }
    hs.hostname = r->address;
    hs.writeToBuffer(&(packet->data));
    
    string resolved_addr = resolve_minecraft_srv(r->address);
    string addr = resolved_addr.substr(0, resolved_addr.find(":"));
    int port = stoi(resolved_addr.substr(resolved_addr.find(":") + 1));
    resolveARecord(&addr);

    int target_sock = dial(addr, port);
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


    if (hs.nextState.getValue() == 2) {
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

        packet->WriteToSocket(target_sock);
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
