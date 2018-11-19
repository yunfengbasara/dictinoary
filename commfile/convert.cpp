#include "stdafx.h"
#include "convert.h"

bool UTF82wchar(const std::basic_string<char> str_utf8, std::basic_string<wchar_t>& str_wchar)
{
	if (str_utf8.empty()) {
		return false;
	}

	int str_w_len = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str_utf8.c_str(), -1, NULL, 0);
	if (str_w_len == 0) {
		return false;
	}

	// Ä©Î²È¥µô0
	str_wchar.resize(str_w_len-1);
	LPWSTR pwchar = (LPWSTR)str_wchar.c_str();
	if (::MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), -1, pwchar, str_w_len) == 0) {
		return false;
	}

	return true;
}

bool wchar2UTF8(const std::basic_string<wchar_t> str_wchar, std::basic_string<char>& str_utf8)
{
	if (str_wchar.empty()) {
		return false;
	}

	int str_utf8_len = ::WideCharToMultiByte(CP_UTF8, 0, str_wchar.c_str(), -1, NULL,0, NULL, NULL);
	if (str_utf8_len == 0) {
		return false;
	}

	str_utf8.resize(str_utf8_len-1);
	LPSTR pchar = (LPSTR)str_utf8.c_str();
	if (::WideCharToMultiByte(CP_UTF8, 0, str_wchar.c_str(), -1, pchar, str_utf8_len, NULL, NULL) == 0) {
		return false;
	}

	return true;
}

bool wchar2char(const std::basic_string<wchar_t> str_wchar, std::basic_string<char>& str_char)
{
	if (str_wchar.empty()) {
		return false;
	}

	int str_char_len = ::WideCharToMultiByte(CP_ACP, 0, str_wchar.c_str(), -1, NULL, 0, NULL, NULL);
	if (str_char_len == 0) {
		return false;
	}

	str_char.resize(str_char_len-1);
	LPSTR pchar = (LPSTR)str_char.c_str();
	if (::WideCharToMultiByte(CP_ACP, 0, str_wchar.c_str(), -1, pchar, str_char_len, NULL, NULL) == 0) {
		return false;
	}

	return true;
}

bool char2wchar(const std::basic_string<char> str_char, std::basic_string<wchar_t>& str_wchar)
{
	if (str_char.empty()) {
		return false;
	}

	int str_w_len = ::MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, str_char.c_str(), -1, NULL, 0);
	if (str_w_len == 0) {
		return false;
	}

	str_wchar.resize(str_w_len-1);
	LPWSTR pwchar = (LPWSTR)str_wchar.c_str();
	if (::MultiByteToWideChar(CP_ACP, 0, str_char.c_str(), -1, pwchar, str_w_len) == 0) {
		return false;
	}

	return true;
}