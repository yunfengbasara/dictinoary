#pragma once
#include "MsgThread.h"
#include "PostCmd.h"
#include "LogApi.h"
#include "Timer.h"

// 日志格式:[时间][INFO]message
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
	// 命令响应
	bool CreateLog();			// 创建日志文件
	bool ClearLog();			// 清理过期日志
	bool WriteLine(std::shared_ptr<LogData> pData);

private:
	// 定时器
	void Clear_Timer();

private:
	uint32_t				m_nOutType;		// 默认CMD、文件
	uint32_t				m_nOutLV;		// 默认全部级别
	uint32_t				m_nOutHead;		// 默认[日志行数][时间]

	uint32_t				m_nKeepDay;		// 日志保留天数,默认10天
	uint32_t				m_nLineCount;	// 日志行数
	std::ofstream			m_fLogFile;		// 日志文件 char 版本的ofstream支持写入中文

	uint32_t				m_nNowDay;		// 当前日期(用来判断是否需要创建新日志)
};

