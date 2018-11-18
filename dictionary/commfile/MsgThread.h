#pragma once

// ��Ϣ�߳�
// how to use:
// YourClassName : public CMsgThread
// �������رյ���Start��Stop
// StopΪ��������,�̳���Ӧ��������׼��

class CMsgThread
{
public:
	// �߳�״̬
	enum MSGTHREADST {
		TS_CONSTRUCT,
		TS_INIT,
		TS_RUNNING,
		TS_EXITING,
		TS_UNINIT,
	};

	// ͳһʹ�õĻص�����
	typedef std::function<bool()>	ThreadFunc;

	// �߳���Ϣ��ʽ
	struct CMDSTRUCT {
		enum CMDOPTION {
			SEND_COMMAND,	// ��������
			POST_COMMAND,	// �첽����
		};

		CMDOPTION	m_nOption;
		ThreadFunc	m_nFunc;
	};

public:
	// �ص����������̳���ʹ��
	virtual	bool Init() = 0;
	virtual bool UnInit() = 0;

public:
	// �����������ṩ���ⲿ�Ĳ����ӿ�
	CMsgThread();
	virtual ~CMsgThread();
	MSGTHREADST	Status() const { return m_nST; }
	int GetCmdCount();

protected:
	void		Start();	// �����ڱ��๹�캯�������Start��ԭ��:��Ϊ�̻߳�ص�Init�̳к���,����ʱ���ಢû�й������
	void		Stop();
	bool		PostCommand(ThreadFunc func);

private:
	void		_ThreadRoutine();
	void		_Running();

private:
	MSGTHREADST					m_nST;				// �߳�״̬
	std::mutex					m_nMutex;			// ��Ϣ��
	std::condition_variable		m_nCondition;		// �����ȴ�
	std::thread					m_nThread;			// ��Ϣ�߳�
	std::list<CMDSTRUCT>		m_nCmdList;			// ��Ϣ����
};

// ����ע��궨��
// how to define:
// YourClassName : public CMsgThread
// CreatePostCmd_0(fun)

// how to use:
// YourClassName.PostCmd_fun()
#define CreatePostCmd(classname, fun) \
template<typename... Args> \
bool PostCmd_##fun(Args &&... t) \
{ \
	auto threadFun = std::bind(&classname::fun, this, std::forward<Args>(t)...); \
	return PostCommand(threadFun); \
}