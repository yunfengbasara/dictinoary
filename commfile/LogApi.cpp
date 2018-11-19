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
	// ������Ϣ��ʽ
	std::shared_ptr<LogData> pData(new LogData);

	// �����ļ���
#if defined UNICODE
	TCHAR filename[MAX_PATH] = { 0 };
	MultiByteToWideChar(CP_ACP, 0, pfilename, strlen(pfilename), filename, sizeof(filename) / sizeof(TCHAR));
	pData->m_sCodeFile = filename;
#else
	pData->m_sCodeFile = pfilename;
#endif
	// ֻ��Ҫ��ȡ������
	int namepos = pData->m_sCodeFile.find_last_of('\\');
	pData->m_sCodeFile = pData->m_sCodeFile.substr(namepos + 1);

	// �����ļ�����
	pData->m_nCodeLine = nLine;

	// ����������
#if defined UNICODE
	TCHAR funcname[MAX_PATH] = { 0 };
	MultiByteToWideChar(CP_ACP, 0, pfuncname, strlen(pfuncname), funcname, sizeof(funcname) / sizeof(TCHAR));
	pData->m_nFuncName = funcname;
#else
	pData->m_nFuncName = pfuncname;
#endif

	// ������Ϣ�ȼ�
	pData->m_nLevel = nLevel;

	// ������ʽ����Ϣ
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