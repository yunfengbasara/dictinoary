#include "stdafx.h"
#include "Timer.h"

CTimer::CTimer()
	: m_bExiting(false)
	, m_bRegister(false)
	, m_nTimeWaitMin(std::chrono::milliseconds(24 * 3600 * 1000))
{
	m_nThread = std::thread(&CTimer::_ThreadRoutine, this);
}

CTimer::~CTimer()
{
	{
		std::unique_lock<std::mutex> lock(m_nMutex);
		m_bExiting = true;
		m_nCondition.notify_all();
	}

	m_nThread.join();
}

void CTimer::Register(TimerFunc func, std::basic_string<TCHAR> funcname, uint32_t nCycle)
{
	TIMERSTRUCT task;
	task.m_nFunc = func;
	task.m_sFuncName = funcname;
	task.m_nCycleTickCnt = std::chrono::milliseconds(nCycle);
	task.m_nWaitTickCnt = std::chrono::milliseconds(0);

	{
		std::unique_lock<std::mutex> lock(m_nMutex);
		m_nTimerList.push_back(task);
		m_bRegister = true;
		m_nCondition.notify_all();
	}
}

void CTimer::Unregister(std::basic_string<TCHAR> funcname)
{
	{
		std::unique_lock<std::mutex> lock(m_nMutex);
		m_nTimerList.remove_if([&](const TIMERSTRUCT& t) {return funcname == t.m_sFuncName; });
		m_bRegister = true;
		m_nCondition.notify_all();
	}
}

void CTimer::_ThreadRoutine()
{
	while (true) {
		auto waitbegin = std::chrono::steady_clock::now();

		// 等待最近任务时间或者等待退出
		{	
			auto fCheckTaskAndExit = [&] {
				if (m_bExiting) {
					return true;
				}

				if (m_bRegister) {
					return true;
				}

				return false;
			};
			std::unique_lock<std::mutex> lock(m_nMutex);
			m_nCondition.wait_for(lock, m_nTimeWaitMin, fCheckTaskAndExit);
			// 注册函数通知复位
			m_bRegister = false;
		}

		// 计算等待的时间
		auto waitend = std::chrono::steady_clock::now();
		auto waittime = std::chrono::duration_cast<std::chrono::milliseconds>(waitend - waitbegin);

		// 线程退出
		if (true == m_bExiting) {
			return;
		}

		// 避免长时间任务阻塞而设置的临时任务列表
		std::list<TIMERSTRUCT>	tempList;
		{
			std::unique_lock<std::mutex> lock(m_nMutex);
			tempList = m_nTimerList;

			// 没有任务就重新等待
			if (tempList.empty()) {
				continue;
			}
		}

		// 执行定时器任务
		auto funbegin = std::chrono::steady_clock::now();
		for (auto &t : tempList) {
			t.m_nWaitTickCnt += waittime;
			if (t.m_nWaitTickCnt >= t.m_nCycleTickCnt) {
				t.m_nWaitTickCnt = std::chrono::milliseconds(0);
				// 可以开个线程去执行函数
				t.m_nFunc();
			}
		}
		// 计算执行所有定时器任务函数的时间
		auto funend = std::chrono::steady_clock::now();
		auto funtime = std::chrono::duration_cast<std::chrono::milliseconds>(funend - funbegin);

		// 更新任务列表时间
		// 在任务执行过程中原有列表可能增加或删除
		{
			std::unique_lock<std::mutex> lock(m_nMutex);
			for (auto &task : m_nTimerList) {
				for (auto &t : tempList) {
					// 更新时间
					if (t.m_sFuncName == task.m_sFuncName) {
						task.m_nWaitTickCnt = t.m_nWaitTickCnt;
					}
				}
				// 所有任务应加上函数执行的时间
				task.m_nWaitTickCnt += funtime;
			}
		}

		// 重新计算等待时间
		{
			std::unique_lock<std::mutex> lock(m_nMutex);
			m_nTimeWaitMin = std::chrono::milliseconds(24 * 3600 * 1000);
			for (auto &task : m_nTimerList) {
				auto needWaitTime = task.m_nCycleTickCnt - task.m_nWaitTickCnt;
				if (needWaitTime < m_nTimeWaitMin) {
					m_nTimeWaitMin = needWaitTime;
				}
			}
		}
	}
}