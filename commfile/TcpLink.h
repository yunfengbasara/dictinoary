#pragma once
#include "NetApi.h"

class CTcpLink
{
public:
	CTcpLink(HANDLE pIOCP);
	~CTcpLink();
	bool CreateClient();
	bool CreateServer(const std::string addr, uint32_t port);
	void Destroy();
	bool Connect(const std::string addr, uint32_t port);
	bool Listen();

	bool NotifyRecv();
	bool NotifySend(const char* pData, uint32_t cbSize);
	bool NotifyClose();
	bool NotifyAccept();

private:
	HANDLE	 m_hIOCP;
	SOCKET	 m_hSock;
};

