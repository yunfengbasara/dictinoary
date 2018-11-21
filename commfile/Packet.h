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

	// 网络层的二进制流格式
	struct PKT {
		uint32_t	m_nSign		= HEAD_SIGN;		// 数据头标识[默认空]
		uint32_t	m_nFlag		= 0;				// encryption + zip
		uint32_t	m_nPktId	= 0;				// 数据id
		uint32_t	m_nPktLen	= 0;				// body长度
		PBYTE		m_pBody		= NULL;				// body数组
		~PKT() {
			delete[] m_pBody;
		}
	};

	bool SendPkt(std::shared_ptr<PKT> pPkt);		// 打包
	bool Unpacket(PBYTE pData, uint32_t len);		// 解包

private:
	bool CheckPkt(std::basic_string<BYTE>&);		// 检查包是否完整
	bool AnalyzeHead(std::basic_string<BYTE>&);		// 解析包头
	bool AnalyzeBody(std::basic_string<BYTE>&);		// 解析包体

private:
	std::mutex						m_nRecvMtx;
	std::basic_string<BYTE>			m_nRecvBytes;	// 已经接收的数据缓存
	std::list<std::shared_ptr<PKT>>	m_lstRecvPkt;	// 已经接收的完整数据包队列

	std::mutex						m_nSendMtx;
	std::basic_string<BYTE>			m_nSendBytes;	// 即将发送的数据缓存
	std::list<std::shared_ptr<PKT>>	m_lstSendPkt;	// 即将发送的完整数据包队列
};