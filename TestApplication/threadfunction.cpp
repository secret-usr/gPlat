#include <thread>
#include <string>
#include <cstdio>
#include <atomic>
#include <iostream>

#include "../include/hello.h"
#include "../include/higplat.h"
#include "../include/qbdtype.h"

extern std::atomic<bool> g_running;  // 控制线程运行的标志

void threadFunction1() {
    int conngplat;
    conngplat = connectgplat("127.0.0.1", 8777);

    unsigned int error;
    subscribe(conngplat, "int1", &error);

    std::string eventname;
    while (g_running) {  // 检查全局运行标志 {
        waitpostdata(conngplat, eventname, 1000, &error); // 等待数据到达
        printf("event received: %s error=%d\n", eventname.c_str(), error);
    }

    printf("work thread exit\n");
}

void threadFunction2() {
    int conngplat;
    conngplat = connectgplat("127.0.0.1", 8777);

    unsigned int error;
    subscribe(conngplat, "int1", &error);

	int a = 0;
    std::string eventname;
    while (g_running) {  // 检查全局运行标志 {
        a++;
		writeb(conngplat, "int1", &a, sizeof(a), &error); // 接收数据;
        printf("writeb a=%d error=%d\n", a, error);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    printf("work thread exit\n");
}