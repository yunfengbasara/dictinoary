#include "stdafx.h"
#include "LogApi.h"
#include "Log.h"

void GenericStart() 
{
	setlocale(LC_CTYPE, "");
	CLog::Instance()->PostCmd_CreateLog();
}

void GenericWrite(LPCSTR pfilename, unsigned int nLine, LPCSTR pfuncname,
	unsigned int nLevel, LPCTSTR pFmt, ...)
{
	// 生成消息格式
	std::shared_ptr<LogData> pData(new LogData);

	// 解析文件名
#if defined UNICODE
	TCHAR filename[MAX_PATH] = { 0 };
	MultiByteToWideChar(CP_ACP, 0, pfilename, strlen(pfilename), filename, sizeof(filename) / sizeof(TCHAR));
	pData->m_sCodeFile = filename;
#else
	pData->m_sCodeFile = pfilename;
#endif
	// 只需要提取函数名
	int namepos = pData->m_sCodeFile.find_last_of('\\');
	pData->m_sCodeFile = pData->m_sCodeFile.substr(namepos + 1);

	// 解析文件行数
	pData->m_nCodeLine = nLine;

	// 解析函数名
#if defined UNICODE
	TCHAR funcname[MAX_PATH] = { 0 };
	MultiByteToWideChar(CP_ACP, 0, pfuncname, strlen(pfuncname), funcname, sizeof(funcname) / sizeof(TCHAR));
	pData->m_nFuncName = funcname;
#else
	pData->m_nFuncName = pfuncname;
#endif

	// 解析消息等级
	pData->m_nLevel = nLevel;

	// 解析格式化消息
	TCHAR logline[MAX_PATH] = { 0 };
	int linelen = sizeof(logline) / sizeof(TCHAR);
	va_list argptr;
	va_start(argptr, pFmt);
	_vsntprintf_s(logline, linelen, _TRUNCATE, pFmt, argptr);
	va_end(argptr);
	pData->m_sLogMsg = logline;

	CLog::Instance()->PostCmd_WriteLine(pData);
}

void GenericStop()
{
	delete CLog::Instance();
}