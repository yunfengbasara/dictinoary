#pragma once

// 消息定义模板

// how to define:
// YourClassName : public CMsgThread
// CreatePostCmd_0(fun)

// how to use:
// YourClassName.PostCmd_fun()

// 保留_0 和 _1宏是为了看代码容易点,能推出_N宏的来源
// 实际过程中用_N宏即可

#define CreatePostCmd_0(classname, fun)	\
bool PostCmd_##fun() \
{ \
	auto threadFun = std::bind(&classname::fun, this); \
	return PostCommand(threadFun); \
}

#define CreatePostCmd_1(classname, fun) \
template<typename T1> \
bool PostCmd_##fun(const T1& t1) \
{ \
	auto threadFun = std::bind(&classname::fun, this, t1); \
	return PostCommand(threadFun); \
}

#define CreatePostCmd_N(classname, fun) \
template<typename... Args> \
bool PostCmd_##fun(Args &&... t) \
{ \
	auto threadFun = std::bind(&classname::fun, this, std::forward<Args>(t)...); \
	return PostCommand(threadFun); \
}