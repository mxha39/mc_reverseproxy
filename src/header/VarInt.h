#ifndef VARINT_H
#define VARINT_H

#include <cstdint>
#include <unistd.h>
#include <vector>

class VarInt {
private:
    uint32_t value;

public:
    VarInt(uint32_t v = 0);
    uint32_t getValue() const;
    bool readFromSocket(int socketfd);
    bool writeToSocket(int socketfd) const;

    uint32_t size() const;

    std::vector<uint8_t> toBytes() const;
    void setValue(uint32_t v);
    VarInt operator-(const VarInt& other) const;
};

#endif // VARINT_H
