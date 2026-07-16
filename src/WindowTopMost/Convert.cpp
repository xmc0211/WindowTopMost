// MIT License
//
// Copyright (c) 2025 Convert - xmc0211 <xmc0211@qq.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "Convert.h"

// std::string and std::wstring interconversion
std::wstring LPC2LPW(const std::string str) {
	INT size = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
	if (size == 0) return L"";

	std::vector<WCHAR> buffer(size + 1);
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer.data(), size);
	buffer[size] = '\0';

	return std::wstring(buffer.data(), size - 1);
}
std::string LPW2LPC(const std::wstring wstr) {
	INT size = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (size == 0) return "";

	std::vector<CHAR> buffer(size + 1);
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, buffer.data(), size, nullptr, nullptr);
	buffer[size] = '\0';

	return std::string(buffer.data(), size - 1);
}

// strlen for UCHAR*
size_t ustrlen(const UCHAR* str) {
	size_t length = 0;
	while (*str != 0) {
		++length;
		++str;
	}
	return length;
}

// char* and unsigned char* interconversion
void CH2UCH(const char* str, UCHAR* res, size_t sz) {
	for (size_t i = 0; i < sz + 1; i++) {
		res[i] = static_cast<UCHAR>(str[i]);
	}
	return;
}
void UCH2CH(const UCHAR* str, char* res, size_t sz) {
	for (size_t i = 0; i < sz + 1; i++) {
		res[i] = static_cast<CHAR>(str[i]);
	}
	return;
}

// ULONG and std::_tstring interconversion
std::_tstring UL2TSTR(const ULONG res) {
	if (res == 0) return TEXT("0");
	std::_tstring ans = TEXT("");
	ULONG tmp = res;
	while (tmp) {
		ans = TCHAR((tmp % 10) + TCHAR('0')) + ans;
		tmp /= 10;
	}
	return ans;
}
ULONG TSTR2UL(const std::_tstring str) {
	size_t sz = str.size();
	ULONG ans = 0;
	for (UINT indx = 0; indx < sz; indx++) {
		ans = ans * 10 + (str[indx] - TCHAR('0'));
	}
	return ans;
}

// UCHAR* and hexadecimal string conversion
void UCH2TSTR(const UCHAR* data, TCHAR* res, size_t sz, size_t res_sz) {
	for (size_t i = 0; i < sz; i++) {
		_sntprintf_s(res + i * 2, res_sz - i * 2, _TRUNCATE, TEXT("%02x"), data[i]);
	}
	return;
}
void TSTR2UCH(const TCHAR* hexStr, UCHAR* data, size_t sz) {
	for (size_t i = 0; i < sz * 2; i += 2) {
		TCHAR byteStr[] = {hexStr[i], hexStr[i + 1], TCHAR('\0')};
		data[i / 2] = (UCHAR)_tcstol(byteStr, NULL, 16);
	}
    return;
}
