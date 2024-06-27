#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include "VarInt.h"
#include <iostream>

class Buffer {
public:
    Buffer();
    std::vector<char> readData(size_t numBytes);

    void setData(const std::vector<uint8_t>& data);
    void resize(size_t newSize);
    uint32_t size() const;

    std::vector<uint8_t> getData() const;

    void clear();
    
    void erase(size_t numBytes);

    void insertVarIntAsFirst(VarInt v);
    void insertStringAsFirst(const std::string& str);
    void insertUShortAsFirst(uint16_t value);

    uint8_t readByte();
    void writeByte(uint8_t byte);

    //types
    VarInt readVarInt();
    void writeVarInt(VarInt varint);
    std::string readString();
    void writeString(const std::string& str);
    uint16_t readShort();
    void writeShort(uint16_t value);


private:
    std::vector<uint8_t> buffer;
};

#endif // BUFFER_H
