#include "stdafx.h"
#include "Log.h"
#include "convert.h"

CLog* CLog::Instance()
{
	static CLog* g_IInstance = NULL;
	static std::once_flag oc;
	// 防止创建多个实例
	std::call_once(oc, [&] { g_IInstance = new CLog(); });
	return g_IInstance;
}

CLog::CLog()
	: m_nOutType(SCREEN | DISKFILE | VSDEBUG)
	, m_nOutLV(INFO | WARN | EROR | DEBUG)
	, m_nOutHead(LOGLINE | TIMESTAMP)
	, m_nKeepDay(10)
	, m_nLineCount(0)
	, m_nNowDay(0)
{
	Start();
}

CLog::~CLog()
{
	Stop();
}

bool CLog::Init()
{
	// 每小时执行一次清理日志操作
	RegisterTimer(CLog, Clear_Timer, 3600 * 1000);

	return true;
}

bool CLog::UnInit()
{
	UnregisterTimer(Clear_Timer);

	m_fLogFile.close();
	return true;
}

bool CLog::CreateLog()
{
	// 如果没有开启写入文件，不产生日志文件
	if ((m_nOutType & DISKFILE) == 0) {
		return true;
	}

	m_nLineCount = 0;
	m_fLogFile.close();

	TCHAR szModule[MAX_PATH] = { 0 };
	int modlen = sizeof(szModule) / sizeof(TCHAR);
	GetModuleFileName(NULL, szModule, modlen);

	TCHAR driver[MAX_PATH] = { 0 };
	TCHAR dir[MAX_PATH] = { 0 };
	TCHAR filename[MAX_PATH] = { 0 };
	TCHAR ext[MAX_PATH] = { 0 };
	_tsplitpath_s(szModule, driver, dir, filename, ext);

	// 日志文件名
	std::basic_string<TCHAR> strFile;
	_tmakepath_s(szModule, driver, dir, NULL, NULL);
	strFile = szModule;

	// 同级目录添加Log目录
	strFile += _T("Log");
	_tmkdir(strFile.c_str());

	// 打开日志文件
	SYSTEMTIME st;
	GetLocalTime(&st);
	m_nNowDay = st.wDay;

	TCHAR logname[MAX_PATH] = { 0 };
	int namelen = sizeof(logname) / sizeof(TCHAR);
	_stprintf_s(logname, namelen, FMT_LOGNAME, st.wYear, st.wMonth, st.wDay);

	strFile += _T("\\");
	strFile += filename;
	strFile += _T("_");
	strFile += logname;

	m_fLogFile.open(strFile.c_str(), std::ios::out | std::ios::app);

	return true;
}

bool CLog::ClearLog()
{
	std::basic_string<TCHAR> logpath;
	logpath = _tgetcwd(NULL, 0);
	auto templogpath = logpath + _T("\\Log\\*");

	// 遍历日志文件夹
	std::vector<std::basic_string<TCHAR>>	logfiles;
	long handle;
	struct _tfinddata_t fileinfo;
	handle = _tfindfirst(templogpath.c_str(), &fileinfo);
	if (handle == -1) {
		LogWrite(ERROR, _T("_tfindfirst error"));
		_findclose(handle);
		return true;
	}

	// 去掉文件夹
	if ((fileinfo.attrib & _A_ARCH) != 0) {
		logfiles.push_back(fileinfo.name);
	}
	
	while (_tfindnext(handle, &fileinfo) != -1) {
		if ((fileinfo.attrib & _A_ARCH) != 0) {
			logfiles.push_back(fileinfo.name);
		}
	}
	_findclose(handle);

	// 删除旧文件
	auto nowdate = std::chrono::system_clock::now();
	auto nowt = std::chrono::system_clock::to_time_t(nowdate);
	std::tm nowtm;
	localtime_s(&nowtm, &nowt);

	for (auto& logname : logfiles) {
		// 去掉文件名前面的dictionary_
		auto filename = logname.substr(logname.find(_T('_')) + 1);
		std::tm logtm = {};
		_stscanf_s(filename.c_str(), FMT_LOGNAME, &logtm.tm_year, &logtm.tm_mon, &logtm.tm_mday);
		logtm.tm_year -= 1900;
		logtm.tm_mon -= 1;
		// 应该加上时分秒
		logtm.tm_hour = nowtm.tm_hour;
		logtm.tm_min = nowtm.tm_min;
		logtm.tm_sec = nowtm.tm_sec;

		auto logdate = std::chrono::system_clock::from_time_t(mktime(&logtm));
		if (nowdate - logdate > std::chrono::hours(24 * m_nKeepDay)) {
			auto dellogpath = logpath + _T("\\Log\\") + logname;
			_tremove(dellogpath.c_str());
		}
	}

	return true;
}

bool CLog::WriteLine(std::shared_ptr<LogData> pData)
{
	// 判断输出级别
	if ((pData->m_nLevel & m_nOutLV) == 0) {
		return true;
	}

	m_nLineCount++;

	// 构造头部
	std::basic_string<TCHAR>	logLine;

	TCHAR msgtemp[MAX_PATH] = { 0 };
	int msglen = sizeof(msgtemp) / sizeof(TCHAR);

	if (m_nOutHead & LOGLINE) {
		_stprintf_s(msgtemp, msglen, FMT_LOGLINE, m_nLineCount);
		logLine += msgtemp;
	}

	if (m_nOutHead & TIMESTAMP) {
		SYSTEMTIME st;
		GetLocalTime(&st);
		_stprintf_s(msgtemp, msglen, FMT_TIMESTAMP, 
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		logLine += msgtemp;
	}

	if (m_nOutHead & THREADID) {
		_stprintf_s(msgtemp, msglen, FMT_THREADID, GetCurrentThreadId());
		logLine += msgtemp;
	}

	switch (pData->m_nLevel) {
	case INFO:	logLine += _T("[INFO]"); break;
	case WARN:	logLine += _T("[WARN]"); break;
	case EROR:	logLine += _T("[ERROR]"); break;
	case DEBUG:	logLine += _T("[DEBUG]"); break;
	}

	if (m_nOutHead & CODEFILE) {
		_stprintf_s(msgtemp, msglen, FMT_CODEFILE, pData->m_sCodeFile.c_str());
		logLine += msgtemp;
	}

	if (m_nOutHead & CODELINE) {
		_stprintf_s(msgtemp, msglen, FMT_CODELINE, pData->m_nCodeLine);
		logLine += msgtemp;
	}

	if (m_nOutHead & FUNCTION) {
		_stprintf_s(msgtemp, msglen, FMT_FUNCTION, pData->m_nFuncName.c_str());
		logLine += msgtemp;
	}

	logLine += _T(" ");
	logLine += pData->m_sLogMsg;
	logLine += _T("\n");

	// 根据输出标志位定向输出
	if (m_nOutType & SCREEN) {
		_tprintf(_T("%s"), logLine.c_str());
	}

	if (m_nOutType & DISKFILE) {
		std::string msg;
#if defined UNICODE
		wchar2UTF8(logLine, msg);
#else
		msg = logLine;
#endif
		m_fLogFile << msg.c_str();
		m_fLogFile.flush();
	}

	if (m_nOutType & VSDEBUG) {
		OutputDebugString(logLine.c_str());
	}

	return true;
}

void CLog::Clear_Timer()
{
	// 是否需要创建新日志
	SYSTEMTIME st;
	GetLocalTime(&st);
	if (m_nNowDay != st.wDay) {
		PostCmd_CreateLog();
	}

	// 清理旧日志
	PostCmd_ClearLog();
}