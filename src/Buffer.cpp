#include "header/Buffer.h"
#include <algorithm>
#include <iostream>

Buffer::Buffer()  {}


std::vector<char> Buffer::readData(size_t numBytes) {
    std::vector<char> dataRead;
    
    size_t remainingBytes = buffer.size();
    size_t bytesToRead = std::min(numBytes, remainingBytes);
    
    dataRead.assign(buffer.begin(), buffer.begin() + bytesToRead);
    
    erase(bytesToRead);
    
    buffer.erase(buffer.begin(), buffer.begin());
      
    return dataRead;
}


void Buffer::resize(size_t newSize) {
    buffer.resize(newSize);
}


void Buffer::setData(const std::vector<uint8_t>& data) {
    buffer.assign(data.begin(), data.end());
}

void Buffer::erase(size_t n) {
    if (n > buffer.size()) {
        throw std::out_of_range("Not enough bytes in the buffer");
    }
    buffer.erase(buffer.begin(), buffer.begin() + n);
}

void Buffer::insertVarIntAsFirst(VarInt v) {
    std::vector<uint8_t> varint = v.toBytes();
    buffer.insert(buffer.begin(), varint.begin(), varint.end());
}

void Buffer::insertStringAsFirst(const std::string& str) {
    insertVarIntAsFirst(VarInt(str.size()));
    buffer.insert(buffer.begin(), str.begin(), str.end());
}

void Buffer::insertUShortAsFirst(uint16_t value) {
    buffer.insert(buffer.begin(), (value >> 8) & 0xFF);
    buffer.insert(buffer.begin(), value & 0xFF);
}

VarInt Buffer::readVarInt() {
    uint32_t varint = 0;
    int shift = 0;
    uint8_t byte;

    do {
        byte = buffer[0];
        erase(1);
        varint |= (byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    return VarInt(varint);

}

uint16_t Buffer::readShort() {
    uint16_t value = (buffer[0] << 8) | buffer[1];
    erase(2);
    return value;
}

void Buffer::writeVarInt(VarInt varint) {
    uint32_t value = varint.getValue();
    uint8_t byte;
    do {
        byte = value & 0x7F;
        value >>= 7;
        if (value != 0) {
            byte |= 0x80;
        }
        buffer.push_back(byte);
    } while (value != 0);
}

void Buffer::writeShort(uint16_t value) {

    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back(value & 0xFF);
}



void Buffer::writeByte(uint8_t byte) {
    buffer.push_back(byte);
}

std::vector<uint8_t> Buffer::getData() const {
    return buffer;
}

void Buffer::clear() {
    buffer.erase(buffer.begin(), buffer.end());
}


uint32_t Buffer::size() const {
    return buffer.size();
}


void Buffer::writeString(const std::string& str) {
    VarInt strLen = VarInt(str.length());
    writeVarInt(strLen);
    
    buffer.insert(buffer.end(), str.begin(), str.end());
}

std::string Buffer::readString() {
    size_t length = readVarInt().getValue();
    
    if (length <= buffer.size()) {
        std::string result(buffer.begin(), buffer.begin() + length);
        erase(length);
        return result;
    }
    
    return "";
}