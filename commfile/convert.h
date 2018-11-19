#pragma once

bool UTF82wchar(const std::basic_string<char> str_utf8, std::basic_string<wchar_t>& str_wchar);
bool wchar2UTF8(const std::basic_string<wchar_t> str_wchar, std::basic_string<char>& str_utf8);
bool wchar2char(const std::basic_string<wchar_t> str_wchar, std::basic_string<char>& str_char);
bool char2wchar(const std::basic_string<char> str_char, std::basic_string<wchar_t>& str_wchar);