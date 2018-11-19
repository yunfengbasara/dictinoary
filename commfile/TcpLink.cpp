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
	m_hSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_hSock == INVALID_SOCKET) {
		return false;
	}
	
	// 立即关闭模式
	struct linger lgr;
	lgr.l_onoff = TRUE;
	lgr.l_linger = 0;
	setsockopt(m_hSock, SOL_SOCKET, SO_LINGER, (const char *)&lgr, sizeof(struct linger));

	HANDLE h = CreateIoCompletionPort((HANDLE)m_hSock, m_hIOCP, 0, 0);
	if (h != m_hIOCP) {
		closesocket(m_hSock);
		return false;
	}

	return true;
}

bool CTcpLink::CreateServer(const std::string addr, uint32_t port)
{
	m_hSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_hSock == INVALID_SOCKET) {
		return false;
	}

	BOOL bNoDelay = TRUE;
	setsockopt(m_hSock, IPPROTO_TCP, TCP_NODELAY, (const char*)&bNoDelay, sizeof(bNoDelay));

	struct linger lgr;
	lgr.l_onoff = TRUE;
	lgr.l_linger = 0;
	setsockopt(m_hSock, SOL_SOCKET, SO_LINGER, (const char *)&lgr, sizeof(struct linger));

	SOCKADDR_IN saLocal;
	saLocal.sin_family = AF_INET;
	saLocal.sin_port = htons(port);
	if (addr == "") {
		LPHOSTENT lphost = gethostbyname("");
		if (lphost == NULL) {
			return false;
		}
		char* pip = inet_ntoa(*(struct in_addr *) *lphost->h_addr_list);
		saLocal.sin_addr.s_addr = inet_addr(pip);
		//saLocal.sin_addr.s_addr = htonl(INADDR_ANY);
	}	
	else{
		saLocal.sin_addr.s_addr = inet_addr(addr.c_str());
	}

	int nRet = bind(m_hSock, (SOCKADDR *)&saLocal, sizeof(SOCKADDR_IN));
	if (nRet == SOCKET_ERROR){
		closesocket(m_hSock);
		return false;
	}

	HANDLE h = CreateIoCompletionPort((HANDLE)m_hSock, m_hIOCP, 0, 0);
	if (h != m_hIOCP) {
		closesocket(m_hSock);
		return false;
	}

	return true;
}

void CTcpLink::Destroy()
{
	if (m_hSock == INVALID_SOCKET) {
		return;
	}

	shutdown(m_hSock, SD_SEND);
	closesocket(m_hSock);
	m_hSock = INVALID_SOCKET;
}

bool CTcpLink::Connect(const std::string addr, uint32_t port)
{
	unsigned long saddr = inet_addr(addr.c_str());
	if (saddr == INADDR_NONE) {
		LPHOSTENT lphost = gethostbyname(addr.c_str());
		if (lphost == NULL) {
			return false;
		}
		saddr = ((LPIN_ADDR)lphost->h_addr)->s_addr;
	}

	SOCKADDR_IN saAddr;
	saAddr.sin_family = AF_INET;
	saAddr.sin_port = htons(port);
	saAddr.sin_addr.s_addr = saddr;

	int nRet = connect(m_hSock, (PSOCKADDR)&saAddr, sizeof(saAddr));
	if (nRet == SOCKET_ERROR){
		return false;
	}

	// 给IOCP发送接收消息信号
	NotifyRecv();
	return true;
}

bool CTcpLink::Listen()
{
	// 如果作为服务器监听
	SYSTEM_INFO si;
	::GetSystemInfo(&si);
	uint32_t nBlock = si.dwNumberOfProcessors * 2;

	DWORD dwRet = listen(m_hSock, nBlock);
	if (dwRet == SOCKET_ERROR) {
		return false;
	}

	for (uint32_t n = 0; n < nBlock; n++) {
		NotifyAccept();
	}

	return true;
}

bool CTcpLink::NotifyRecv()
{
	RECVOVLP *pOvlp = new RECVOVLP;
	pOvlp->event = NET_RECV;
	pOvlp->hSock = m_hSock;

	DWORD dwBytes = 0;
	DWORD dwFlags = 0;
	WSABUF wsaBufs[1];
	wsaBufs[0].buf = (char*)pOvlp->data;
	wsaBufs[0].len = IOBUF_RECVMAXSIZE;

	if (WSARecv(pOvlp->hSock, wsaBufs, 1,
		&dwBytes, &dwFlags, &pOvlp->ovlp,
		NULL) != SOCKET_ERROR) {
		return true;
	}

	int error = WSAGetLastError();
	if (error == WSA_IO_PENDING) {
		return true;
	}

	return false;
}

bool CTcpLink::NotifySend(const char* pData, uint32_t cbSize)
{
	SENDOVLP *pOvlp = new SENDOVLP;
	pOvlp->event = NET_SEND;
	pOvlp->hSock = m_hSock;
	memcpy(pOvlp->data, pData, cbSize);
	pOvlp->dwBytes = cbSize;

	WSABUF wsaBufs[1];
	wsaBufs[0].buf = (char*)pOvlp->data;
	wsaBufs[0].len = pOvlp->dwBytes;

	if (WSASend(pOvlp->hSock, wsaBufs, 1,
		&pOvlp->dwBytes, 0, &pOvlp->ovlp,
		NULL) != SOCKET_ERROR) {
		return true;
	}

	int error = WSAGetLastError();
	if (error == WSA_IO_PENDING) {
		return true;
	}

	return false;
}

bool CTcpLink::NotifyClose()
{
	OVLPSTRUCT *pOvlp = NULL;

	pOvlp->hSock = m_hSock;
	pOvlp->event = NET_CLOS;

	if (!::PostQueuedCompletionStatus(m_hIOCP, 0, 0, &pOvlp->ovlp)) {
		return false;
	}

	return true;
}

bool CTcpLink::NotifyAccept()
{
	SOCKET hSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hSock == INVALID_SOCKET) {
		return false;
	}

	ACCEPTOVLP *pOvlp = new ACCEPTOVLP;
	pOvlp->event = NET_ACPT;
	pOvlp->hSock = m_hSock;
	pOvlp->hAcceptSock = hSock;

	if (::AcceptEx(pOvlp->hSock, pOvlp->hAcceptSock,
		pOvlp->data, 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, &pOvlp->dwBytes,
		&pOvlp->ovlp)) {
		return true;
	}

	// 错误处理一下,过滤掉WSA_IO_PENDING
	int error = WSAGetLastError();
	if (error == WSA_IO_PENDING) {
		return true;
	}

	return false;
}