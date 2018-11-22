#pragma once
#include "NetApi.h"
#include "Packet.h"

// socket连接 IOCP非阻塞模式
// 所有调用函数均为非阻塞
class CTcpLink
{
public:
	CTcpLink(HANDLE pIOCP, SOCKET hSocket = INVALID_SOCKET);
	virtual ~CTcpLink();

	SOCKET&	GetSocket();

	// client
	bool CreateClient();
	bool Connect(const std::string addr, uint32_t port);

	// server
	bool CreateServer(const std::string addr, uint32_t port);
	bool Listen();
	bool Accept();

	// normal
	bool Close();
	bool Recv();
	bool Send(const std::basic_string<BYTE>&);

	// iocp callback
	bool OnSend(const PBYTE pData, uint32_t len);
	bool OnRecv(const PBYTE pData, uint32_t len);

	void SendPkt();		// 发送数据包

private:
	HANDLE						m_hIOCP;
	SOCKET						m_hSock;
	std::shared_ptr<CPacket>	m_pPkt;
};