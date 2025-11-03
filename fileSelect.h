#pragma once
#include <iostream>
#include <ShlObj.h>
#include <windows.h>
#include <Commdlg.h>
#include  <string>



char* wcharTochar(const wchar_t* _wchar)
{
	char* _char;
	int len = WideCharToMultiByte(CP_ACP, 0, _wchar, (int)wcslen(_wchar), NULL, 0, NULL, NULL);
	_char = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, _wchar, (int)wcslen(_wchar), _char, len, NULL, NULL);
	_char[len] = '\0';
	return _char;
}

void openFileDialog()
{
	OPENFILENAME ofn;			// 公共对话框结构
	TCHAR szFile[MAX_PATH];		// 保存获取文件名称的缓冲区   
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	//ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0Image\0*.PNG;*.JPG\0"; //过滤规则
	ofn.lpstrFilter = L"STL\0*.stl\0SLC\0*.slc\0"; //过滤规则
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = L"C:\\Users\\zhang\\Desktop\\teng\\model";	//指定默认路径
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&ofn))
	{

		//显示选择的文件。 
		wchar_t* t = ofn.lpstrFile;
		// 将wchar_t转化为char输出
		std::cout << wcharTochar(t) << std::endl;
	}
}

