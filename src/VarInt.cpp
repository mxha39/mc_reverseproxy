#include "header/VarInt.h"
#include <sys/socket.h>
#include <vector>
#include <iostream>


VarInt::VarInt(uint32_t v) : value(v) {}

uint32_t VarInt::getValue() const {
    return value;
}

bool VarInt::readFromSocket(int socketfd) {
    value = 0;
    uint32_t tempValue = 0;
    uint8_t byte = 0;
    int shift = 0;
    
    do {
        ssize_t bytesRead = read(socketfd, &byte, sizeof(byte));
        if (bytesRead != sizeof(byte)) {
            perror("Error reading from socket");
            return true;
        }
        
        tempValue |= static_cast<uint32_t>(byte & 0x7F) << shift;
        shift += 7;
        
    } while (byte & 0x80);

    value = tempValue;
    return false;
}

std::vector<uint8_t> VarInt::toBytes() const {
    std::vector<uint8_t> bytes;
    uint32_t tempValue = value;
    uint8_t byte = 0;
    
    do {
        byte = tempValue & 0x7F;
        tempValue >>= 7;
        if (tempValue != 0) {
            byte |= 0x80;
        }
        
        bytes.push_back(byte);
    } while (tempValue != 0);

    return bytes;
}

VarInt VarInt::operator-(const VarInt& other) const {
    return VarInt(value - other.value);
}

uint32_t VarInt::size() const {
    uint32_t tempValue = value;
    uint32_t size = 0;
    
    do {
        tempValue >>= 7;
        size++;
    } while (tempValue != 0);
    
    return size;
}

bool VarInt::writeToSocket(int socketfd) const {
   uint8_t byte;
   uint32_t v = value;

   do {
        byte = v & 0x7F;
        v >>= 7;
        if (v != 0) {
            byte |= 0x80;
        }
        if (send(socketfd, &byte, sizeof(byte), 0) != 1) {
            perror("Error writing varint");
            return true;;
        }
    } while (v != 0);

    return false;
}

void VarInt::setValue(uint32_t v) {
    value = v;
}