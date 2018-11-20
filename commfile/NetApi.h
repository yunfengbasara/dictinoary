#pragma once

enum NET_EVENT {
	NET_INIT = 0x00,	// ��ʼ��
	NET_CONN = 0x01,	// connect��Ӧ
	NET_SEND = 0x02,	// SEND��Ӧ
	NET_RECV = 0x04,	// RECV��Ӧ
	NET_ACPT = 0x08,	// ACCEPT��Ӧ
	NET_CLOS = 0x10,	// ���ӹر�
	NET_EXIT = 0x20,	// �˳�ϵͳ
};

#define IOBUF_SENDMAXSIZE	(4*1024)
#define IOBUF_RECVMAXSIZE	(4*1024)
#define IOBUF_ACCEPTMAXSIZE (1*1024)

// ��ɶ˿��Զ���ṹ��
struct OVLPSTRUCT {
	OVERLAPPED ovlp = { 0 };
	NET_EVENT  event = NET_INIT;
	SOCKET	   hSock = INVALID_SOCKET;
	DWORD	   dwBytes = 0;
};

struct CONNOVLP : public OVLPSTRUCT
{
	CONNOVLP(){ event = NET_CONN; }
};

struct SENDOVLP : public OVLPSTRUCT
{
	BYTE data[IOBUF_SENDMAXSIZE] = { 0 };
	SENDOVLP() { event = NET_SEND; }
};

struct RECVOVLP : public OVLPSTRUCT
{
	BYTE data[IOBUF_RECVMAXSIZE] = { 0 };
	RECVOVLP() { event = NET_RECV; }
};

struct ACPTOVLP : public OVLPSTRUCT
{
	BYTE data[IOBUF_ACCEPTMAXSIZE] = { 0 };
	SOCKET     hAcceptSock = INVALID_SOCKET;
	ACPTOVLP() { event = NET_ACPT; }
};

struct CLOSOVLP : public OVLPSTRUCT
{
	CLOSOVLP() { event = NET_CLOS; }
};

struct EXITOVLP : public OVLPSTRUCT
{
	EXITOVLP() { event = NET_EXIT; }
};