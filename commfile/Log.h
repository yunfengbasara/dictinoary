#pragma once
#include "MsgThread.h"
#include "PostCmd.h"
#include "LogApi.h"
#include "Timer.h"

// ��־��ʽ:[ʱ��][INFO]message
class CLog : public CMsgThread, public CTimer
{
public:
	static CLog* Instance();

	CLog();
	virtual ~CLog();
	virtual	bool Init() override;
	virtual bool UnInit() override;

public:
	CreatePostCmd(CLog, CreateLog)
	CreatePostCmd(CLog, ClearLog)
	CreatePostCmd(CLog, WriteLine)

private:
	// ������Ӧ
	bool CreateLog();			// ������־�ļ�
	bool ClearLog();			// ���������־
	bool WriteLine(std::shared_ptr<LogData> pData);

private:
	// ��ʱ��
	void Clear_Timer();

private:
	uint32_t				m_nOutType;		// Ĭ��CMD���ļ�
	uint32_t				m_nOutLV;		// Ĭ��ȫ������
	uint32_t				m_nOutHead;		// Ĭ��[��־����][ʱ��]

	uint32_t				m_nKeepDay;		// ��־��������,Ĭ��10��
	uint32_t				m_nLineCount;	// ��־����
	std::ofstream			m_fLogFile;		// ��־�ļ� char �汾��ofstream֧��д������

	uint32_t				m_nNowDay;		// ��ǰ����(�����ж��Ƿ���Ҫ��������־)
};

