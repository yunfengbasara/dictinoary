#pragma once
#include "NetApi.h"
#include "Packet.h"

// socket���� IOCP������ģʽ
// ���е��ú�����Ϊ������
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

	void SendPkt();		// �������ݰ�

private:
	HANDLE						m_hIOCP;
	SOCKET						m_hSock;
	std::shared_ptr<CPacket>	m_pPkt;
};