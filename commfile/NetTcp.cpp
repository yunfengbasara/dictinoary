#include "stdafx.h"
#include "NetTcp.h"

CNetTcp::CNetTcp()
	: m_hIOCP(NULL)
{
}

CNetTcp::~CNetTcp()
{
}

bool CNetTcp::Init(int threadN)
{
	m_hIOCP = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (m_hIOCP == NULL) {
		return false;
	}

	if (threadN == 0) {
		SYSTEM_INFO si;
		::GetSystemInfo(&si);
		threadN = si.dwNumberOfProcessors * 2;
	}

	for (int s = 0; s < threadN; s++) {
		m_lIocpNetPool.push_back(std::thread(&CNetTcp::IOCPRoutine, this, m_hIOCP));
	}

	return true;
}

bool CNetTcp::Uninit()
{
	for (auto &pThread : m_lIocpNetPool) {
		std::shared_ptr<OVLPSTRUCT> pOvlp(new OVLPSTRUCT);
		pOvlp->event = NET_EVENT::NET_CLOS;
		::PostQueuedCompletionStatus(m_hIOCP, 0, NET_EVENT::NET_CLOS, &pOvlp->ovlp);
	}

	for (auto &pThread : m_lIocpNetPool) {
		pThread.join();
	}
	return true;
}

void CNetTcp::IOCPRoutine(HANDLE hIOCP)
{
	DWORD dwBytes = 0;
	ULONG_PTR nEvent = 0;
	LPOVERLAPPED pOvlp = NULL;
	DWORD dwStatus = 0;	// ·µ»Ø×´Ì¬
	while (true) {
		if (::GetQueuedCompletionStatus(hIOCP, &dwBytes, &nEvent, &pOvlp, INFINITE)) {
			dwStatus = ERROR_SUCCESS;
		}
		else {
			dwStatus = GetLastError();
		}

		OVLPSTRUCT* pContext = CONTAINING_RECORD(pOvlp, OVLPSTRUCT, ovlp);
		if (pContext->event == NET_EVENT::NET_CLOS) {
			break;
		}

		IOCPCompletionFunc(dwStatus, dwBytes, pContext);
	}
}

void CNetTcp::IOCPCompletionFunc(DWORD dwStatus, DWORD dwBytes, OVLPSTRUCT* pContext)
{

}