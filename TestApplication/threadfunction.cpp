#include <thread>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <iostream>
#include <string.h>

#include "../include/hello.h"
#include "../include/higplat.h"
#include "../include/qbdtype.h"

extern std::atomic<bool> g_running;  // 控制线程运行的标志

void threadFunction1() {
    int conngplat;
    conngplat = connectgplat("127.0.0.1", 8777);
    bool ret{ false };
    unsigned int error;
    //subscribe(conngplat, "int1", &error);
	subscribedelaypost(conngplat, "int1", "int1_delay500", 1000, &error); // 延时订阅
    //subscribe(conngplat, "string1", &error);
    //subscribe(conngplat, "timer_500ms", &error);
    //subscribe(conngplat, "timer_1s", &error);
    //subscribe(conngplat, "timer_2s", &error);
    //subscribe(conngplat, "timer_3s", &error);
    //subscribe(conngplat, "timer_5s", &error);

    int a = 0;
    std::string eventname;
    while (g_running) {  // 检查全局运行标志 {
        waitpostdata(conngplat, eventname, -1, &error); // 等待数据到达
        //if (eventname != "int1") {
            printf("eventname=%s, error=%d\n", eventname.c_str(), error);
        //}
        if (eventname == "int1") {
            int a = 0;
            ret = readb(conngplat, "int1", &a, sizeof(a), &error); // 接收数据
            if(!ret) {
                printf("readb failed, error=%d\n", error);
			}
            //printf("readb a=%d error=%d\n", a, error);
        }
        else if (eventname == "int1_delay") {
            ret = readb(conngplat, "int1", &a, sizeof(a), &error); // 接收数据
            if (!ret) {
                printf("readb failed, error=%d\n", error);
            }
            printf("readb a=%f error=%d\n", a, error);
        }
        else if (eventname == "int2") {
            ret = readb(conngplat, "int2", &a, sizeof(a), &error); // 接收数据
            if (!ret) {
                printf("readb failed, error=%d\n", error);
            }
            //printf("readb b=%f error=%d\n", a, error);
        }
        else if (eventname == "string1") {
            std::string str2;
            ret = readb_string2(conngplat, "string1", str2, &error); // 接收数据
            if (ret) {
                printf("readb_string2 str2=%s error=%d\n", str2.c_str(), error);
            }
            else {
                printf("readb_string2 failed, error=%d\n", error);
            }
        }
    }
    printf("work thread exit\n");
}

void threadFunction2() {
    int conngplat;
    conngplat = connectgplat("127.0.0.1", 8777);
    bool ret{ false };
    unsigned int error;

    //std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    int a = 0;
    std::string eventname;
    while (g_running) {  // 检查全局运行标志 {
        a++;
		ret = writeb(conngplat, "int1", &a, sizeof(a), &error); // 接收数据;
        if (!ret) {
            printf("writeb failed, error=%d\n", error);
		}
        //printf("writeb a=%d error=%d\n", a, error);
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    printf("work thread exit\n");
}

void threadFunction3() {
    int conngplat;
    conngplat = connectgplat("127.0.0.1", 8777);
    bool ret{ false };
    unsigned int error;

    int a = 0;
    std::string eventname;
    while (g_running) {
        std::string suffix = std::to_string(a++);
        char str1[200] = "hello world, gyb 77777777777777777777777777777777777777777777777777777777777777777777777777777 loop=";
        strcat(str1, suffix.c_str()); // 将数字转换为字符串并复制到 str1
        ret = writeb_string(conngplat, "string1", str1, &error); // 发送数据

        //char str2[1000] = { 0 };
        //ret = readb_string(conngplat, "string1", str2, sizeof(str2), &error); // 接收数据
        //if (ret) {
        //    printf("readb_string str2=%s error=%d\n", str2, error);
        //}
        //else {
        //    printf("writeb_string failed, error=%d\n", error);
        //}

        std::string str2;
        ret = readb_string2(conngplat, "string1", str2, &error); // 接收数据
        //if (ret) {
        //    printf("readb_string2 str2=%s error=%d\n", str2.c_str(), error);
        //}
        //else {
        //    printf("readb_string2 failed, error=%d\n", error);
        //}

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    printf("work thread exit\n");   
}