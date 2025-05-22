#include <cstdio>
#include <chrono>
#include <thread>       //std::this_thread::sleep_for   std::chrono::milliseconds
#include <unistd.h>     //sleep(1)
#include <signal.h>

#include "../include/hello.h"
#include "../include/higplat.h"
#include "../include/qbdtype.h"
#include <iostream>

int main()
{
	printf("size = %d\n", sizeof(MyStruct));

	int conngplat;
	conngplat = connectgplat("127.0.0.1", 8777);

	MyStruct myStruct;
	myStruct.a = 1;
	myStruct.b = 1;
	myStruct.c = 1.999;

	int a = 777;

	unsigned int error;

	//subscribe(conngplat, "int1", &error);

	auto start = std::chrono::high_resolution_clock::now();
	
	for (int i = 0; i < 10; i++)
	{
		myStruct.a++;
		myStruct.b++;
		myStruct.c++;

		writeq(conngplat, "MyStruct1", &myStruct, sizeof(myStruct), &error); // 发送数据
		myStruct.a = 11;
		myStruct.b = 22;
		myStruct.c = 0;
		readq(conngplat, "MyStruct1", &myStruct, sizeof(myStruct), &error); // 接收数据
		printf("接收数据：%d %d %f error=%d\n", myStruct.a, myStruct.b, myStruct.c, error);

		a++;
		writeb(conngplat, "int1", &a, sizeof(a), &error); // 接收数据
		a = 0;
		readb(conngplat, "int1", &a, sizeof(a), &error); // 接收数据
		printf("a=%d\n", a);

		writeb(conngplat, "mystruct1", &myStruct, sizeof(myStruct), &error); // 发送数据
		myStruct.a = 11;
		myStruct.b = 22;
		myStruct.c = 0;
		readb(conngplat, "mystruct1", &myStruct, sizeof(myStruct), &error); // 接收数据
		printf("接收数据：%d %d %f error=%d\n", myStruct.a, myStruct.b, myStruct.c, error);

		printf("\n");

		//sleep(0.5);
		//std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

	std::cout << "执行时间: " << duration.count() << " 微秒" << std::endl;


	std::string eventname;
	int tagsize = 0;
	
	while (1)
	{
		eventname = waitpostdata(conngplat, eventname, tagsize, 1000, &error); // 等待数据到达
		if (eventname == "int1")
		{
			int a = 0;
			readb(conngplat, eventname.c_str(), &a, sizeof(a), &error); // 接收数据
			printf("接收数据：%d error=%d\n", a, error);
		}
		else if (eventname == "MyStruct1")
		{
			MyStruct myStruct;
			readq(conngplat, eventname.c_str(), &myStruct, sizeof(myStruct), &error); // 接收数据
			printf("接收数据：%d %d %f error=%d\n", myStruct.a, myStruct.b, myStruct.c, error);
		}
		else if (eventname == "WAIT_TIMEOUT")
		{
			break;
		}
	}
	eventname = waitpostdata(conngplat, eventname, tagsize, 1000, &error); // 等待数据到达

	printf("程序执行完毕\n");
	
    return 0;
}