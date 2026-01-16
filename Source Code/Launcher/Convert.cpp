// MIT License
//
// Copyright (c) 2025-2026 Convert - xmc0211 <xmc0211@qq.com>
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
#include <iomanip>
#include <assert.h>

// Base64 Chars
const std::string _BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// std::_tstring and std::wstring interconversion
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
size_t ustrlen(LPCUSTR str) {
	assert(str != NULL);
	size_t length = 0;
	while (*str != 0) {
		++length;
		++str;
	}
	return length;
}

// char* and unsigned char* interconversion
void CH2UCH(LPCSTR str, LPUSTR res, size_t sz, size_t res_sz) {
	assert(sz <= res_sz);
	assert(str != NULL && res != NULL);
	for (size_t i = 0; i < sz + 1; i++) {
		res[i] = static_cast<UCHAR>(str[i]);
	}
	return;
}
void UCH2CH(LPCUSTR str, LPSTR res, size_t sz, size_t res_sz) {
	assert(sz <= res_sz);
	assert(str != NULL && res != NULL);
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

// Character encoding conversion
BOOL ConvertCodePage(LPCSTR str, LPSTR res, size_t sz, size_t res_sz, UINT SourceCodePage, UINT TargetCodePage) {
	assert(str != NULL && res != NULL);

    // Step 1: SourceCodePage -> UTF-16
    int UTF16_Count = MultiByteToWideChar(
        SourceCodePage,
        0,
        str,
        (int)sz,
        nullptr,
        0
    );
    if (UTF16_Count <= 0) return FALSE;

    std::vector<WCHAR> UTF16Buffer(UTF16_Count + 1);
	if (!MultiByteToWideChar(
		SourceCodePage,
		0,
		str,
		(int)sz,
		UTF16Buffer.data(),
		UTF16_Count
	)) return FALSE;

    // Step 2: UTF-16 -> TargetCodePage
    int Target_Count = WideCharToMultiByte(
        TargetCodePage,
        0,
        UTF16Buffer.data(),
        UTF16_Count,
        nullptr,
        0,
        nullptr,
        nullptr
    );
	if (Target_Count <= 0) return FALSE;

    std::vector<CHAR> Result(Target_Count + 1);
	if (!WideCharToMultiByte(
		TargetCodePage,
		0,
		UTF16Buffer.data(),
		UTF16_Count,
		Result.data(),
		Target_Count,
		nullptr,
		nullptr
	)) return FALSE;

	strcpy_s(res, res_sz, Result.data());
	return TRUE;
}

// UCHAR* and hexadecimal _tstring conversion
void UCH2TSTR(LPCUSTR data, LPTSTR res, size_t sz, size_t res_sz) {
	assert(sz * 2 <= res_sz);
	assert(data != NULL && res != NULL);
	for (size_t i = 0; i < sz; i++) {
		_sntprintf_s(res + i * 2, res_sz - i * 2, _TRUNCATE, TEXT("%02x"), data[i]);
	}
	return;
}
void TSTR2UCH(LPCTSTR data, LPUSTR res, size_t sz, size_t res_sz) {
	assert(sz <= res_sz * 2);
	assert(data != NULL && res != NULL);
	for (size_t i = 0; i < sz * 2; i += 2) {
		TCHAR byteStr[] = {data[i], data[i + 1], TCHAR('\0')};
		res[i / 2] = (UCHAR)_tcstol(byteStr, NULL, 16);
	}
    return;
}

// Base64 encode and decode
std::_tstring Base64Encode(std::string String) {
	std::string Result;
	int Value = 0;
	int ValueBits = -6;
	for (unsigned char it : String) {
		Value = (Value << 8) + it;
		ValueBits += 8;
		while (ValueBits >= 0) {
			Result.push_back(_BASE64_CHARS[(Value >> ValueBits) & 0x3F]);
			ValueBits -= 6;
		}
	}
	if (ValueBits > -6) {
		Result.push_back(_BASE64_CHARS[((Value << 8) >> (ValueBits + 8)) & 0x3F]);
	}
	while (Result.size() % 4) {
		Result.push_back('=');
	}
	return LPC2LPT(Result);
}
std::string Base64Decode(std::_tstring String) {
	std::string Result;
	std::vector<int> Buffer(256, -1);
	for (int i = 0; i < 64; i++) Buffer[_BASE64_CHARS[i]] = i;
	int Value = 0, ValueBits = -8;
	for (unsigned char it : LPT2LPC(String)) {
		if (Buffer[it] == -1) break;
		Value = (Value << 6) + Buffer[it];
		ValueBits += 6;
		if (ValueBits >= 0) {
			Result.push_back(char((Value >> ValueBits) & 0xFF));
			ValueBits -= 8;
		}
	}
	return Result;
}