#include "stdafx.h"
#include "MsgThread.h"

CMsgThread::CMsgThread()
	: m_nST(TS_CONSTRUCT)
{
	
}

CMsgThread::~CMsgThread()
{
	
}

int CMsgThread::GetCmdCount()
{
	std::unique_lock<std::mutex> lock(m_nMutex);
	return m_nCmdList.size();
}

void CMsgThread::Start()
{
	// 为什么需要一个专门的start函数？
	// 因为如果放在构造函数中，线程开启的时候，无法调用子类Init继承函数
	// 因为子类Init函数还未构建好
	m_nThread = std::thread(&CMsgThread::_ThreadRoutine, this);
}

void CMsgThread::Stop()
{
	{
		std::unique_lock<std::mutex> lock(m_nMutex);
		m_nST = TS_EXITING;
		m_nCondition.notify_all();
	}

	m_nThread.join();
}

bool CMsgThread::PostCommand(ThreadFunc func)
{
	{
		std::unique_lock<std::mutex> lock(m_nMutex);
		if (m_nST == TS_EXITING || m_nST == TS_UNINIT) {
			return false;
		}

		CMDSTRUCT cmd;
		cmd.m_nOption = CMDSTRUCT::POST_COMMAND;
		cmd.m_nFunc = func;

		m_nCmdList.push_back(cmd);
		m_nCondition.notify_all();
	}

	return true;
}

void CMsgThread::_ThreadRoutine()
{
	// Init
	m_nST = TS_INIT;
	if (!Init()) {
		return;
	}

	// Running
	m_nST = TS_RUNNING;
	_Running();
	
	// UnInit
	m_nST = TS_UNINIT;
	if (!UnInit()) {
		return;
	}
}

void CMsgThread::_Running()
{
	// 循环处理命令队列
	while (true) {
		{	
			// 用于判断命令队列是否有内容 或 是否退出
			auto fCheckTaskAndExit = [&] {
				if (TS_EXITING == m_nST) {
					return true;
				}

				if (!m_nCmdList.empty()) {
					return true;
				}

				return false;
			};
			std::unique_lock<std::mutex> lock(m_nMutex);
			auto nTimeOut = std::chrono::hours(24);
			// 如果等待超时,继续等待即可
			auto bWaitOutTime = m_nCondition.wait_for(lock, nTimeOut, fCheckTaskAndExit);
			if(!bWaitOutTime){
				continue;
			}
		}

		// 是否退出
		if (TS_EXITING == m_nST) {
			return;
		}

		CMDSTRUCT cmd;
		{
			std::unique_lock<std::mutex> lock(m_nMutex);
			cmd = m_nCmdList.front();
			m_nCmdList.pop_front();
		}

		// 执行函数
		bool bRet = cmd.m_nFunc();

		// 函数执行失败退出线程
		if (!bRet) {
			return;
		}
	}
}