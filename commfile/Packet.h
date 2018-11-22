#pragma once

// �Զ������ݰ�����
// ������ʽ HEAD + BODY
class CPacket
{
public:
	CPacket();
	~CPacket();

	const static uint32_t	HEAD_SIGN	= 0xFEEFFEEF;		// ���ݰ�ͷ����ʶ
	const static uint32_t	HEAD_LEN	= 16;				// ���ݰ�ͷ����

	// ��ͷ��ʽ
	struct PKTHEAD {
		uint32_t	m_nSign		= HEAD_SIGN;		// ����ͷ��ʶ[Ĭ�Ͽ�]
		uint32_t	m_nFlag		= 0;				// encryption + zip
		uint32_t	m_nPktId	= 0;				// ����id
		uint32_t	m_nPktLen	= HEAD_LEN;			// body����(����ͷ������)
	};

	// ������ʽ
	struct PKT {
		PKTHEAD						m_nHead;		// ��ͷ
		std::basic_string<BYTE>		m_nBody;		// ����
	};

	void   Packet(std::shared_ptr<PKT> pPkt);		// ���
	void   Unpacket(PBYTE pData, uint32_t len);		// ���
	size_t GetRecvBytesLen();
	size_t GetSendBytesLen();
	bool   GetSendBytes(std::basic_string<BYTE>&, uint32_t);
	bool   EraseSendBytes(uint32_t);

private:
	bool CheckPKT(std::basic_string<BYTE>&);		// ������
	std::shared_ptr<CPacket::PKT> AnalyzePKT(		// ��������
		std::basic_string<BYTE>&);					

private:
	std::mutex						m_nRecvMtx;
	std::basic_string<BYTE>			m_nRecvBytes;	// �Ѿ����յ����ݻ���
	std::list<std::shared_ptr<PKT>>	m_lstRecvPkt;	// �Ѿ����յ��������ݰ�����

	std::mutex						m_nSendMtx;
	std::basic_string<BYTE>			m_nSendBytes;	// �������͵����ݻ���
};