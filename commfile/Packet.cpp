#include "stdafx.h"
#include "Packet.h"

CPacket::CPacket()
{
}

CPacket::~CPacket()
{
}

bool CPacket::SendPkt(std::shared_ptr<PKT> pPkt)
{
	std::unique_lock<std::mutex> lock(m_nSendMtx);
	m_lstSendPkt.push_back(pPkt);
	return true;
}

bool CPacket::Unpacket(PBYTE pData, uint32_t len)
{
	std::unique_lock<std::mutex> lock(m_nRecvMtx);

	// 收到的数据放入缓存中
	m_nRecvBytes.append(pData, len);

	

	return true;
}

bool CPacket::CheckPkt(std::basic_string<BYTE>& cache)
{
	size_t len = cache.size();
	// check head sign
	if (len < 4) {
		uint32_t param;
		memcpy_s(&param, 4, cache.c_str(), 4);
		
	}

	// check head
	if (cache.size() < HEAD_LEN) {
		return false;
	}

	// check body

	return true;
}

bool CPacket::AnalyzeHead(std::basic_string<BYTE>& cache)
{
	return true;
}

bool CPacket::AnalyzeBody(std::basic_string<BYTE>& cache)
{
	return true;
}