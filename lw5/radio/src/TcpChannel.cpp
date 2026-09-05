#include "TcpChannel.h"

#include <array>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

namespace radio
{

namespace
{
constexpr size_t PacketHeaderSize = 28;
}

bool SendAll(TcpSocket& socket, const std::byte* data, size_t size)
{
	boost::system::error_code errorCode;
	boost::asio::write(socket, boost::asio::buffer(data, size), errorCode);
	return !errorCode;
}

bool ReceiveAll(TcpSocket& socket, std::byte* data, size_t size)
{
	boost::system::error_code errorCode;
	boost::asio::read(socket, boost::asio::buffer(data, size), errorCode);
	return !errorCode;
}

bool SendPacket(TcpSocket& socket, const PacketHeader& header, const std::vector<std::byte>& payload)
{
	PacketHeader headerCopy = header;
	headerCopy.payloadSize = static_cast<uint32_t>(payload.size());
	const std::vector<std::byte> serializedHeader = SerializePacketHeader(headerCopy);

	if (!SendAll(socket, serializedHeader.data(), serializedHeader.size()))
	{
		return false;
	}

	if (!payload.empty())
	{
		return SendAll(socket, payload.data(), payload.size());
	}
	return true;
}

bool ReceivePacket(TcpSocket& socket, ReceivedPacket& packet)
{
	std::array<std::byte, PacketHeaderSize> headerBytes{};
	if (!ReceiveAll(socket, headerBytes.data(), headerBytes.size()))
	{
		return false;
	}

	packet.header = DeserializePacketHeader({ headerBytes.begin(), headerBytes.end() });
	packet.payload.resize(packet.header.payloadSize);
	if (!packet.payload.empty() && !ReceiveAll(socket, packet.payload.data(), packet.payload.size()))
	{
		return false;
	}

	return true;
}

} // namespace radio
