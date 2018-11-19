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
	void IOCPCompletionFunc(DWORD dwStatus, DWORD dwBytes, std::shared_ptr<OVLPSTRUCT> pContext);

private:
	HANDLE											m_hIOCP;			// IOCP通知句柄
	std::vector<std::thread>						m_lIocpNetPool;		// IOCP事件线程池
	std::list<std::shared_ptr<CTcpLink>>			m_lLinkList;		// IOCP管理通知的SOCKET链表
};