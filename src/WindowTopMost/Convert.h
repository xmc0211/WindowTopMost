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

#ifndef CONVERT_H
#define CONVERT_H

#include <windows.h>
#include <string>
#include <vector>
#include <iomanip>
#include <tchar.h>

#if defined(UNICODE)
#define _tstring wstring
#else
#define _tstring string
#endif

// std::string and std::wstring interconversion
std::wstring LPC2LPW(const std::string str);
std::string LPW2LPC(const std::wstring wstr);

#ifdef UNICODE
#define LPT2LPW(str) (str)
#define LPT2LPC(str) LPW2LPC(str)
#define LPW2LPT(str) (str)
#define LPC2LPT(str) LPC2LPW(str)
#else
#define LPT2LPW(str) LPC2LPW(str)
#define LPT2LPC(str) (str)
#define LPW2LPT(str) LPW2LPC(str)
#define LPC2LPT(str) (str)
#endif

// strlen for UCHAR*
size_t ustrlen(const UCHAR* str);

// char* and unsigned char* interconversion
void CH2UCH(const char* str, UCHAR* res, size_t sz);
void UCH2CH(const UCHAR* str, char* res, size_t sz);

// ULONG and std::_tstring interconversion
std::_tstring UL2TSTR(const ULONG res);
ULONG TSTR2UL(const std::_tstring str);

// UCHAR* and hexadecimal string conversion
void UCH2TSTR(const UCHAR* data, TCHAR* res, size_t sz, size_t res_sz);
void TSTR2UCH(const TCHAR* hexStr, UCHAR* data, size_t sz);

#endif
