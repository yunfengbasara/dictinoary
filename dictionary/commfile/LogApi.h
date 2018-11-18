#pragma once

enum LOGOUTTYPE {			// 日志输出目标
	SCREEN		= 1,		// CMD窗口
	DISKFILE	= 2,		// 文件
	VSDEBUG		= 4,		// VS DebugOutput
};

enum LOGOUTLV {				// 日志输出级别
	INFO		= 1,		// 普通消息
	WARN		= 2,		// 警告
	EROR		= 4,		// 错误
	DEBUG		= 8,		// 调试
};

enum LOGOUTHEAD {			// 日志输出头部
	LOGLINE		= 1,		// 日志行数
	TIMESTAMP	= 2,		// 时间
	THREADID	= 4,		// 线程ID
	CODEFILE	= 8,		// 源码文件
	CODELINE	= 16,		// 源码行数
	FUNCTION	= 32,		// 函数名
};

// 日志文件名称格式
const TCHAR		FMT_LOGNAME[]		=	_T("%04u%02u%02u.log");

// 日志输出格式
const TCHAR		FMT_LOGLINE[]		=	_T("[%06u]");
const TCHAR		FMT_TIMESTAMP[]		=	_T("[%02u:%02u:%02u.%03u]");
const TCHAR		FMT_THREADID[]		=	_T("[tid:%04x]");
const TCHAR		FMT_CODEFILE[]		=	_T("[file:%s]");
const TCHAR		FMT_CODELINE[]		=	_T("[line:%u]");
const TCHAR		FMT_FUNCTION[]		=	_T("[func:%s]");

// LOG日志单行消息
struct LogData {
	unsigned int					m_nCodeLine =	0;
	unsigned int					m_nLevel	=	INFO;
	std::basic_string<TCHAR>		m_sCodeFile	=	_T("");
	std::basic_string<TCHAR>		m_nFuncName =	_T("");;
	std::basic_string<TCHAR>		m_sLogMsg	=	_T("");;
};

// 日志内部调用
void GenericStart();
void GenericWrite(LPCSTR pfilename, unsigned int nLine, LPCSTR pfuncname,
	unsigned int nLevel, LPCTSTR pFmt, ...);
void GenericStop();

// 调用以下函数即可
#define	LogStart() GenericStart()	// 必须让LOG线程先启动创建日志文件
#define LogWrite(nLevel, pFmt, ...) GenericWrite(__FILE__, __LINE__, __FUNCTION__, nLevel, pFmt, __VA_ARGS__)
#define	LogStop() GenericStop()
