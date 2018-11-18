#pragma once
class CTcpLink
{
public:
	CTcpLink(HANDLE pIOCP);
	~CTcpLink();
	bool CreateClient();
	bool CreateServer(LPTSTR addr, uint32_t port);

private:
	HANDLE	 m_hIOCP;
	SOCKET	 m_hSock;
};

