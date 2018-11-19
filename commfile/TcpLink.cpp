#include "stdafx.h"
#include "TcpLink.h"

CTcpLink::CTcpLink(HANDLE pIOCP)
	: m_hSock(INVALID_SOCKET)
	, m_hIOCP(pIOCP)
{

}

CTcpLink::~CTcpLink()
{
}

bool CTcpLink::CreateClient()
{

	return true;
}

bool CTcpLink::CreateServer(LPTSTR addr, uint32_t port)
{
	return true;
}
