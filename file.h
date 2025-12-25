#define _CRT_SECURE_NO_WARNINGS
#pragma once
#include <filesystem>
#include <iostream>
#include <ShlObj.h>
#include <windows.h>
#include <Commdlg.h>
#include  <string>
#include<tchar.h>

//编码转换
#include <locale>
#include <codecvt>
inline std::string gb2312_to_utf8(std::string const& strGb2312)
{
	std::vector<wchar_t> buff(strGb2312.size());
#ifdef _MSC_VER
	std::locale loc("zh-CN");
#else
	std::locale loc("zh_CN.GB18030");
#endif
	wchar_t* pwszNext = nullptr;
	const char* pszNext = nullptr;
	mbstate_t state = {};
	int res = std::use_facet<std::codecvt<wchar_t, char, mbstate_t> >
		(loc).in(state,
			strGb2312.data(), strGb2312.data() + strGb2312.size(), pszNext,
			buff.data(), buff.data() + buff.size(), pwszNext);

	if (std::codecvt_base::ok == res)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> cutf8;
		return cutf8.to_bytes(std::wstring(buff.data(), pwszNext));
	}

	return "";

}

inline std::string utf8_to_gb2312(std::string const& strUtf8)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> cutf8;
	std::wstring wTemp = cutf8.from_bytes(strUtf8);
#ifdef _MSC_VER
	std::locale loc("zh-CN");
#else
	std::locale loc("zh_CN.GB18030");
#endif
	const wchar_t* pwszNext = nullptr;
	char* pszNext = nullptr;
	mbstate_t state = {};

	std::vector<char> buff(wTemp.size() * 2);
	int res = std::use_facet<std::codecvt<wchar_t, char, mbstate_t> >
		(loc).out(state,
			wTemp.data(), wTemp.data() + wTemp.size(), pwszNext,
			buff.data(), buff.data() + buff.size(), pszNext);

	if (std::codecvt_base::ok == res)
	{
		return std::string(buff.data(), pszNext);
	}
	return "";
}


inline const bool SelectOpenFiles(char path[])//获取的文件名是gb2312编码
{
	char temp[256];
	GetCurrentDirectoryA(256, temp);

	TCHAR szBuffer[MAX_PATH] = { 0 };
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFilter = _T(
		"STL File (*.stl)\0*.stl\0"
		"SLC File (*.slc)\0*.slc\0"
		"BMP File (*.bmp)\0*.bmp\0"
		"PNG File (*.png)\0*.png\0"
		"CTB File (*.ctb)\0*.ctb\0"
		"CT5 File (*.ct5)\0*.ct5\0"
		"All Files (*.*)\0*.*\0\0"
	); // 要选择的文件后缀
	ofn.lpstrInitialDir = L"C:\\Users\\zhang\\Desktop\\teng\\model";    // 默认的文件路径   
	ofn.lpstrFile = szBuffer;     // 存放文件的缓冲区  
	ofn.lpstrTitle = _T("请选择一个文件");
	ofn.nMaxFile = sizeof(szBuffer) / sizeof(*szBuffer);
	ofn.nFilterIndex = 0;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER; //标志如果是多选要加上OFN_ALLOWMULTISELECT  
	BOOL bSel = GetOpenFileName(&ofn);
	LPWSTR lpwszStrIn = szBuffer;

	SetCurrentDirectoryA(temp);

	if (lpwszStrIn != NULL)
	{
		int nInputStrLen = wcslen(lpwszStrIn);

		// Double NULL Termination  
		int nOutputStrLen = WideCharToMultiByte(CP_ACP, 0, lpwszStrIn, nInputStrLen, NULL, 0, 0, 0) + 2;
		if (path)
		{
			memset(path, 0x00, nOutputStrLen);
			WideCharToMultiByte(CP_ACP, 0, lpwszStrIn, nInputStrLen, path, nOutputStrLen, 0, 0);


			std::cout << path << std::endl;
			return true;
		}
	}
	return false;
}

inline std::string ConvertLPWSTRToString(LPWSTR lpwstr) {
	int bufferSize = WideCharToMultiByte(CP_UTF8, 0, lpwstr, -1, NULL, 0, NULL, NULL);
	std::string result(bufferSize, '\0');
	WideCharToMultiByte(CP_UTF8, 0, lpwstr, -1, &result[0], bufferSize, NULL, NULL);
	// Remove the null terminator added by WideCharToMultiByte
	result.resize(bufferSize - 1);
	return result;
}

inline const bool SelectOpenFiles(std::vector<std::string>& outFilePaths)//获取的文件名是gb2312编码
{
	if (outFilePaths.size() != 0)
		outFilePaths.clear();
	// 初始化 OPENFILENAME 结构体
	OPENFILENAME ofn;
	wchar_t fileNames[4096] = { 0 };  // 缓冲区，用于存储多个文件的路径
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = fileNames;
	ofn.nMaxFile = sizeof(fileNames);
	ofn.lpstrFilter = _T("STL File(*.stl)\0*.stl\0SLC File(*.slc)\0*.slc\0");
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

	// 显示文件打开对话框
	if (GetOpenFileName(&ofn) == TRUE) {
		// 解析返回的文件路径
		wchar_t* p = ofn.lpstrFile;
		std::wstring directory(p);  // 目录路径
		p += directory.length() + 1;

		if (*p == '\0') {
			// 仅选择了一个文件
			outFilePaths.push_back(utf8_to_gb2312(ConvertLPWSTRToString((LPWSTR)directory.c_str())));
		}
		else {
			// 选择了多个文件
			while (*p) {
				std::wstring filePath = directory + L"\\" + p;
				outFilePaths.push_back(utf8_to_gb2312(ConvertLPWSTRToString((LPWSTR)filePath.c_str())));
				p += wcslen(p) + 1;
			}
		}
		return true;
	}

	return false;
}


inline bool IsSupport(const std::string& str) {
	// 获取字符串长度
	size_t len = str.length();

	// 如果字符串长度小于6，返回整个字符串
	if (len <= 6) {
		return false;
	}

	//// 获取最后6个字符的子串
	if (str.substr(len - 6, 6) == "_s.slc")
		return true;
	else
		return false;
}

// 获取最后错误信息的字符串（辅助调试）
inline std::string getLastErrorString() {
	DWORD errorCode = GetLastError();
	LPSTR buffer = nullptr;

	// 格式化错误信息
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&buffer,
		0,
		nullptr
	);

	std::string errorMsg(buffer ? buffer : "Unknown error");
	LocalFree(buffer);  // 释放 FormatMessage 分配的内存
	return errorMsg;
}

// -------------------------- 核心功能函数 --------------------------
/**
 * @brief 获取当前用户登录名（返回 std::string，多字节编码）
 * @return 用户名（如 "Administrator"）
 * @throw std::runtime_error 获取失败时抛出异常（含错误信息）
 */
inline std::string getCurrentUserName() {
	// 1. 初始化缓冲区（UNLEN=256 是用户名最大长度）
	char buffer[256 + 1] = { 0 };
	DWORD bufferSize = sizeof(buffer) / sizeof(char);

	// 2. 调用 GetUserNameA（ANSI 版本）
	if (!GetUserNameA(buffer, &bufferSize)) {
		std::string errorMsg = "获取用户名失败：" + getLastErrorString();
		throw std::runtime_error(errorMsg);
	}

	// 3. 转换为 std::string 并返回
	return std::string(buffer);
}

/**
 * @brief 获取当前用户登录名（返回 std::wstring，宽字符编码，推荐）
 * @return 用户名（如 L"Administrator"）
 * @throw std::runtime_error 获取失败时抛出异常
 */
inline std::wstring getCurrentUserNameW() {
	wchar_t buffer[256 + 1] = { 0 };
	DWORD bufferSize = sizeof(buffer) / sizeof(wchar_t);

	if (!GetUserNameW(buffer, &bufferSize)) {
		std::string errorMsg = "获取用户名失败：" + getLastErrorString();
		throw std::runtime_error(errorMsg);
	}

	return std::wstring(buffer);
}


template<typename T>
inline int SaveVectorData(const std::vector<T>& data1, const std::vector<T>& data2, const std::string filename) {
	std::ofstream outFile(filename, std::ios::out);
	if (!outFile) {
		std::cerr << "无法打开文件！\n";
		return 1;
	}
	assert(data1.size() == data2.size());
	for (int i = 0; i < data1.size(); i++) {
		outFile << i << "\t" << data1[i] << "\t" << data2[i] << "\n"; // 每行一个
	}
	outFile.close();
	std::cout << filename << "\n";
	return 0;
}
inline bool SelectSaveFile(std::string& filename) {
	// 缓冲区保存选择的文件路径
	wchar_t filePath[MAX_PATH] = L"";

	// 设置 OPENFILENAME 结构体
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = _T("文本文件 (*.txt)\0*.txt\0所有文件 (*.*)\0*.*\0");
	ofn.lpstrFile = filePath;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = _T("保存文件");
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = _T("txt");

	if (GetSaveFileName(&ofn)) {
		filename = utf8_to_gb2312(ConvertLPWSTRToString((LPWSTR)filePath));
		std::cout << "用户选择的路径是:\n" << filename << std::endl;
		return true;
	}
	return false;
}