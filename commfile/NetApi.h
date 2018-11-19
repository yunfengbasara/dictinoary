#pragma once

enum NET_EVENT {
	NET_INIT = 0x00,
	NET_SEND = 0x01,
	NET_RECV = 0x02,
	NET_ACPT = 0x04,
	NET_CLOS = 0x08,
};

#define IOBUF_SENDMAXSIZE	(4*1024)
#define IOBUF_RECVMAXSIZE	(4*1024)
#define IOBUF_ACCEPTMAXSIZE (1*1024)

// 完成端口自定义结构体
struct OVLPSTRUCT {
	OVERLAPPED ovlp = { 0 };
	NET_EVENT  event = NET_INIT;
	SOCKET	   hSock = INVALID_SOCKET;
	DWORD	   dwBytes = 0;
};

struct SENDOVLP : public OVLPSTRUCT
{
	BYTE data[IOBUF_SENDMAXSIZE] = { 0 };
};

struct RECVOVLP : public OVLPSTRUCT
{
	BYTE data[IOBUF_RECVMAXSIZE] = { 0 };
};

struct ACCEPTOVLP : public OVLPSTRUCT
{
	BYTE data[IOBUF_ACCEPTMAXSIZE] = { 0 };
	SOCKET     hAcceptSock = INVALID_SOCKET;
};