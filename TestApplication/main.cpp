#include <cstdio>
#include <chrono>
#include <thread>       //std::this_thread::sleep_for   std::chrono::milliseconds
#include <atomic>
#include <unistd.h>     //sleep(1)
#include <signal.h>
#include <iostream>

#include "../include/hello.h"
#include "../include/higplat.h"
#include "../include/qbdtype.h"

std::atomic<bool> g_running(true);  // 控制线程运行的标志

void threadFunction1();
void threadFunction2();

int main()
{
    // 创建并启动线程
    std::thread worker1(threadFunction1);
    std::thread worker2(threadFunction2);

    // 或者分离线程让它独立运行
    //worker.detach();

    // 主线程可以继续做其他工作...
    // 主线程等待键盘输入
    std::string input;
    while (true) {
        std::cin >> input;  // 等待用户输入
        if (input == "q") {
            g_running = false;  // 通知线程退出
            std::cout << "exiting..." << std::endl;
            break;
        }
        else {
            std::cout << "unknown command，input 'q' exit" << std::endl;
        }
    }

    // 如果需要等待线程完成（虽然这个线程是无限循环）
    worker1.join();
	worker2.join(); 

    std::cout << "main thread exit" << std::endl;

    return 0;
}


//int main()
//{
//	printf("size = %d\n", sizeof(MyStruct));
//
//	int conngplat;
//	conngplat = connectgplat("127.0.0.1", 8777);
//
//	MyStruct myStruct;
//	myStruct.a = 1;
//	myStruct.b = 1;
//	myStruct.c = 1.999f;
//
//	int a = 777;
//
//	unsigned int error;
//
//	subscribe(conngplat, "int1", &error);
//
//	auto start = std::chrono::high_resolution_clock::now();
//	
//	//for (int i = 0; i < 10; i++)
//	//{
//	//	myStruct.a++;
//	//	myStruct.b++;
//	//	myStruct.c++;
//
//	//	writeq(conngplat, "MyStruct1", &myStruct, sizeof(myStruct), &error); // 发送数据
//	//	myStruct.a = 11;
//	//	myStruct.b = 22;
//	//	myStruct.c = 0;
//	//	readq(conngplat, "MyStruct1", &myStruct, sizeof(myStruct), &error); // 接收数据
//	//	printf("接收数据：%d %d %f error=%d\n", myStruct.a, myStruct.b, myStruct.c, error);
//
//	//	a++;
//	//	writeb(conngplat, "int1", &a, sizeof(a), &error); // 接收数据
//	//	a = 0;
//	//	readb(conngplat, "int1", &a, sizeof(a), &error); // 接收数据
//	//	printf("a=%d\n", a);
//
//	//	writeb(conngplat, "mystruct1", &myStruct, sizeof(myStruct), &error); // 发送数据
//	//	myStruct.a = 11;
//	//	myStruct.b = 22;
//	//	myStruct.c = 0;
//	//	readb(conngplat, "mystruct1", &myStruct, sizeof(myStruct), &error); // 接收数据
//	//	printf("接收数据：%d %d %f error=%d\n", myStruct.a, myStruct.b, myStruct.c, error);
//
//	//	printf("\n");
//
//	//	//sleep(0.5);
//	//	//std::this_thread::sleep_for(std::chrono::milliseconds(100));
//	//}
//
//	auto end = std::chrono::high_resolution_clock::now();
//	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
//
//	std::cout << "执行时间: " << duration.count() << " 微秒" << std::endl;
//
//
//	std::string eventname;
//
//	while (1)
//	{
//		waitpostdata(conngplat, eventname, 1000, &error); // 等待数据到达
//		printf("收到事件：%s error=%d\n", eventname.c_str(), error);
//	}
//
//	//while (1)
//	//{
//	//	eventname = waitpostdata(conngplat, eventname, tagsize, 1000, &error); // 等待数据到达
//	//	if (eventname == "int1")
//	//	{
//	//		int a = 0;
//	//		readb(conngplat, eventname.c_str(), &a, sizeof(a), &error); // 接收数据
//	//		printf("接收数据：%d error=%d\n", a, error);
//	//	}
//	//	else if (eventname == "MyStruct1")
//	//	{
//	//		MyStruct myStruct;
//	//		readq(conngplat, eventname.c_str(), &myStruct, sizeof(myStruct), &error); // 接收数据
//	//		printf("接收数据：%d %d %f error=%d\n", myStruct.a, myStruct.b, myStruct.c, error);
//	//	}
//	//	else if (eventname == "WAIT_TIMEOUT")
//	//	{
//	//		break;
//	//	}
//	//}
//
//	printf("程序执行完毕\n");
//	
//    return 0;
//}