#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include<time.h>

using namespace std;

using namespace std;
const int Mlenx = 250;
const unsigned char ACK = 0x03;
const unsigned char NAK = 0x07;
const unsigned char LAST_PACK = 0x18;
const unsigned char NOTLAST_PACK = 0x08;
const unsigned char SHAKE_1 = 0x01;
const unsigned char SHAKE_2 = 0x02;
const unsigned char SHAKE_3 = 0x04;
const unsigned char WAVE_1 = 0x80;
const unsigned char WAVE_2 = 0x40;
const int TIMEOUT = 500;//毫秒
char buffer[200000000];
int len;

SOCKADDR_IN serverAddr, clientAddr;
SOCKET server; //选择udp协议


unsigned char sum_cal(char *arr, int lent) {
    if (lent == 0)
        return ~(0);
    unsigned char ret = arr[0];

    for (int i = 1; i < lent; i++) {
        unsigned int tmp = ret + (unsigned char) arr[i];
        tmp = tmp / (1 << 8) + tmp % (1 << 8);
        tmp = tmp / (1 << 8) + tmp % (1 << 8);
        ret = tmp;
    }
    return ~ret;
}

void wait_shake_hand() {
    while (1) {
        char recv[2];
        int connect = 0;
        int lentmp = sizeof(clientAddr);
        while (recvfrom(server, recv, 2, 0, (sockaddr *) &clientAddr, &lentmp) == SOCKET_ERROR);
        if (sum_cal(recv, 2) != 0 || recv[1] != SHAKE_1)
            continue;

        while (1) {
            recv[1] = SHAKE_2;
            recv[0] = sum_cal(recv + 1, 1);
            sendto(server, recv, 2, 0, (sockaddr *) &clientAddr, sizeof(clientAddr));
            while (recvfrom(server, recv, 2, 0, (sockaddr *) &clientAddr, &lentmp) == SOCKET_ERROR);
            if (sum_cal(recv, 2) == 0 && recv[1] == SHAKE_1)
                continue;
            if (sum_cal(recv, 2) != 0 || recv[1] != SHAKE_3) {
                printf("链接建立失败，请重启客户端。");
                connect = 1;
            }
            break;
        }
        if (connect == 1)
            continue;
        break;
    }
}

void wait_wave_hand() {
    while (1) {
        char recv[2];
        int lentmp = sizeof(clientAddr);
        while (recvfrom(server, recv, 2, 0, (sockaddr *) &clientAddr, &lentmp) == SOCKET_ERROR);
        if (sum_cal(recv, 2) != 0 || recv[1] != (char) WAVE_1)
            continue;
        recv[1] = WAVE_2;
        recv[0] = sum_cal(recv + 1, 1);
        sendto(server, recv, 2, 0, (sockaddr *) &clientAddr, sizeof(clientAddr));
        break;
    }
}

void recv_message(char *message, int &len_recv) {
    char recv[Mlenx + 4];
    int lentmp = sizeof(clientAddr);
    static unsigned char last_order = 0;
    len_recv = 0;
    while (1) {
        while (1) {
            memset(recv, 0, sizeof(recv));
            while (recvfrom(server, recv, Mlenx + 4, 0, (sockaddr *) &clientAddr, &lentmp) == SOCKET_ERROR);
            char send[3];
            int flag = 0;
            if (sum_cal(recv, Mlenx + 4) == 0 && (unsigned char) recv[2] == last_order) {
                last_order++;
                flag = 1;
            }


            send[1] = ACK;
            send[2] = last_order - 1;
            send[0] = sum_cal(send + 1, 2);
            sendto(server, send, 3, 0, (sockaddr *) &clientAddr, sizeof(clientAddr));
            if (flag)
                break;
        }

        if (LAST_PACK == recv[1]) {
            for (int i = 4; i < recv[3] + 4; i++)
                message[len_recv++] = recv[i];

            break;
        } else {
            for (int i = 3; i < Mlenx + 3; i++)
                message[len_recv++] = recv[i];
        }
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

    server = socket(AF_INET, SOCK_DGRAM, 0);
    //设置非阻塞
    int time_out = 1;//1ms超时
    setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, (char *) &time_out, sizeof(time_out));

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
    printf("等待接入...\n");
    wait_shake_hand();
    printf("用户已接入。\n正在接收数据...\n");
    recv_message(buffer, len);
    printf("第一条信息接收成功。");
    buffer[len] = 0;
    cout << buffer << endl;
    string file_name(buffer);
    printf("开始接收第二条信息...\n");
    recv_message(buffer, len);
    printf("第二条信息接收成功。\n");
    ofstream fout(file_name.c_str(), ofstream::binary);
    for (int i = 0; i < len; i++)
        fout << buffer[i];
    fout.close();
    wait_wave_hand();
    printf("链接断开");
    return 0;
}