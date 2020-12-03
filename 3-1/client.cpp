//
// Created by fengjk on 2020/12/3.
//
#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <fstream>
#include <vector>

using namespace std;

string buffer[2000000];
int len;

char sum_cal(char *arr, int lent) {
    if (lent == 0)
        return ~(0);
    char ret = arr[0];

    for (int i = 1; i < lent; i++) {
        ret = arr[i] + (char)((int(arr[i]) + ret) % ((1 << 8) - 1));
    }
    return ~ret;
}


int main() {
    WSADATA wsadata;
    int error = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (error) {
        printf("init error");
        return 0;
    }
    string serverip;
    while (1) {
        printf("请输入接收方ip地址\n");
        getline(cin, serverip);

        if (inet_addr(serverip.c_str()) == INADDR_NONE) {
            printf("ip地址不合法，555~\n");
            continue;
        }
        break;
    }

    int port = 11451;
    SOCKADDR_IN serverAddr, clientAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(serverip.c_str());

    SOCKET client = socket(AF_INET, SOCK_DGRAM, 0);
    if (client == INVALID_SOCKET) {
        printf("creat udp socket error");
        return 0;
    }
    string filename;
    while (1) {
        printf("请输入要发送的文件名：");
        cin >> filename;
        ifstream fin(filename.c_str(), fstream::binary);
        if (!fin) {
            printf("文件未找到，555~\n");
            continue;
        }
        char t = fin.get();
        while (!fin) {
            buffer[++len] = t;
            t = fin.get();
        }
        fin.close();
        break;
    }

    while (1) {


    }


    return 0;
}

