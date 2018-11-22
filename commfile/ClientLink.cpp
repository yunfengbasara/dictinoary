#include "stdafx.h"
#include "ClientLink.h"

CClientLink::CClientLink(HANDLE pIOCP, SOCKET hSocket)
	: CTcpLink(pIOCP, hSocket)
{
	__super::CreateClient();
}

CClientLink::~CClientLink()
{
}

bool CClientLink::Connect(const std::string addr, uint32_t port)
{
	return __super::Connect(addr, port);
}