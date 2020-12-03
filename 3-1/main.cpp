#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <fstream>
#include <vector>

using namespace std;

string buffer[2000000];




int main() {
    WSADATA wsadata;
    int nError = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (nError) {
        printf("start error\n");
        return 0;
    }

    int Port = 11451;
    SOCKADDR_IN serverAddr, clinentAddr;
    serverAddr.sin_family = AF_INET; //使用ipv4
    serverAddr.sin_port = htons(Port); //端口
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    SOCKET server = socket(AF_INET, SOCK_DGRAM, 0); //选择udp协议

    if (server == INVALID_SOCKET) {
        printf("create fail");
        closesocket(server);
        return 0;
    }
    int retVAL = bind(server, (sockaddr *) (&serverAddr), sizeof(serverAddr));
    if (retVAL == SOCKET_ERROR) {
        printf("bind fail");
        closesocket(server);
        WSACleanup();
        return 0;
    }

    int len = sizeof(clinentAddr);

    ifstream fin("../helloworld.txt");

    if (!fin) {
        printf("open file error");
        return 0;
    }
    int cnt = 0;
    while (getline(fin, buffer[cnt++]));
    



    return 0;
}