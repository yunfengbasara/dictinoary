#include "stdafx.h"
#include "TcpLink.h"
#include "LogApi.h"

CTcpLink::CTcpLink(HANDLE pIOCP, SOCKET hSocket)
	: m_hSock(hSocket)
	, m_hIOCP(pIOCP)
{
	if (m_hSock != INVALID_SOCKET) {
		Create();
	}
}

CTcpLink::~CTcpLink()
{
}

SOCKET&	CTcpLink::GetSocket()
{
	return m_hSock;
}

bool CTcpLink::Create()
{
	if (m_hSock == INVALID_SOCKET) {
		return false;
	}

	// 服务器accept产生的socket
	// 立即关闭模式, 不用等待发送完毕
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

bool CTcpLink::CreateClient()
{
	if (m_hSock != INVALID_SOCKET) {
		return false;
	}

	m_hSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_hSock == INVALID_SOCKET) {
		return false;
	}
	
	// 客户端产生的socket
	// 等待发送完毕再关闭
	struct linger lgr;
	lgr.l_onoff = FALSE;
	lgr.l_linger = 1;
	setsockopt(m_hSock, SOL_SOCKET, SO_LINGER, (const char *)&lgr, sizeof(struct linger));

	HANDLE h = CreateIoCompletionPort((HANDLE)m_hSock, m_hIOCP, 0, 0);
	if (h != m_hIOCP) {
		closesocket(m_hSock);
		return false;
	}

	return true;
}

bool CTcpLink::Connect(const std::string addr, uint32_t port)
{
	if (m_hSock == INVALID_SOCKET) {
		return false;
	}
	// connectEx函数通知IOCP
	// 如果没有connectEx函数用connect
	// connect之后记得NotifyRecv()
	int rc = 0;
	LPFN_CONNECTEX ConnectEx;
	GUID guid = WSAID_CONNECTEX;
	DWORD dwBytes;
	rc = WSAIoctl(m_hSock, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid, sizeof(guid), &ConnectEx, sizeof(ConnectEx),
		&dwBytes, NULL, NULL);

	if (rc != 0) {
		return false;
	}

	// connectEx 函数需要先绑定
	SOCKADDR_IN saAddrinit;
	saAddrinit.sin_family = AF_INET;
	saAddrinit.sin_port = 0;
	saAddrinit.sin_addr.s_addr = NULL;
	if (SOCKET_ERROR == bind(m_hSock, (PSOCKADDR)&saAddrinit,
		sizeof(SOCKADDR_IN))) {
		return false;
	}

	// 构造目标地址和端口
	unsigned long saddr = inet_addr(addr.c_str());
	if (saddr == INADDR_NONE) {
		LPHOSTENT lphost = gethostbyname(addr.c_str());
		if (lphost == NULL) {
			return false;
		}
		saddr = ((LPIN_ADDR)lphost->h_addr)->s_addr;
	}

	SOCKADDR_IN saAddr;
	ZeroMemory(&saAddr, sizeof(saAddr));
	saAddr.sin_family = AF_INET;
	saAddr.sin_port = htons(port);
	saAddr.sin_addr.s_addr = saddr;

	// 构造IOCP通知
	CONNOVLP *pOvlp = new CONNOVLP;
	pOvlp->hSock = m_hSock;

	if (ConnectEx(pOvlp->hSock, (PSOCKADDR)&saAddr,
		sizeof(SOCKADDR_IN), NULL, 0, NULL, &pOvlp->ovlp)) {
		return true;
	}

	int error = WSAGetLastError();
	if (error == WSA_IO_PENDING) {
		return true;
	}

	return false;
}

bool CTcpLink::CreateServer(const std::string addr, uint32_t port)
{
	if (m_hSock != INVALID_SOCKET) {
		return false;
	}

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

	unsigned long saddr = inet_addr(addr.c_str());
	if (saddr == INADDR_NONE) {
		LPHOSTENT lphost = gethostbyname(addr.c_str());
		if (lphost == NULL) {
			return false;
		}
		saddr = ((LPIN_ADDR)lphost->h_addr)->s_addr;
		// 127.0.0.1
		//char* pip = inet_ntoa(*(struct in_addr *) *lphost->h_addr_list);
	}

	SOCKADDR_IN saLocal;
	saLocal.sin_family = AF_INET;
	saLocal.sin_port = htons(port);
	saLocal.sin_addr.s_addr = saddr;
	//saLocal.sin_addr.s_addr = htonl(INADDR_ANY);

	int nRet = bind(m_hSock, (PSOCKADDR)&saLocal, sizeof(SOCKADDR_IN));
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

bool CTcpLink::Listen()
{
	if (m_hSock == INVALID_SOCKET) {
		return false;
	}

	// 如果作为服务器监听
	SYSTEM_INFO si;
	::GetSystemInfo(&si);
	uint32_t nBlock = si.dwNumberOfProcessors * 2;

	DWORD dwRet = listen(m_hSock, nBlock);
	if (dwRet == SOCKET_ERROR) {
		return false;
	}

	for (uint32_t n = 0; n < nBlock; n++) {
		Accept();
	}

	return true;
}

bool CTcpLink::Accept()
{
	if (m_hSock == INVALID_SOCKET) {
		return false;
	}

	SOCKET hSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hSock == INVALID_SOCKET) {
		return false;
	}

	ACPTOVLP *pOvlp = new ACPTOVLP;
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

bool CTcpLink::Recv()
{
	if (m_hSock == INVALID_SOCKET) {
		return false;
	}

	RECVOVLP *pOvlp = new RECVOVLP;
	pOvlp->hSock = m_hSock;

	DWORD dwBytes = 0;
	DWORD dwFlags = 0;
	WSABUF wsaBufs[1];
	wsaBufs[0].buf = (char*)pOvlp->data;
	wsaBufs[0].len = IOBUF_RECVMAXSIZE;

	int ret = WSARecv(pOvlp->hSock, wsaBufs, 1,
		&dwBytes, &dwFlags, &pOvlp->ovlp, NULL);
	if (ret == SOCKET_ERROR) {
		return false;
	}

	int error = WSAGetLastError();
	if (error == WSA_IO_PENDING) {
		return true;
	}

	return false;
}

bool CTcpLink::Send(const char* pData, uint32_t cbSize)
{
	if (m_hSock == INVALID_SOCKET) {
		return false;
	}

	SENDOVLP *pOvlp = new SENDOVLP;
	pOvlp->hSock = m_hSock;
	memcpy(pOvlp->data, pData, cbSize);
	pOvlp->dwBytes = cbSize;

	WSABUF wsaBufs[1];
	wsaBufs[0].buf = (char*)pOvlp->data;
	wsaBufs[0].len = pOvlp->dwBytes;

	int ret = WSASend(pOvlp->hSock, wsaBufs, 1,
		&pOvlp->dwBytes, 0, &pOvlp->ovlp, NULL);
	if (ret == SOCKET_ERROR) {
		return false;
	}

	int error = WSAGetLastError();
	if (error == WSA_IO_PENDING) {
		return true;
	}

	return false;
}

bool CTcpLink::Close()
{
	if (m_hSock == INVALID_SOCKET) {
		return false;
	}

	int rc = 0;
	LPFN_DISCONNECTEX DisconnectEx;
	GUID guid = WSAID_DISCONNECTEX;
	DWORD dwBytes;
	rc = WSAIoctl(m_hSock, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid, sizeof(guid), &DisconnectEx, sizeof(DisconnectEx),
		&dwBytes, NULL, NULL);

	if (rc != 0) {
		return false;
	}

	CLOSOVLP *pOvlp = new CLOSOVLP;
	pOvlp->hSock = m_hSock;

	// DisconnectEx 效果同 shutdown(pOvlp->hSock, SD_BOTH);
	if (DisconnectEx(pOvlp->hSock, &pOvlp->ovlp, 0, 0)) {
		return true;
	}

	int error = WSAGetLastError();
	if (error == WSA_IO_PENDING) {
		return true;
	}

	return true;
}