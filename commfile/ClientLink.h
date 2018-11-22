#pragma once
#include "TcpLink.h"

class CClientLink : public CTcpLink
{
public:
	CClientLink(HANDLE pIOCP, SOCKET hSocket = INVALID_SOCKET);
	virtual ~CClientLink();
	bool Connect(const std::string addr, uint32_t port);
};

