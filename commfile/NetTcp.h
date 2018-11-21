#pragma once
#include "NetApi.h"
#include "TcpLink.h"

// IOCP通知
class CNetTcp
{
public:
	CNetTcp();
	~CNetTcp();

	bool Init(int threadN = 0);
	bool Uninit();

private:
	void IOCPRoutine(HANDLE hIOCP);

	void OnConn(std::shared_ptr<CONNOVLP> pOvlp);
	void OnSend(std::shared_ptr<SENDOVLP> pOvlp);
	void OnRecv(std::shared_ptr<RECVOVLP> pOvlp);
	void OnAcpt(std::shared_ptr<ACPTOVLP> pOvlp);
	void OnClos(std::shared_ptr<CLOSOVLP> pOvlp);

private:
	HANDLE											m_hIOCP;			// IOCP通知句柄
	std::vector<std::thread>						m_lIocpPool;		// IOCP事件线程池
	typedef std::shared_ptr<CTcpLink>				TcpLink;
	std::mutex										m_nMutex;
	std::map<SOCKET, TcpLink>						m_lLinkList;		// 管理socket
};