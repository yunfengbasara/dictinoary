#include "stdafx.h"
#include "TcpLink.h"
#include "LogApi.h"

CTcpLink::CTcpLink(HANDLE pIOCP, SOCKET hSocket)
	: m_hSock(hSocket)
	, m_hIOCP(pIOCP)
	, m_pPkt(new CPacket)
{
}

CTcpLink::~CTcpLink()
{
	//static int io = 0;
	//io++;
	//LogWrite(INFO, _T("CTcpLink clean %d"), io);
}

SOCKET&	CTcpLink::GetSocket()
{
	return m_hSock;
}

bool CTcpLink::CreateClient()
{
	if (m_hSock == INVALID_SOCKET) {
		m_hSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
			NULL, 0, WSA_FLAG_OVERLAPPED);
	}

	if (m_hSock == INVALID_SOCKET) {
		return false;
	}

	// 立即关闭模式, 不用等待发送完毕
	struct linger lgr;
	lgr.l_onoff = TRUE;
	lgr.l_linger = 0;
	setsockopt(m_hSock, SOL_SOCKET, SO_LINGER,
		(const char *)&lgr, sizeof(struct linger));

	HANDLE h = CreateIoCompletionPort((HANDLE)m_hSock, m_hIOCP, 0, 0);
	if (h != m_hIOCP) {
		closesocket(m_hSock);
		return false;
	}

	return true;
}

bool CTcpLink::Connect(const std::string addr, uint32_t port)
{
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
	// 如果作为服务器监听
	SYSTEM_INFO si;
	::GetSystemInfo(&si);
	uint32_t nBlock = si.dwNumberOfProcessors * 2;

	DWORD dwRet = listen(m_hSock, nBlock);
	if (dwRet == SOCKET_ERROR) {
		return false;
	}

	for (uint32_t n = 0; n < nBlock; n++) {
		if (!Accept()) {
			return false;
		}
	}

	return true;
}

bool CTcpLink::Accept()
{
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
	RECVOVLP *pOvlp = new RECVOVLP;
	pOvlp->hSock = m_hSock;

	DWORD dwBytes = 0;
	DWORD dwFlags = 0;
	WSABUF wsaBufs[1];
	wsaBufs[0].buf = (char*)pOvlp->data;
	wsaBufs[0].len = IOBUF_RECVMAXSIZE;

	int ret = WSARecv(pOvlp->hSock, wsaBufs, 1,
		&dwBytes, &dwFlags, &pOvlp->ovlp, NULL);
	if (ret == 0) {
		return true;
	}

	int error = WSAGetLastError();
	if (error == WSA_IO_PENDING ||
		error == 0) {
		return true;
	}

	return false;
}

bool CTcpLink::Close()
{
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

void CTcpLink::SendPkt()
{
	BYTE temp[] = { "hello world" };

	std::shared_ptr<CPacket::PKT> pPkt(new CPacket::PKT);
	pPkt->m_nBody = temp;
	m_pPkt->Packet(pPkt);

	uint32_t sendBufferSize = m_pPkt->GetSendBytesLen();
	if (sendBufferSize == 0) {
		return;
	}

	uint32_t sendLen = min(IOBUF_SENDMAXSIZE, sendBufferSize);

	std::basic_string<BYTE> needSendBuf;
	if (!m_pPkt->GetSendBytes(needSendBuf, sendLen)) {
		return;
	}

	if (!Send(needSendBuf)) {
		return;
	}
}

bool CTcpLink::OnSend(const PBYTE pData, uint32_t len)
{
	if (!m_pPkt->EraseSendBytes(len)) {
		return false;
	}

	uint32_t sendBufferSize = m_pPkt->GetSendBytesLen();
	if (sendBufferSize == 0) {
		return true;
	}

	uint32_t sendLen = min(IOBUF_SENDMAXSIZE, sendBufferSize);

	std::basic_string<BYTE> needSendBuf;
	if (!m_pPkt->GetSendBytes(needSendBuf, sendLen)) {
		return false;
	}

	if (!Send(needSendBuf)) {
		return false;
	}

	return true;
}

bool CTcpLink::OnRecv(const PBYTE pData, uint32_t len)
{
	if (len == 0) {
		// 这里是个关键点，如果返回接收数据为0
		// 服务器能判断客户端断开
		//LogWrite(EROR, _T("OnRecvStream Close %d"), m_hSock);
		return false;
	}

	m_pPkt->Unpacket(pData, len);

	// 处理业务


	return Recv();
}

bool CTcpLink::Send(const std::basic_string<BYTE>& sendBytes)
{
	SENDOVLP *pOvlp = new SENDOVLP;
	pOvlp->hSock = m_hSock;
	pOvlp->dwBytes = sendBytes.size();
	memcpy_s(pOvlp->data, IOBUF_SENDMAXSIZE, 
		sendBytes.c_str(), sendBytes.size());

	WSABUF wsaBufs[1];
	wsaBufs[0].buf = (char*)pOvlp->data;
	wsaBufs[0].len = pOvlp->dwBytes;

	int ret = WSASend(pOvlp->hSock, wsaBufs, 1,
		&pOvlp->dwBytes, 0, &pOvlp->ovlp, NULL);
	if (ret == 0) {
		return true;
	}

	int error = WSAGetLastError();
	if (error == WSA_IO_PENDING ||
		error == 0) {
		return true;
	}

	return false;
}