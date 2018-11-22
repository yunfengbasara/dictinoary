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
		m_lIocpPool.push_back(std::thread(&CNetTcp::IOCPRoutine, this, m_hIOCP));
	}

	// test
	TcpLink server(new CTcpLink(m_hIOCP));
	server->CreateServer("", 8888);
	server->Listen();

	// 提前放如队列
	m_lLinkList.insert(std::make_pair(server->GetSocket(), server));

	for (int i = 0; i < 10; i++) {
		TcpLink client(new CTcpLink(m_hIOCP));
		client->CreateClient();	
		{
			std::unique_lock<std::mutex> lock(m_nMutex);
			m_lLinkList.insert(std::make_pair(client->GetSocket(), client));
		}
		client->Connect("", 8888);
	}
	
	//LogWrite(INFO, _T("Server Link %d"), server->GetSocket());
	//LogWrite(INFO, _T("Client Link %d"), client->GetSocket());

	return true;
}

bool CNetTcp::Uninit()
{
	for (auto &pThread : m_lIocpPool) {
		// win10 sdk可直接使用智能指针
		EXITOVLP *pOvlp = new EXITOVLP;
		::PostQueuedCompletionStatus(m_hIOCP, 0, 0, &pOvlp->ovlp);
	}

	for (auto &pThread : m_lIocpPool) {
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

CNetTcp::TcpLink CNetTcp::FindLink(SOCKET hSocket)
{
	std::unique_lock<std::mutex> lock(m_nMutex);
	auto pit = m_lLinkList.find(hSocket);
	if (pit == m_lLinkList.end()) {
		return NULL;
	}
	return pit->second;
}

void CNetTcp::OnConn(std::shared_ptr<CONNOVLP> pOvlp)
{
	// connectex回调后设置socket属性
	setsockopt(pOvlp->hSock, SOL_SOCKET, 
		SO_UPDATE_CONNECT_CONTEXT, NULL, 0);

	TcpLink pLink = FindLink(pOvlp->hSock);
	if (pLink == NULL) {
		return;
	}

	SOCKADDR_IN local;
	ZeroMemory(&local, sizeof(local));
	int locallen = sizeof(SOCKADDR_IN);
	getsockname(pOvlp->hSock, (PSOCKADDR)&local, &locallen);

	SOCKADDR_IN remote;
	ZeroMemory(&remote, sizeof(remote));
	int remotelen = sizeof(SOCKADDR_IN);
	getpeername(pOvlp->hSock, (PSOCKADDR)&remote, &remotelen);

	if (!pLink->Recv()) {
		pLink->Close();
		return;
	}

	pLink->SendPkt();
	pLink->Close();
}

void CNetTcp::OnSend(std::shared_ptr<SENDOVLP> pOvlp)
{
	TcpLink pLink = FindLink(pOvlp->hSock);
	if (pLink == NULL) {
		return;
	}

	if (!pLink->OnSendStream(
		pOvlp->data, pOvlp->dwBytes)) {
		pLink->Close();
	}
}

void CNetTcp::OnRecv(std::shared_ptr<RECVOVLP> pOvlp)
{
	TcpLink pLink = FindLink(pOvlp->hSock);
	if (pLink == NULL) {
		return;
	}

	if (!pLink->OnRecvStream(
		pOvlp->data, pOvlp->dwBytes)) {
		pLink->Close();
	}
}

void CNetTcp::OnAcpt(std::shared_ptr<ACPTOVLP> pOvlp)
{
	setsockopt(pOvlp->hAcceptSock, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, 
		(char*)&pOvlp->hSock, sizeof(pOvlp->hSock));

	// 监听socket再次发送accept事件
	TcpLink pLink = FindLink(pOvlp->hSock);
	if (pLink != NULL) {
		pLink->Accept();
	}

	// 插入新socket
	{
		//LogWrite(INFO, _T("OnAcpt Link %d"), pOvlp->hAcceptSock);
		std::unique_lock<std::mutex> lock(m_nMutex);
		TcpLink pAccept(new CTcpLink(m_hIOCP, pOvlp->hAcceptSock));
		m_lLinkList.insert(std::make_pair(pAccept->GetSocket(), pAccept));

		SOCKADDR_IN* psaLocal = NULL;
		SOCKADDR_IN* psaRemote = NULL;
		int nLocalLen, nRemoteLen;
		GetAcceptExSockaddrs(pOvlp->data, 0,
			sizeof(SOCKADDR_IN) + 16,
			sizeof(SOCKADDR_IN) + 16,
			(PSOCKADDR*)&psaLocal, &nLocalLen,
			(PSOCKADDR*)&psaRemote, &nRemoteLen);

		if (!pAccept->Recv()) {
			pAccept->Close();
		}
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