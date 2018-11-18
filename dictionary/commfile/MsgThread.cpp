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
	// Ϊʲô��Ҫһ��ר�ŵ�start������
	// ��Ϊ������ڹ��캯���У��߳̿�����ʱ���޷���������Init�̳к���
	// ��Ϊ����Init������δ������
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
	// ѭ�������������
	while (true) {
		{	
			// �����ж���������Ƿ������� �� �Ƿ��˳�
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
			// ����ȴ���ʱ,�����ȴ�����
			auto bWaitOutTime = m_nCondition.wait_for(lock, nTimeOut, fCheckTaskAndExit);
			if(!bWaitOutTime){
				continue;
			}
		}

		// �Ƿ��˳�
		if (TS_EXITING == m_nST) {
			return;
		}

		CMDSTRUCT cmd;
		{
			std::unique_lock<std::mutex> lock(m_nMutex);
			cmd = m_nCmdList.front();
			m_nCmdList.pop_front();
		}

		// ִ�к���
		bool bRet = cmd.m_nFunc();

		// ����ִ��ʧ���˳��߳�
		if (!bRet) {
			return;
		}
	}
}