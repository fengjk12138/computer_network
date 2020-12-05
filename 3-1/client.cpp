//
// Created by fengjk on 2020/12/3.
//
#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <time.h>

using namespace std;
const int Mlenx = 509;
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

SOCKET client = socket(AF_INET, SOCK_DGRAM, 0);
SOCKADDR_IN serverAddr, clientAddr;

char sum_cal(char *arr, int lent) {
    if (lent == 0)
        return ~(0);
    char ret = arr[0];

    for (int i = 1; i < lent; i++) {
        ret = arr[i] + (char) ((int(arr[i]) + ret) % ((1 << 8) - 1));
    }
    return ~ret;
}

bool send_package(char *message, int lent, int order, int last = 0) {
    if (lent > Mlenx) {
        return false;
    }
    if (last == false && lent != Mlenx) {
        return false;
    }
    char *tmp;
    int tmp_len;

    if (last) {
        tmp = new char[lent + 4];
        tmp[1] = LAST_PACK;
        tmp[2] = order;
        tmp[3] = lent;
        for (int i = 4; i < lent + 4; i++)
            tmp[i] = message[i - 4];
        tmp[0] = sum_cal(tmp + 1, lent + 3);
        tmp_len = lent + 4;
    } else {
        tmp = new char[lent + 3];
        tmp[1] = NOTLAST_PACK;
        tmp[2] = order;
        for (int i = 3; i < lent + 3; i++)
            tmp[i] = message[i - 3];
        tmp[0] = sum_cal(tmp + 1, lent + 2);
        tmp_len = lent + 3;
    }
    while (1) {
        sendto(client, tmp, tmp_len, 0, (sockaddr *) &serverAddr, sizeof(serverAddr));
        int begintime = clock();
        char recv[3];
        int lentmp = sizeof(serverAddr);
        int fail_send = 0;
        while (recvfrom(client, recv, 3, 0, (sockaddr *) &serverAddr, &lentmp) == SOCKET_ERROR)
            if (clock() - begintime > TIMEOUT) {
                fail_send = 1;
                break;
            }
        if (fail_send == 0 && sum_cal(recv, 3) == 0 && recv[1] == ACK && recv[2] == order)
            return true;
    }
}


void shake_hand() {
    while (1) {
        //发送shake_1
        char tmp[2];
        tmp[1] = SHAKE_1;
        tmp[0] = sum_cal(tmp + 1, 1);
        sendto(client, tmp, 2, 0, (sockaddr *) &serverAddr, sizeof(serverAddr));
        int begintime = clock();
        char recv[2];
        int lentmp = sizeof(clientAddr);
        int fail_send = 0;
        while (recvfrom(client, recv, 2, 0, (sockaddr *) &serverAddr, &lentmp) == SOCKET_ERROR)
            if (clock() - begintime > TIMEOUT) {
                fail_send = 1;
                break;
            }
        //接受shake_2并校验
        if (fail_send == 0 && sum_cal(recv, 2) == 0 && recv[1] == SHAKE_2) {
            {
                //发送shake_3
                tmp[1] = SHAKE_3;
                tmp[0] = sum_cal(tmp + 1, 1);
                sendto(client, tmp, 2, 0, (sockaddr *) &serverAddr, sizeof(serverAddr));
                break;
            }
        }
    }
}

void wave_hand() {
    int tot_fail = 0;
    while (1) {
        //发送wave_1
        char tmp[2];
        tmp[1] = WAVE_1;
        tmp[0] = sum_cal(tmp + 1, 1);
        sendto(client, tmp, 2, 0, (sockaddr *) &serverAddr, sizeof(serverAddr));
        int begintime = clock();
        char recv[2];
        int lentmp = sizeof(serverAddr);
        int fail_send = 0;
        while (recvfrom(client, recv, 2, 0, (sockaddr *) &serverAddr, &lentmp) == SOCKET_ERROR)
            if (clock() - begintime > TIMEOUT) {
                fail_send = 1;
                tot_fail++;
                break;
            }
        //接受wave_2并校验
        if (fail_send == 0 && sum_cal(recv, 2) == 0 && recv[1] == WAVE_2)
            break;
        else {
            if (tot_fail == 3) {
                printf("断开失败，正在自动释放资源...");
                break;
            }
            continue;
        }
    }
}


void send_message(char *message, int lent) {
    int package_num = lent / Mlenx + (lent % Mlenx != 0);
    static int order = 0;
    for (int i = 0; i < package_num; i++)
        send_package(message + i * Mlenx, i == package_num - 1 ? lent - (package_num - 1) * Mlenx : Mlenx, order++,
                     i == package_num - 1);
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

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(serverip.c_str());

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
    printf("链接建立中...\n");
    shake_hand();
    printf("链接建立完成。 \n正在发送信息...\n");
    send_message((char *) (filename.c_str()), filename.length());
    printf("文件名发送完毕，正在发送文件内容...\n");
    send_message(buffer, len);
    printf("文件内容发送完毕。\n 开始断开连接...\n");
    wave_hand();
    printf("连接已断开。");
    closesocket(client);
    WSACleanup();

    return 0;
}

