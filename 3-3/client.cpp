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
#include <queue>

using namespace std;
const int Mlenx = 240;
const unsigned char ACK = 0x03;
const unsigned char NAK = 0x07;
const unsigned char LAST_PACK = 0x18;
const unsigned char NOTLAST_PACK = 0x08;
const unsigned char SHAKE_1 = 0x01;
const unsigned char SHAKE_2 = 0x02;
const unsigned char SHAKE_3 = 0x04;
const unsigned char WAVE_1 = 0x80;
const unsigned char WAVE_2 = 0x40;
int WINDOW_SIZE;
const int TIMEOUT = 2000;//毫秒
char buffer[200000000];
int len;

SOCKET client;
SOCKADDR_IN serverAddr, clientAddr;

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
    sendto(client, tmp, tmp_len, 0, (sockaddr *) &serverAddr, sizeof(serverAddr));
    return true;
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

bool in_list[UCHAR_MAX + 1];

void send_message(char *message, int lent) {
    int ssthresh = UCHAR_MAX;
    WINDOW_SIZE = 1;
    int status = 0;// 0 慢启动 1 拥塞控制 2 快速恢复
    int now_window_has_send_succ = 0;
    queue<pair<int, int>> timer_list;//timer, order
    int leave_cnt = 0;
    static int base = 0;
    int has_send = 0;
    int last_ask = -1;
    int dupack_cnt = 0;
    int next_package = base;
    int has_send_succ = 0;
    int tot_package = lent / Mlenx + (lent % Mlenx != 0);

    while (1) {
        if (has_send_succ == tot_package)
            break;

        if (timer_list.size() < WINDOW_SIZE && has_send != tot_package) {
//            cerr << next_package << " windo=" << WINDOW_SIZE << endl;
            send_package(message + has_send * Mlenx,
                         has_send == tot_package - 1 ? lent - (tot_package - 1) * Mlenx : Mlenx,
                         next_package % ((int) UCHAR_MAX + 1),
                         has_send == tot_package - 1);
            timer_list.push(make_pair(clock(), next_package % ((int) UCHAR_MAX + 1)));
            in_list[next_package % ((int) UCHAR_MAX + 1)] = 1;
            next_package++;
            has_send++;

        }

        char recv[3];
        int lentmp = sizeof(serverAddr);
        bool my_tmp_save = (recvfrom(client, recv, 3, 0, (sockaddr *) &serverAddr, &lentmp) != SOCKET_ERROR &&
                            sum_cal(recv, 3) == 0 &&
                            recv[1] == ACK);
//        if (my_tmp_save)
//            cerr << "ack " << (unsigned int) recv[2] << endl;
        if (my_tmp_save && in_list[(unsigned char) recv[2]]) {

            while (timer_list.front().second != (unsigned char) recv[2]) {
                has_send_succ++;
                base++;
                now_window_has_send_succ++;
                in_list[timer_list.front().second] = 0;
                timer_list.pop();
            }
            in_list[timer_list.front().second] = 0;
            has_send_succ++;
            now_window_has_send_succ++;
            base++;
            leave_cnt = 0;
            timer_list.pop();
            last_ask = (unsigned char) recv[2];
            if (now_window_has_send_succ >= WINDOW_SIZE) {
                now_window_has_send_succ -= WINDOW_SIZE;
                if (status == 0 && WINDOW_SIZE < ssthresh) {
                    WINDOW_SIZE *= 2;
                    WINDOW_SIZE %= UCHAR_MAX;
                } else if (status == 0 && WINDOW_SIZE >= ssthresh) {
                    WINDOW_SIZE += 1;
                    WINDOW_SIZE %= UCHAR_MAX;
                    status = 1;
                } else if (status == 1 || status == 2) {
                    WINDOW_SIZE += 1;
                    WINDOW_SIZE %= UCHAR_MAX;
                    status = 1;
                }
            }
            dupack_cnt = 0;

        } else {
            if (my_tmp_save) {
                if (last_ask == (unsigned char) recv[2]) {
                    dupack_cnt++;
                    if (status == 2) {
                        WINDOW_SIZE++;
                        WINDOW_SIZE %= UCHAR_MAX;
                    }
                }
            }
            if ((dupack_cnt == 3 && status != 2) || clock() - timer_list.front().first > TIMEOUT) {
                next_package = base;
                leave_cnt++;
                has_send -= timer_list.size();
                while (!timer_list.empty()) timer_list.pop();
//                cerr << "pack_loss" << endl;
                if (dupack_cnt == 3) {
                    now_window_has_send_succ = 0;
                    status = 2;
                    dupack_cnt = 0;
                    ssthresh = WINDOW_SIZE / 2;
                    WINDOW_SIZE = ssthresh + 3;

                } else {
                    now_window_has_send_succ = 0;
                    status = 0;
                    ssthresh = WINDOW_SIZE / 2;
                    WINDOW_SIZE = 1;

                }
            }
        }

        if (leave_cnt >= 5) {
            wave_hand();
            return;
        }




//    queue<pair<int, int>> timer_list;
//    static int base = 0;
//    int next_package = base;
//
//    int has_send_succ = 0;
//    int ssthresh = UCHAR_MAX;
//    WINDOW_SIZE = 1;
//    int status = 0;// 0 慢启动 1 拥塞控制 2 快速恢复
//    int now_window_has_send_succ = 0;
//
//    while (1) {
//        if (has_send_succ == tot_package)
//            break;
//
        if (base % 100 == 0)
            printf("此文件已经发送%.2f%%\n", (float) base / tot_package * 100);
    }

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
        printf("请输入接收方ip地址:\n");
        getline(cin, serverip);

        if (inet_addr(serverip.c_str()) == INADDR_NONE) {
            printf("ip地址不合法，555~\n");
            continue;
        }
        break;
    }

    int port = 4545;

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(serverip.c_str());
    client = socket(AF_INET, SOCK_DGRAM, 0);
    //设置非阻塞
    int time_out = 1;//1ms超时
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (char *) &time_out, sizeof(time_out));

    if (client == INVALID_SOCKET) {
        printf("creat udp socket error");
        return 0;
    }
    string filename;
    while (1) {
        printf("请输入要发送的文件名：");
        cin >> filename;
        ifstream fin(filename.c_str(), ifstream::binary);
        if (!fin) {
            printf("文件未找到，555~\n");
            continue;
        }
        unsigned char t = fin.get();
        while (fin) {
            buffer[len++] = t;
            t = fin.get();
        }
        fin.close();
        break;
    }
//    printf("请输入发送窗口大小：\n");
//    cin >> WINDOW_SIZE;
    WINDOW_SIZE = 1;
    WINDOW_SIZE %= UCHAR_MAX; //防止窗口大小大于序号域长度
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

