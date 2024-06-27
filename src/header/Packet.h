#ifndef PACKET_H
#define PACKET_H

#include "Buffer.h"
#include "VarInt.h"

class Packet {
public:
    Packet();

    VarInt ID;
    Buffer data;

    bool ReadFromSocket(int socketfd);
    bool WriteToSocket(int socketfd);

    void clear();
};

#endif // PACKET_H
