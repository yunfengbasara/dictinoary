#pragma once
#include "../commfile/MsgThread.h"

class CAnalyze : public CMsgThread
{
public:
	CAnalyze();
	~CAnalyze();

	virtual	bool Init() override;
	virtual bool UnInit() override;

	CreatePostCmd(CAnalyze, StartAnalyzeEcdict)
	CreatePostCmd(CAnalyze, QueryWord)
	CreatePostCmd(CAnalyze, SerializeEcdict)
	CreatePostCmd(CAnalyze, ReadFile)

private:
	bool StartAnalyzeEcdict();
	bool QueryWord(std::basic_string<char> word);
	bool SerializeEcdict();
	bool ReadFile();

private:
	bool AnalyzeECDICT(char *pBuffer, DWORD dwSize);
	bool AnalyzeLine(std::basic_string<wchar_t>& line,
		std::pair<std::basic_string<TCHAR>, std::basic_string<TCHAR>>& item);
	bool AnalyzeWord(IN std::basic_string<TCHAR>& line, IN int startpos,
		OUT std::basic_string<TCHAR>& word, OUT int& endpos);
	uint32_t BKDRHash(std::basic_string<char>& word);
	void GetStartAndLen(char t, uint32_t& s, uint32_t& l);	// 获取资源字母序的偏移地址和长度

private:
	std::mutex					 m_nMutex;
	std::map<uint32_t, uint32_t> m_nHashDICT;
	char*						 m_pDICTBuffer;				// 词典资源UFT8编码
	std::vector<std::pair<std::basic_string<wchar_t>, std::basic_string<wchar_t>>> m_trans;
};