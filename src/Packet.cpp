#include "header/Packet.h"

Packet::Packet() {
    ID = 0;
    data.clear();
}


bool Packet::ReadFromSocket(int socketfd) {
    VarInt packet_length(0);
    packet_length.readFromSocket(socketfd);
    ID.readFromSocket(socketfd);
    std::vector<uint8_t> tmp(packet_length.getValue() - ID.size());

    size_t total_bytes_read = 0;
    while (total_bytes_read < tmp.size()) {
        ssize_t bytesRead = read(socketfd, tmp.data() + total_bytes_read, tmp.size() - total_bytes_read);
        if (bytesRead < 0) {
            perror("Error reading from socket");
            // close(socketfd);
            return true;
        }
        total_bytes_read += bytesRead;
    }

    data.setData(tmp);

    return false;
}

void Packet::clear() {
    data.clear();
    ID = 0;
}


bool Packet::WriteToSocket(int socketfd) {
    VarInt(ID.size() + data.size()).writeToSocket(socketfd);
    ID.writeToSocket(socketfd);

    ssize_t bytesWritten = write(socketfd, data.getData().data(), data.size());
    if (bytesWritten != static_cast<ssize_t>(data.size())) {
        perror("Error writing to socket");
        return true;
    }

    return false;
}

