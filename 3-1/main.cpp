#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include<time.h>

using namespace std;

using namespace std;
const int Mlenx = 510;
const char ACK = 0x03;
const char NAK = 0x07;
const char LAST_PACK = 0x18;
const char NOTLAST_PACK = 0x08;
const char SHAKE_1 = 0x01;
const char SHAKE_2 = 0x02;
const char SHAKE_3 = 0x04;
const char WAVE_1 = 0x80;
const char WAVE_2 = 0x40;
const int TIMEOUT = 500;//毫秒
char buffer[2000000];
int len;

SOCKADDR_IN serverAddr, clientAddr;
SOCKET server = socket(AF_INET, SOCK_DGRAM, 0); //选择udp协议


char sum_cal(char *arr, int lent) {
    if (lent == 0)
        return ~(0);
    char ret = arr[0];

    for (int i = 1; i < lent; i++) {
        ret = arr[i] + (char) ((int(arr[i]) + ret) % ((1 << 8) - 1));
    }
    return ~ret;
}

void wait_shake_hand() {
    while (1) {
        char recv[2];
        int begintime = clock();
        int lentmp = sizeof(clientAddr);
        while (recvfrom(server, recv, 2, 0, (sockaddr *) &clientAddr, &lentmp) == SOCKET_ERROR);
        if (sum_cal(recv, 2) != 0 || recv[1] != SHAKE_1)
            continue;
        recv[1] = SHAKE_2;
        recv[0] = sum_cal(recv + 1, 1);
        sendto(server, recv, 2, 0, (sockaddr *) &clientAddr, sizeof(clientAddr));
        while (recvfrom(server, recv, 2, 0, (sockaddr *) &clientAddr, &lentmp) == SOCKET_ERROR);
        if (sum_cal(recv, 2) != 0 || recv[1] != SHAKE_3) {
            printf("链接建立失败。\n");
            exit(0);
        }
        break;
    }
}

void recv_message(char *message, int &len_recv) {
    char recv[512];
    int lentmp = sizeof(clientAddr);
    int last_order=-1;
    while(1) {
        while(1) {
            while (recvfrom(server, recv, 512, 0, (sockaddr *) &clientAddr, &lentmp) == SOCKET_ERROR);
            char send[2];
            if (sum_cal(recv,512)==0)
            {
                send[1]=ACK;
                send[0]=sum_cal(send+1,1);
                sendto(server, recv, 2, 0, (sockaddr *) &clientAddr, sizeof(clientAddr));
                break;
            }else{
                send[1]=NAK;
                send[0]=sum_cal(send+1,1);
                sendto(server, recv, 2, 0, (sockaddr *) &clientAddr, sizeof(clientAddr));
                continue;
            }
        }
        if(last_order==recv[1])

        break;
    }

}

int main() {
    WSADATA wsadata;
    int nError = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (nError) {
        printf("start error\n");
        return 0;
    }

    int Port = 11451;

    serverAddr.sin_family = AF_INET; //使用ipv4
    serverAddr.sin_port = htons(Port); //端口
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);


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

    //等待接入
    printf("等待接入");
    wait_shake_hand();
    printf("用户已接入。\n正在接收数据...\n");
    recv_message(buffer, len);


    return 0;
}