#include <thread>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <iostream>
#include <string.h>

#include "../include/higplat.h"

extern std::atomic<bool> g_running;  // 控制线程运行的标志

void threadWritePlc() {
    int conngplat;
    conngplat = connectgplat("127.0.0.1", 8777);
    bool ret{ false };
    unsigned int error;
    //此线程只能subscribe定时器，不能subscribe普通tag，否则可能会出现问题，因为这个线程是专门用来写PLC的
    subscribe(conngplat, "timer_500ms", &error);    //用于控制线程退出的定时器
    registertag(conngplat, "int1", &error);

    int a = 0;
    std::string tagname;
    while (g_running) {  // 检查全局运行标志
		char value[1024] = { 0 };
        waitpostdata(conngplat, tagname, value, 1024, -1, &error); // 等待数据到达
        
        if (tagname == "timer_500ms") {
            continue;
        }

        // tag的值保存在value中，tag的名字保存在tagname中，可以根据需要进行处理
    }
    printf("work thread exit\n");
}