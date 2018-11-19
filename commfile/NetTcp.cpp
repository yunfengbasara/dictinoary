#include "stdafx.h"
#include "NetTcp.h"
#include "TcpLink.h"
#include "LogApi.h"

CNetTcp::CNetTcp()
	: m_hIOCP(NULL)
{
	WSADATA wsaData;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret != 0){
		LogWrite(EROR, _T("WSAStartup failed with error %d"), ret);
	}
}

CNetTcp::~CNetTcp()
{
	if (WSACleanup() == SOCKET_ERROR){
		LogWrite(EROR, _T("WSACleanup failed with error %d"), WSAGetLastError());
	}
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

	// test
	std::shared_ptr<CTcpLink> server(new CTcpLink(m_hIOCP));
	server->CreateServer("", 8888);
	server->Listen();

	std::shared_ptr<CTcpLink> client(new CTcpLink(m_hIOCP));
	client->CreateClient();
	client->Connect("", 8888);

	m_lLinkList.push_back(client);
	m_lLinkList.push_back(server);
	return true;
}

bool CNetTcp::Uninit()
{
	for (auto &pThread : m_lIocpNetPool) {
		auto pOvlp = new OVLPSTRUCT;
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

		auto pContext = CONTAINING_RECORD(pOvlp, OVLPSTRUCT, ovlp);
		std::shared_ptr<OVLPSTRUCT> pShareOvlp(pContext);

		if (pShareOvlp->event == NET_EVENT::NET_CLOS) {
			break;
		}
	
		IOCPCompletionFunc(dwStatus, dwBytes, pShareOvlp);
	}
}

void CNetTcp::IOCPCompletionFunc(DWORD dwStatus, DWORD dwBytes, std::shared_ptr<OVLPSTRUCT> pContext)
{
	switch (pContext->event) {
	case NET_ACPT: {
		int i = 0;
	}
		break;
	case NET_SEND: {
		int i = 0;
	}
		break;
	case NET_RECV: {
		int i = 0;
	}
		break;
	}
}