#pragma once

enum LOGOUTTYPE {			// ��־���Ŀ��
	SCREEN		= 1,		// CMD����
	DISKFILE	= 2,		// �ļ�
	VSDEBUG		= 4,		// VS DebugOutput
};

enum LOGOUTLV {				// ��־�������
	INFO		= 1,		// ��ͨ��Ϣ
	WARN		= 2,		// ����
	EROR		= 4,		// ����
	DEBUG		= 8,		// ����
};

enum LOGOUTHEAD {			// ��־���ͷ��
	LOGLINE		= 1,		// ��־����
	TIMESTAMP	= 2,		// ʱ��
	THREADID	= 4,		// �߳�ID
	CODEFILE	= 8,		// Դ���ļ�
	CODELINE	= 16,		// Դ������
	FUNCTION	= 32,		// ������
};

// ��־�ļ����Ƹ�ʽ
const TCHAR		FMT_LOGNAME[]		=	_T("%04u%02u%02u.log");

// ��־�����ʽ
const TCHAR		FMT_LOGLINE[]		=	_T("[%06u]");
const TCHAR		FMT_TIMESTAMP[]		=	_T("[%02u:%02u:%02u.%03u]");
const TCHAR		FMT_THREADID[]		=	_T("[tid:%04x]");
const TCHAR		FMT_CODEFILE[]		=	_T("[file:%s]");
const TCHAR		FMT_CODELINE[]		=	_T("[line:%u]");
const TCHAR		FMT_FUNCTION[]		=	_T("[func:%s]");

// LOG��־������Ϣ
struct LogData {
	unsigned int					m_nCodeLine =	0;
	unsigned int					m_nLevel	=	INFO;
	std::basic_string<TCHAR>		m_sCodeFile	=	_T("");
	std::basic_string<TCHAR>		m_nFuncName =	_T("");;
	std::basic_string<TCHAR>		m_sLogMsg	=	_T("");;
};

// ��־�ڲ�����
void GenericStart();
void GenericWrite(LPCSTR pfilename, unsigned int nLine, LPCSTR pfuncname,
	unsigned int nLevel, LPCTSTR pFmt, ...);
void GenericStop();

// �������º�������
#define	LogStart() GenericStart()	// ������LOG�߳�������������־�ļ�
#define LogWrite(nLevel, pFmt, ...) GenericWrite(__FILE__, __LINE__, __FUNCTION__, nLevel, pFmt, __VA_ARGS__)
#define	LogStop() GenericStop()
