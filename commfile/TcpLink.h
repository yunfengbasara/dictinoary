#pragma once
#include "NetApi.h"

// socket���� IOCP������ģʽ
// ���е��ú�����Ϊ������
class CTcpLink
{
public:
	CTcpLink(HANDLE pIOCP, SOCKET hSocket = INVALID_SOCKET);
	~CTcpLink();

	SOCKET&	GetSocket();

	// accept socket
	bool Create();

	// client
	bool CreateClient();
	bool Connect(const std::string addr, uint32_t port);

	// server
	bool CreateServer(const std::string addr, uint32_t port);
	bool Listen();
	bool Accept();

	// normal
	bool Recv();
	bool Send(const char* pData, uint32_t cbSize);
	bool Close();

private:
	HANDLE	 m_hIOCP;
	SOCKET	 m_hSock;
};