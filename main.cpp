#include<modbus.h>
#include<iostream>
#include<thread>
#include <DbgHelp.h>
#include <ctime>

#include"control.h"
#include"mainUI.h"

#pragma comment(lib, "Dbghelp.lib")

using namespace std;
LONG WINAPI MyUnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo);
int main()
{
	// 注册未处理异常过滤器
	SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
	//(*write()).MW12_12_14.Z轴运动结束信号 = 1;
	//(*write()).MW12_12_14.F轴运动结束信号 = 1;
	//(*write()).MW12_12_14.C轴运动结束信号 = 1;
	//bool moveStopFlag = (*(uint16_t*)(&get().MW12_12_14) & (1 << (12 + 1 ))) >> ( 12 + 1 );
	//std::cout << moveStopFlag << std::endl;
	ModbusInitial();
	//加入读取寄存器的线程
	thread t5(Modbus);
	thread t2(ReadRegister);
	//加入UI刷新线程
	thread t1(showUI);
	//thread t3(ShowUICOMMand);
	//加入运动控制台线程
	//thread t4(MoveCom);
	//this_thread::sleep_for(chrono::milliseconds(3000));
	t5.join();
	t2.join();
	t1.join();
	//t3.join();
	//t4.join();

	return 0;
	
}



LONG WINAPI MyUnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo)
{
    // 生成 dump 文件名
    SYSTEMTIME st;
    GetLocalTime(&st);

    char filename[MAX_PATH];
    sprintf_s(filename, "Crash_%04d%02d%02d_%02d%02d%02d.dmp",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    // 创建 dump 文件
    HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ExceptionPointers = pExceptionInfo;
        dumpInfo.ClientPointers = FALSE;

        // 写入 dump
        MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            MiniDumpWithFullMemory, // 或 MiniDumpNormal
            &dumpInfo,
            NULL,
            NULL
        );

        CloseHandle(hFile);
        std::cout << "Dump file created: " << filename << std::endl;
    }
    else {
        std::cerr << "Failed to create dump file." << std::endl;
    }

    return EXCEPTION_EXECUTE_HANDLER; // 继续异常流程
}