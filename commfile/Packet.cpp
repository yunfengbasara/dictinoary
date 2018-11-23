#include "stdafx.h"
#include "Packet.h"
#include "convert.h"
#include "LogApi.h"

CPacket::CPacket()
{
}

CPacket::~CPacket()
{
}

void CPacket::Packet(std::shared_ptr<PKT> pPkt)
{
	std::unique_lock<std::mutex> lock(m_nSendMtx);

	// �����ֽ���ת����
	pPkt->m_nHead.m_nSign = htonl(pPkt->m_nHead.m_nSign);
	pPkt->m_nHead.m_nFlag = htonl(pPkt->m_nHead.m_nFlag);
	pPkt->m_nHead.m_nPktId = htonl(pPkt->m_nHead.m_nPktId);
	pPkt->m_nHead.m_nPktLen = htonl(pPkt->m_nBody.size() + HEAD_LEN);
	
	m_nSendBytes.append((PBYTE)&pPkt->m_nHead.m_nSign, sizeof(uint32_t));
	m_nSendBytes.append((PBYTE)&pPkt->m_nHead.m_nFlag, sizeof(uint32_t));
	m_nSendBytes.append((PBYTE)&pPkt->m_nHead.m_nPktId, sizeof(uint32_t));
	m_nSendBytes.append((PBYTE)&pPkt->m_nHead.m_nPktLen, sizeof(uint32_t));
	m_nSendBytes.append((PBYTE)pPkt->m_nBody.c_str(), pPkt->m_nBody.size());
}

void CPacket::Unpacket(PBYTE pData, uint32_t len)
{
	std::unique_lock<std::mutex> lock(m_nRecvMtx);

	// �յ������ݷ��뻺����
	m_nRecvBytes.append(pData, len);

	// ѭ��������(�����п��ܴ��ڶ����)
	while (CheckPKT(m_nRecvBytes)) {
		auto pPkt = AnalyzePKT(m_nRecvBytes);
		m_lstRecvPkt.push_back(pPkt);

		// ɾ���Ѿ������Ļ���
		m_nRecvBytes.erase(0, pPkt->m_nHead.m_nPktLen);

		std::basic_string<char> str((char*)pPkt->m_nBody.c_str(), pPkt->m_nBody.size());
		std::basic_string<wchar_t> wstr;
		char2wchar(str, wstr);
		LogWrite(INFO, _T("packet %s"), wstr.c_str());
	}
}

size_t CPacket::GetRecvBytesLen()
{
	std::unique_lock<std::mutex> lock(m_nRecvMtx);
	return m_nRecvBytes.size();
}

size_t CPacket::GetSendBytesLen()
{
	std::unique_lock<std::mutex> lock(m_nSendMtx);
	return m_nSendBytes.size();
}

bool CPacket::GetSendBytes(
	std::basic_string<BYTE>& nSendBytes, 
	uint32_t nSendSize)
{
	std::unique_lock<std::mutex> lock(m_nSendMtx);
	if (m_nSendBytes.size() < nSendSize) {
		return false;
	}
	nSendBytes.append(m_nSendBytes.c_str(), nSendSize);
	return true;
}

bool CPacket::EraseSendBytes(uint32_t nSendSize)
{
	std::unique_lock<std::mutex> lock(m_nSendMtx);
	if (m_nSendBytes.size() < nSendSize) {
		return false;
	}
	m_nSendBytes.erase(0, nSendSize);
	return true;
}

bool CPacket::CheckPKT(std::basic_string<BYTE>& cache)
{
	std::basic_stringstream<BYTE> readStream(cache);

	PKTHEAD head;
	readStream.read((PBYTE)&head, sizeof(PKTHEAD));
	// ͷ����������ͷ�����Ȳ�����
	if (readStream.eof()) {
		return false;
	}

	// �����ֽ���ת����
	head.m_nFlag = ntohl(head.m_nFlag);
	head.m_nPktId = ntohl(head.m_nPktId);
	head.m_nPktLen = ntohl(head.m_nPktLen);
	head.m_nSign = ntohl(head.m_nSign);

	// ���Ǳ�Ӧ�ñ�ʶ
	if (head.m_nSign != HEAD_SIGN) {
		return false;
	}
	// ��������С��ͷ������
	if (head.m_nPktLen < HEAD_LEN) {
		return false;
	}
	// ���泤�Ȳ�����������
	if (cache.size() < head.m_nPktLen) {
		return false;
	}
	return true;
}

std::shared_ptr<CPacket::PKT> CPacket::AnalyzePKT(
	std::basic_string<BYTE>& cache)
{
	std::basic_stringstream<BYTE> readStream(cache);

	std::shared_ptr<PKT> pPkt(new PKT);
	readStream.read((PBYTE)&pPkt->m_nHead, sizeof(PKTHEAD));

	// �����ֽ���ת����
	pPkt->m_nHead.m_nFlag = ntohl(pPkt->m_nHead.m_nFlag);
	pPkt->m_nHead.m_nPktId = ntohl(pPkt->m_nHead.m_nPktId);
	pPkt->m_nHead.m_nPktLen = ntohl(pPkt->m_nHead.m_nPktLen);
	pPkt->m_nHead.m_nSign = ntohl(pPkt->m_nHead.m_nSign);

	// body����ȥ��ͷ����
	uint32_t bodyLen = pPkt->m_nHead.m_nPktLen - HEAD_LEN;
	pPkt->m_nBody.append(cache.c_str() + sizeof(PKTHEAD), bodyLen);

	return pPkt;
}