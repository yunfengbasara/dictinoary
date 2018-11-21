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

	// �����Ķ���������ʽ
	struct PKT {
		uint32_t	m_nSign		= HEAD_SIGN;		// ����ͷ��ʶ[Ĭ�Ͽ�]
		uint32_t	m_nFlag		= 0;				// encryption + zip
		uint32_t	m_nPktId	= 0;				// ����id
		uint32_t	m_nPktLen	= 0;				// body����
		PBYTE		m_pBody		= NULL;				// body����
		~PKT() {
			delete[] m_pBody;
		}
	};

	bool SendPkt(std::shared_ptr<PKT> pPkt);		// ���
	bool Unpacket(PBYTE pData, uint32_t len);		// ���

private:
	bool CheckPkt(std::basic_string<BYTE>&);		// �����Ƿ�����
	bool AnalyzeHead(std::basic_string<BYTE>&);		// ������ͷ
	bool AnalyzeBody(std::basic_string<BYTE>&);		// ��������

private:
	std::mutex						m_nRecvMtx;
	std::basic_string<BYTE>			m_nRecvBytes;	// �Ѿ����յ����ݻ���
	std::list<std::shared_ptr<PKT>>	m_lstRecvPkt;	// �Ѿ����յ��������ݰ�����

	std::mutex						m_nSendMtx;
	std::basic_string<BYTE>			m_nSendBytes;	// �������͵����ݻ���
	std::list<std::shared_ptr<PKT>>	m_lstSendPkt;	// �������͵��������ݰ�����
};