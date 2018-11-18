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

		// �ȴ��������ʱ����ߵȴ��˳�
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
			// ע�ắ��֪ͨ��λ
			m_bRegister = false;
		}

		// ����ȴ���ʱ��
		auto waitend = std::chrono::steady_clock::now();
		auto waittime = std::chrono::duration_cast<std::chrono::milliseconds>(waitend - waitbegin);

		// �߳��˳�
		if (true == m_bExiting) {
			return;
		}

		// ���ⳤʱ���������������õ���ʱ�����б�
		std::list<TIMERSTRUCT>	tempList;
		{
			std::unique_lock<std::mutex> lock(m_nMutex);
			tempList = m_nTimerList;

			// û����������µȴ�
			if (tempList.empty()) {
				continue;
			}
		}

		// ִ�ж�ʱ������
		auto funbegin = std::chrono::steady_clock::now();
		for (auto &t : tempList) {
			t.m_nWaitTickCnt += waittime;
			if (t.m_nWaitTickCnt >= t.m_nCycleTickCnt) {
				t.m_nWaitTickCnt = std::chrono::milliseconds(0);
				// ���Կ����߳�ȥִ�к���
				t.m_nFunc();
			}
		}
		// ����ִ�����ж�ʱ����������ʱ��
		auto funend = std::chrono::steady_clock::now();
		auto funtime = std::chrono::duration_cast<std::chrono::milliseconds>(funend - funbegin);

		// ���������б�ʱ��
		// ������ִ�й�����ԭ���б�������ӻ�ɾ��
		{
			std::unique_lock<std::mutex> lock(m_nMutex);
			for (auto &task : m_nTimerList) {
				for (auto &t : tempList) {
					// ����ʱ��
					if (t.m_sFuncName == task.m_sFuncName) {
						task.m_nWaitTickCnt = t.m_nWaitTickCnt;
					}
				}
				// ��������Ӧ���Ϻ���ִ�е�ʱ��
				task.m_nWaitTickCnt += funtime;
			}
		}

		// ���¼���ȴ�ʱ��
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