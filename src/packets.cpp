#include <cstdint>
#include "header/VarInt.h"
#include "header/Buffer.h"
#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

class UUID {
public:
    UUID() {
        for (int i = 0; i < 16; i++) {
            uuid[i] = 0;
        }
    }
    UUID(const char *uuid) {
        for (int i = 0; i < 16; i++) {
            this->uuid[i] = uuid[i];
        }
    }
    UUID(const string &uuid) {
        for (int i = 0; i < 16; i++) {
            this->uuid[i] = uuid[i];
        }
    }
    std::string toString() const {
        std::stringstream s_stream;
        s_stream << std::hex << std::setfill('0');
        for (int i = 0; i < 16; ++i) {
            if (i == 4 || i == 6 || i == 8 || i == 10) {
                s_stream << "-";
            }
            s_stream << std::setw(2) << static_cast<unsigned>(static_cast<unsigned char>(uuid[i]));
        }
        return s_stream.str();
    }
    char *get() {
        return uuid;
    }
private:
    char uuid[16];
};

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