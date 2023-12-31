#pragma once
#include "framework.h"

void UTF16LE2GBK(const wchar_t utf[], int utfLength, std::string& gbk);
void SHIFTJIS2GBK(const char jis[], std::string& gbk);