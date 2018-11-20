#include "stdafx.h"
#include "NetTcp.h"
#include "TcpLink.h"
#include "LogApi.h"
#include "convert.h"

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
	TcpLink server(new CTcpLink(m_hIOCP));
	server->CreateServer("", 8888);
	server->Listen();

	TcpLink client(new CTcpLink(m_hIOCP));
	client->CreateClient();

	// 提前放如队列
	m_lLinkList.insert(std::make_pair(server->GetSocket(), server));
	m_lLinkList.insert(std::make_pair(client->GetSocket(), client));
	LogWrite(INFO, _T("Server Link %d"), server->GetSocket());
	LogWrite(INFO, _T("Client Link %d"), client->GetSocket());

	client->Connect("", 8888);

	return true;
}

bool CNetTcp::Uninit()
{
	for (auto &pThread : m_lIocpNetPool) {
		// win10 sdk可直接使用智能指针
		EXITOVLP *pOvlp = new EXITOVLP;
		::PostQueuedCompletionStatus(m_hIOCP, 0, 0, &pOvlp->ovlp);
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
	DWORD dwStatus = 0;	// 返回状态
	while (true) {
		dwStatus = ERROR_SUCCESS;
		if (!::GetQueuedCompletionStatus(hIOCP, 
			&dwBytes, &nEvent, &pOvlp, INFINITE)) {

			// 关闭socket响应
			dwStatus = WSAGetLastError();	
			auto pContext = CONTAINING_RECORD(pOvlp, OVLPSTRUCT, ovlp);
			CLOSOVLP* pClos = (CLOSOVLP*)pContext;
			std::shared_ptr<CLOSOVLP> pClose(pClos);
			{
				std::unique_lock<std::mutex> lock(m_nMutex);
				m_lLinkList.erase(pClose->hSock);
			}
			continue;
		}

		// OVLP转自定义结构
		auto pContext = CONTAINING_RECORD(pOvlp, OVLPSTRUCT, ovlp);
		pContext->dwBytes = dwBytes;

		switch (pContext->event) {
		case NET_CONN: {
			CONNOVLP* p = (CONNOVLP*)pContext;
			std::shared_ptr<CONNOVLP> pShareOvlp(p);
			OnConn(pShareOvlp);
		}break;
		case NET_SEND: {
			SENDOVLP* p = (SENDOVLP*)pContext;
			std::shared_ptr<SENDOVLP> pShareOvlp(p);
			OnSend(pShareOvlp);
		}break;
		case NET_RECV: {
			RECVOVLP* p = (RECVOVLP*)pContext;
			std::shared_ptr<RECVOVLP> pShareOvlp(p);
			OnRecv(pShareOvlp);
		}break;
		case NET_ACPT: {
			ACPTOVLP* p = (ACPTOVLP*)pContext;
			std::shared_ptr<ACPTOVLP> pShareOvlp(p);
			OnAcpt(pShareOvlp);
		}break;
		case NET_CLOS: {
			CLOSOVLP* p = (CLOSOVLP*)pContext;
			std::shared_ptr<CLOSOVLP> pShareOvlp(p);
			OnClos(pShareOvlp);
		}break;
		case NET_EXIT: {
			EXITOVLP* p = (EXITOVLP*)pContext;
			std::shared_ptr<EXITOVLP> pShareOvlp(p);
			return;
		}break;
		}
	}
}

void CNetTcp::OnConn(std::shared_ptr<CONNOVLP> pOvlp)
{
	TcpLink pLink;
	{
		std::unique_lock<std::mutex> lock(m_nMutex);
		auto pit = m_lLinkList.find(pOvlp->hSock);
		if (pit == m_lLinkList.end()) {
			return;
		}
		pLink = pit->second;
	}

	pLink->Recv();
	pLink->Send("hello world", 11);
	pLink->Close();
}

void CNetTcp::OnSend(std::shared_ptr<SENDOVLP> pOvlp)
{
	std::basic_string<char> str((char*)pOvlp->data, pOvlp->dwBytes);
	std::basic_string<wchar_t> wstr;
	char2wchar(str, wstr);
	LogWrite(INFO, _T("OnSend %s"), wstr.c_str());
}

void CNetTcp::OnRecv(std::shared_ptr<RECVOVLP> pOvlp)
{
	std::basic_string<char> str((char*)pOvlp->data, pOvlp->dwBytes);
	std::basic_string<wchar_t> wstr;
	char2wchar(str, wstr);
	LogWrite(INFO, _T("OnRecv %s"), wstr.c_str());

	{
		std::unique_lock<std::mutex> lock(m_nMutex);
		auto pit = m_lLinkList.find(pOvlp->hSock);
		if (pit == m_lLinkList.end()) {
			return;
		}
		if (!pit->second->Recv()) {
			pit->second->Close();
		}
	}
}

void CNetTcp::OnAcpt(std::shared_ptr<ACPTOVLP> pOvlp)
{
	TcpLink pAccept(new CTcpLink(m_hIOCP, pOvlp->hAcceptSock));
	{
		std::unique_lock<std::mutex> lock(m_nMutex);
		auto it = m_lLinkList.find(pOvlp->hSock);
		if (it != m_lLinkList.end()) {
			it->second->Accept();
		}
		LogWrite(INFO, _T("OnAcpt Link %d"), pAccept->GetSocket());
		m_lLinkList.insert(std::make_pair(pAccept->GetSocket(), pAccept));
	}

	//SOCKADDR* psaLocal = NULL;
	//SOCKADDR* psaRemote = NULL;
	//int nLocalLen, nRemoteLen;
	//GetAcceptExSockaddrs(pOvlp->data, 0,
	//	sizeof(SOCKADDR_IN) + 16,
	//	sizeof(SOCKADDR_IN) + 16,
	//	&psaLocal, &nLocalLen,
	//	&psaRemote, &nRemoteLen);

	if (!pAccept->Recv()) {
		pAccept->Close();
	}
}

void CNetTcp::OnClos(std::shared_ptr<CLOSOVLP> pOvlp)
{
	closesocket(pOvlp->hSock);
	{
		std::unique_lock<std::mutex> lock(m_nMutex);
		m_lLinkList.erase(pOvlp->hSock);
	}
}