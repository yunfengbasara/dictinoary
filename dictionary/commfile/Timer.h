#pragma once

// 定时器，没有任务的时候sleep无限时间
// 有任务的时候sleep到最近的时间
class CTimer
{
public:
	// 统一使用的定时器回调函数
	typedef std::function<void()>	TimerFunc;

	// 定时器消息格式
	struct TIMERSTRUCT {
		TimerFunc					m_nFunc;
		std::basic_string<TCHAR>	m_sFuncName;
		std::chrono::milliseconds	m_nWaitTickCnt;		// 已经等待时间
		std::chrono::milliseconds	m_nCycleTickCnt;	// 命令周期时间
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
	std::chrono::milliseconds	m_nTimeWaitMin;		// 当前任务最小等待时间
	std::list<TIMERSTRUCT>		m_nTimerList;		// 定时器消息队列
	bool						m_bExiting;			// 是否退出
	bool						m_bRegister;		// 是否新任务注册
};

// 注册定时器
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