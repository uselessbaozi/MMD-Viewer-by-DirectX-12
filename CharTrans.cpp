#include "pch.h"
#include "CharTrans.h"

void UTF16LE2GBK(const wchar_t utf[], int utfLength, std::string& gbk)
{
	int gbkLength = WideCharToMultiByte(CP_ACP, 0, utf, utfLength, NULL, 0, NULL, NULL);
	char* s(new char[gbkLength]);
	WideCharToMultiByte(CP_ACP, 0, utf, utfLength, s, gbkLength, NULL, NULL);
	gbk = s;
	delete[] s;
	auto index(gbk.find(-3));
	gbk = gbk.substr(0, index);
}

void SHIFTJIS2GBK(const char jis[], std::string& gbk)
{
	int ansi_length = MultiByteToWideChar(932, 0, jis, -1, NULL, 0);
	wchar_t ansi_str[256];
	MultiByteToWideChar(932, 0, jis, -1, ansi_str, ansi_length);
	char gbk_char_str[256];
	WideCharToMultiByte(936, 0, ansi_str, -1, gbk_char_str, sizeof(gbk_char_str), NULL, NULL);
	gbk = gbk_char_str;
}
