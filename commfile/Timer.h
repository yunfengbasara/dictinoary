#pragma once

// ��ʱ����û�������ʱ��sleep����ʱ��
// �������ʱ��sleep�������ʱ��
class CTimer
{
public:
	// ͳһʹ�õĶ�ʱ���ص�����
	typedef std::function<void()>	TimerFunc;

	// ��ʱ����Ϣ��ʽ
	struct TIMERSTRUCT {
		TimerFunc					m_nFunc;
		std::basic_string<TCHAR>	m_sFuncName;
		std::chrono::milliseconds	m_nWaitTickCnt;		// �Ѿ��ȴ�ʱ��
		std::chrono::milliseconds	m_nCycleTickCnt;	// ��������ʱ��
	};

public:
	CTimer();
	virtual ~CTimer();
	void Register(TimerFunc func, std::basic_string<TCHAR> funcname, uint32_t nCycle);
	void Unregister(std::basic_string<TCHAR> funcname);

private:
	void _ThreadRoutine();

private:
	std::mutex					m_nMutex;
	std::condition_variable		m_nCondition;
	std::thread					m_nThread;
	std::chrono::milliseconds	m_nTimeWaitMin;		// ��ǰ������С�ȴ�ʱ��
	std::list<TIMERSTRUCT>		m_nTimerList;		// ��ʱ����Ϣ����
	bool						m_bExiting;			// �Ƿ��˳�
	bool						m_bRegister;		// �Ƿ�������ע��
};

// ע�ᶨʱ��
#if defined UNICODE

#define RegisterTimer(classname, fun, cycle) \
	auto func = std::bind(&classname::fun, this); \
	TCHAR funcName[MAX_PATH] = { 0 }; \
	MultiByteToWideChar(CP_ACP, 0, #fun, strlen(#fun), funcName, sizeof(funcName) / sizeof(TCHAR)); \
	Register(func, funcName, cycle)

#define UnregisterTimer(fun) \
	TCHAR funcName[MAX_PATH] = { 0 }; \
	MultiByteToWideChar(CP_ACP, 0, #fun, strlen(#fun), funcName, sizeof(funcName) / sizeof(TCHAR)); \
	Unregister(funcName)

#else

#define RegisterTimer(classname, fun, cycle) \
	auto func = std::bind(&classname::fun, this); \
	Register(func, #fun, cycle)

#define UnregisterTimer(fun) \
	Unregister(#fun)

#endif