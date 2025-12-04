#include "Socket.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "192.168.1.17" // 服务端IP（本地测试用回环地址）
#define PORT 8888             // 服务端端口
#define BUF_SIZE 1024

namespace ThermalConnector {
    WSADATA wsaData;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    char sendBuf[BUF_SIZE], recvBuf[BUF_SIZE];
    int ret;
    int init() {


        // 1. 初始化Winsock
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            printf("WSAStartup失败！错误码：%d\n", WSAGetLastError());
            return 1;
        }

        // 2. 创建客户端套接字
        clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (clientSocket == INVALID_SOCKET) {
            printf("socket创建失败！错误码：%d\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // 3. 设置服务端地址
        serverAddr.sin_family = AF_INET;
        inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr); // IP转换为网络字节序
        serverAddr.sin_port = htons(PORT);

        // 4. 连接服务端
        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            printf("connect失败！错误码：%d\n", WSAGetLastError());
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }
        printf("已连接到服务端 %s:%d\n", SERVER_IP, PORT);
    }

    int sendMessage(const char* s) {
        // 发送消息到服务端
        strcpy_s(sendBuf, s);
        ret = send(clientSocket, sendBuf, strlen(sendBuf), 0);
        if (ret == SOCKET_ERROR) {
            printf("send失败！错误码：%d\n", WSAGetLastError());
            printf("尝试重新连接中。。。");
            init();
            Sleep(5000);
            sendMessage(s);
            printf("恢复连接");
            return 0;
        }
    }
    int exit() {
        // 6. 清理资源
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }
}
