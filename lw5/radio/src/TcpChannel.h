#pragma once

#include "Protocol.h"

#include <boost/asio/ip/tcp.hpp>

namespace radio
{

using TcpSocket = boost::asio::ip::tcp::socket;

struct ReceivedPacket
{
    PacketHeader header;
    std::vector<std::byte> payload;
};

[[nodiscard]] bool SendAll(TcpSocket& socket, const std::byte* data, size_t size);
[[nodiscard]] bool ReceiveAll(TcpSocket& socket, std::byte* data, size_t size);
[[nodiscard]] bool SendPacket(TcpSocket& socket, const PacketHeader& header, const std::vector<std::byte>& payload);
[[nodiscard]] bool ReceivePacket(TcpSocket& socket, ReceivedPacket& packet);

}
