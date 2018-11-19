#pragma once
#include "NetApi.h"
#include "TcpLink.h"

// IOCP֪ͨ
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
	HANDLE											m_hIOCP;			// IOCP֪ͨ���
	std::vector<std::thread>						m_lIocpNetPool;		// IOCP�¼��̳߳�
	std::list<std::shared_ptr<CTcpLink>>			m_lLinkList;		// IOCP����֪ͨ��SOCKET����
};