#pragma once

// ��Ϣ����ģ��

// how to define:
// YourClassName : public CMsgThread
// CreatePostCmd_0(fun)

// how to use:
// YourClassName.PostCmd_fun()

// ����_0 �� _1����Ϊ�˿��������׵�,���Ƴ�_N�����Դ
// ʵ�ʹ�������_N�꼴��

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