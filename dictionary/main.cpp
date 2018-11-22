// dictionary.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "resource.h"
#include "Analyze.h"

#include "../commfile/NetTcp.h"
#include "../commfile/Timer.h"
#include "../commfile/LogApi.h"
#include "../commfile/MsgThread.h"
#pragma comment(lib, "../Debug/commfile.lib")

// 数据测试
class CDataTest
{
public:
	CDataTest(int vn):m(vn) { std::cout << "CDataTest begin" << std::endl; }
	~CDataTest() { std::cout << "CDataTest end" << std::endl; }
	int m;
};

class CThreadDemo : public CMsgThread , public CTimer
{
public:
	CThreadDemo() { Start(); }
	virtual ~CThreadDemo() { Stop(); }
	virtual	bool Init() override { RegisterTimer(CThreadDemo, ClearLog, 10000);  return true; }
	virtual bool UnInit() override { return true; }
public:
	// 注册消息
	CreatePostCmd(CThreadDemo, TestFun_0_Param)
	CreatePostCmd(CThreadDemo, TestFun_1_Param)
	CreatePostCmd(CThreadDemo, RefTest)
	CreatePostCmd(CThreadDemo, MultiParam)

private:
	void Log() { LogWrite(INFO, _T("%s %d"), _T("SOME LOGTEST"), 87867); }
	// 消息响应
	bool TestFun_0_Param() { Log(); return true; }
	bool TestFun_1_Param(int i) { std::cout << i << std::endl; Log(); return true;}
	bool RefTest(std::shared_ptr<CDataTest> pData) { std::cout << pData->m <<std::endl; return true; }
	bool MultiParam(int, char, std::string) { return true; }

	// 定时器
	void ClearLog() { 
		LogWrite(INFO, _T("start clear log...")); 
		//std::this_thread::sleep_for(std::chrono::milliseconds(4000));
		//LogWrite(INFO, _T("end clear log..."));
	}
};

int main()
{
	LogStart();

	CNetTcp netCore;
	netCore.Init();

	getchar();
	netCore.Uninit();
	//netCore.Uninit();

	//CAnalyze dict;
	//dict.PostCmd_StartAnalyzeEcdict();
	//dict.PostCmd_ReadFile();
	//while (true) {
	//	std::basic_string<char> word;
	//	std::getline(std::cin, word);
	//	dict.PostCmd_QueryWord(word);
	//}

	LogStop();
    return 0;
}

