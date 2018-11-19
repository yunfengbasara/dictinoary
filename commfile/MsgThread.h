#pragma once

// 消息线程
// how to use:
// YourClassName : public CMsgThread
// 开启、关闭调用Start、Stop
// Stop为阻塞函数,继承类应做好阻塞准备

class CMsgThread
{
public:
	// 线程状态
	enum MSGTHREADST {
		TS_CONSTRUCT,
		TS_INIT,
		TS_RUNNING,
		TS_EXITING,
		TS_UNINIT,
	};

	// 统一使用的回调函数
	typedef std::function<bool()>	ThreadFunc;

	// 线程消息格式
	struct CMDSTRUCT {
		enum CMDOPTION {
			SEND_COMMAND,	// 阻塞调用
			POST_COMMAND,	// 异步调用
		};

		CMDOPTION	m_nOption;
		ThreadFunc	m_nFunc;
	};

public:
	// 回调函数，供继承类使用
	virtual	bool Init() = 0;
	virtual bool UnInit() = 0;

public:
	// 操作函数，提供给外部的操作接口
	CMsgThread();
	virtual ~CMsgThread();
	MSGTHREADST	Status() const { return m_nST; }
	int GetCmdCount();

protected:
	void		Start();	// 不能在本类构造函数里调用Start的原因:因为线程会回调Init继承函数,而此时子类并没有构造完毕
	void		Stop();
	bool		PostCommand(ThreadFunc func);

private:
	void		_ThreadRoutine();
	void		_Running();

private:
	MSGTHREADST					m_nST;				// 线程状态
	std::mutex					m_nMutex;			// 消息锁
	std::condition_variable		m_nCondition;		// 阻塞等待
	std::thread					m_nThread;			// 消息线程
	std::list<CMDSTRUCT>		m_nCmdList;			// 消息队列
};

// 命令注册宏定义
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