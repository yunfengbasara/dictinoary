#pragma once

// 自定义数据包解析
// 基本格式 HEAD + BODY
class CPacket
{
public:
	CPacket();
	~CPacket();

	const static uint32_t	HEAD_SIGN	= 0xFEEFFEEF;		// 数据包头部标识
	const static uint32_t	HEAD_LEN	= 16;				// 数据包头长度

	// 包头格式
	struct PKTHEAD {
		uint32_t	m_nSign		= HEAD_SIGN;		// 数据头标识[默认空]
		uint32_t	m_nFlag		= 0;				// encryption + zip
		uint32_t	m_nPktId	= 0;				// 数据id
		uint32_t	m_nPktLen	= HEAD_LEN;			// body长度(包括头部长度)
	};

	// 整包格式
	struct PKT {
		PKTHEAD						m_nHead;		// 包头
		std::basic_string<BYTE>		m_nBody;		// 包体
	};

	void   Packet(std::shared_ptr<PKT> pPkt);		// 打包
	void   Unpacket(PBYTE pData, uint32_t len);		// 解包
	size_t GetRecvBytesLen();
	size_t GetSendBytesLen();
	bool   GetSendBytes(std::basic_string<BYTE>&, uint32_t);
	bool   EraseSendBytes(uint32_t);

private:
	bool CheckPKT(std::basic_string<BYTE>&);		// 检查包体
	std::shared_ptr<CPacket::PKT> AnalyzePKT(		// 解析整包
		std::basic_string<BYTE>&);					

private:
	std::mutex						m_nRecvMtx;
	std::basic_string<BYTE>			m_nRecvBytes;	// 已经接收的数据缓存
	std::list<std::shared_ptr<PKT>>	m_lstRecvPkt;	// 已经接收的完整数据包队列

	std::mutex						m_nSendMtx;
	std::basic_string<BYTE>			m_nSendBytes;	// 即将发送的数据缓存
};