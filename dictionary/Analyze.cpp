#include "stdafx.h"
#include "Analyze.h"
#include "resource.h"
#include "../commfile/convert.h"
#include "../commfile/LogApi.h"

CAnalyze::CAnalyze()
{
	Start();
}

CAnalyze::~CAnalyze()
{
	Stop();
}

bool CAnalyze::Init()
{
	return true;
}

bool CAnalyze::UnInit()
{
	return true;
}

bool CAnalyze::StartAnalyzeEcdict()
{
	LogWrite(INFO, _T("start analyze ecdict"));
	// 初始化词典资源
	{
		HRSRC hRsrc = ::FindResource(NULL, MAKEINTRESOURCE(IDR_ECDICT), _T("DICT"));
		DWORD dwSize = ::SizeofResource(NULL, hRsrc);
		HGLOBAL hGlobal = ::LoadResource(NULL, hRsrc);
		m_pDICTBuffer = (char*)::LockResource(hGlobal); // utf8 编码的资源
		::FreeResource(hGlobal);	// 此处可以释放掉资源，只保留m_pDICTBuffer

		// 平均耗时60秒
		// 从词典资源直接初始化hash表
		//if (!AnalyzeECDICT(m_pDICTBuffer, dwSize)) {
		//	LogWrite(EROR, _T("analyze ecdict error"));
		//}
	}

	// 初始化词典hash
	{
		HRSRC hRsrc = ::FindResource(NULL, MAKEINTRESOURCE(IDR_DICTINDEX), _T("DICT"));
		DWORD dwSize = ::SizeofResource(NULL, hRsrc);
		HGLOBAL hGlobal = ::LoadResource(NULL, hRsrc);
		char* pHash = (char*)::LockResource(hGlobal);
		::FreeResource(hGlobal);

		// 平均耗时18秒
		std::istrstream dict(pHash, dwSize);
		uint32_t hash = 0, pos = 0;
		while (!dict.eof()) {
			dict.read(reinterpret_cast<char*>(&hash), 4);
			dict.read(reinterpret_cast<char*>(&pos), 4);
			m_nHashDICT.insert(std::make_pair(hash, pos));
		}
	}

	LogWrite(INFO, _T("end analyze ecdict"));
	return true;
}

bool CAnalyze::QueryWord(std::basic_string<char> word)
{
	//LogWrite(INFO, _T("start query word"));

	uint32_t hash = BKDRHash(word);
	auto it = m_nHashDICT.find(hash);
	if (it == m_nHashDICT.end()) {
		word += "不能知道";
		std::basic_string<wchar_t> msg;
		char2wchar(word, msg);
		LogWrite(EROR, _T("can't find word : %s"), msg.c_str());
		return true;
	}

	// 读出一行
	std::basic_string<char> line;
	for (int s = 0; ;s++) {
		char r = *(m_pDICTBuffer + it->second + s);
		char n = *(m_pDICTBuffer + it->second + s + 1);
		if (r == '\r' && n == '\n') {
			line += r;
			line += n;
			break;
		}
		line += r;
	}

	std::basic_string<wchar_t> unicodeline;
	UTF82wchar(line, unicodeline);

	std::pair<std::basic_string<TCHAR>, std::basic_string<TCHAR>> item;
	if (!AnalyzeLine(unicodeline, item)) {
		LogWrite(EROR, _T("AnalyzeLine wrong: %s"), unicodeline.c_str());
		return true;
	}

	LogWrite(INFO, _T("%s"), item.second.c_str());
	//LogWrite(INFO, _T("end query word"));
	m_trans.push_back(std::make_pair(item.first, item.second));
	return true;
}

bool CAnalyze::SerializeEcdict()
{
	std::basic_string<char> msg;
	std::ofstream wordlist;
	wordlist.open("D:\\code\\cproj\\dictinoary\\dictionary\\Debug\\bookdraft2017nov5_trans.txt", std::ios::out);
	for (auto &item : m_trans) {
		wchar2UTF8(item.first, msg);
		wordlist << msg;
		wordlist << ":";
		wchar2UTF8(item.second, msg);
		wordlist << msg;
		wordlist << std::endl;
	}

	wordlist.close();
	return true;
}

bool CAnalyze::ReadFile()
{
	// 构建不重复的单词表
	std::set<std::basic_string<char>> wordlist;

	std::ifstream bookfile;
	bookfile.open("D:\\code\\cproj\\dictinoary\\dictionary\\Debug\\bookdraft2017nov5.txt");

	std::basic_string<char> word;
	char letter;
	while (!bookfile.eof()) {
		bookfile.read(&letter, 1);
		if (letter == '.' || letter == ' ') {
			// 超过三个字母的单词才需要查找
			if (word.size() >= 4) {
				// 如果所有字母大写，原样查找
				// 如果只有第一个字母大写，将大写转小写
				if ((word[0] >= 'A' && word[0] <= 'Z') &&
					(word[1] >= 'a' && word[1] <= 'z')) {
					word[0] += 32;
				}
				// 查找单词
				if (wordlist.find(word) == wordlist.end()) {
					wordlist.insert(word);
					PostCmd_QueryWord(word);
				}
			}
			word = "";
			continue;
		}

		if (letter >= 'a' && letter <= 'z') {
			word += letter;
			continue;
		}

		if (letter >= 'A' && letter <= 'Z') {
			word += letter;
			continue;
		}

		word = "";
	}

	bookfile.close();

	PostCmd_SerializeEcdict();
	return true;
}

bool CAnalyze::AnalyzeECDICT(char *pBuffer, DWORD dwSize)
{
	std::vector<std::thread> threadAnalyzeByLetter;
	for (char c = 'a'; c <= 'z'; c++) {
		threadAnalyzeByLetter.push_back(std::thread([=] {
			// 获取每个词段开始和长度，根据字母序
			uint32_t s, l, wordPos = 0;
			GetStartAndLen(c, s, l);
			std::istrstream input(pBuffer + s, l);
			// 逐行扫描
			std::basic_string<char> line;
			while (std::getline(input, line)) {
				// 分析单词
				size_t endpos = line.find(',');
				if (endpos == -1) { continue; }
				auto word = line.substr(0, endpos);
				// 获取hash编码和位置
				uint32_t hash = BKDRHash(word);
				{
					std::unique_lock<std::mutex> lock(m_nMutex);
					m_nHashDICT.insert(std::make_pair(hash, wordPos + s));
				}	
				wordPos += line.length() + 1;
			}
		}));
	}

	for (auto& thread : threadAnalyzeByLetter) {
		thread.join();
	}

	return true;
}

bool CAnalyze::AnalyzeLine(std::basic_string<wchar_t>& line,
	std::pair<std::basic_string<TCHAR>, std::basic_string<TCHAR>>& item)
{
	// 过滤不解析单词
	if (line[0] == _T('\'') ||
		line[0] == _T('-') ||
		line[0] == _T('.') ||
		(line[0] <= _T('9') && line[0] >= _T('0'))) {
		return false;
	}

	int startpos = 0, endpos = 0;
	std::basic_string<TCHAR> word;
	if (!AnalyzeWord(line, startpos, word, endpos)) {
		return false;
	}

	startpos = endpos;
	std::basic_string<TCHAR> phonetic;
	if (!AnalyzeWord(line, startpos, phonetic, endpos)) {
		return false;
	}

	startpos = endpos;
	std::basic_string<TCHAR> definition;
	if (!AnalyzeWord(line, startpos, definition, endpos)) {
		return false;
	}

	startpos = endpos;
	std::basic_string<TCHAR> translation;
	if (!AnalyzeWord(line, startpos, translation, endpos)) {
		return false;
	}

	item = std::make_pair(word, translation);
	return true;
}

bool CAnalyze::AnalyzeWord(IN std::basic_string<TCHAR>& line, IN int startpos,
	OUT std::basic_string<TCHAR>& word, OUT int& endpos)
{
	// 检测长度
	if (line.length() < size_t(startpos + 1)) {
		return false;
	}

	// 解析双引号流程
	if (line[startpos + 1] == _T('\"')) {
		// 找下一个双引号
		endpos = line.find(_T('\"'), startpos + 2);
		if (endpos == -1) { return false; }

		// 去掉前后双引号
		word = line.substr(startpos + 2, endpos - startpos - 2);

		// 把末尾引号也包括进来
		endpos++;
	}
	// 解析逗号流程
	else {
		// 找下一个逗号
		endpos = line.find(_T(','), startpos + 1);
		if (endpos == -1) { return false; }

		// 对第一个词取全部
		if (startpos != 0) {
			word = line.substr(startpos + 1, endpos - startpos - 1);
		}
		else {
			word = line.substr(startpos, endpos - startpos);
		}
	}

	return true;
}

uint32_t CAnalyze::BKDRHash(std::basic_string<char>& word)
{
	uint32_t seed = 131; // 31 131 1313 13131 131313 etc..
	uint32_t hash = 0;

	for (char& c : word) {
		hash = hash * seed + c;
	}

	return (hash & 0x7FFFFFFF);
}

void CAnalyze::GetStartAndLen(char t, uint32_t& s, uint32_t& l)
{
	switch(t) {
	case 'a': s = 57690;	l = 4822984; break;
	case 'b': s = 4880674;  l = 3819168; break;
	case 'c': s = 8699842;  l = 6302676; break;
	case 'd': s = 15002518; l = 3467232; break;
	case 'e': s = 18469750; l = 2571978; break;
	case 'f': s = 21041728; l = 2679695; break;
	case 'g': s = 23721423; l = 2320216; break;
	case 'h': s = 26041639; l = 2663295; break;
	case 'i': s = 28704934; l = 2434378; break;
	case 'j': s = 31139312; l = 556038;  break;
	case 'k': s = 31695350; l = 759970;  break;
	case 'l': s = 32455320; l = 2380662; break;
	case 'm': s = 34835982; l = 3961019; break;
	case 'n': s = 38797001; l = 1701602; break;
	case 'o': s = 40498603; l = 1793186; break;
	case 'p': s = 42291789; l = 5565506; break;
	case 'q': s = 47857295; l = 333694;  break;
	case 'r': s = 48190989; l = 3157461; break;
	case 's': s = 51348450; l = 6866808; break;
	case 't': s = 58215258; l = 3483149; break;
	case 'u': s = 61698407; l = 1202729; break;
	case 'v': s = 62901136; l = 1029015; break;
	case 'w': s = 63930151; l = 1420936; break;
	case 'x': s = 65351087; l = 127114;  break;
	case 'y': s = 65478201; l = 212033;  break;
	case 'z': s = 65690234; l = 246465;  break;
	}
}