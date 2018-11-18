#pragma once
#include "NetApi.h"

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
	void IOCPCompletionFunc(DWORD dwStatus, DWORD dwBytes, OVLPSTRUCT* pContext);

private:
	HANDLE						m_hIOCP;
	std::vector<std::thread>	m_lIocpNetPool;		// IOCP事件线程池
};