#include <cstdio>
#include <chrono>
#include <thread>       //std::this_thread::sleep_for   std::chrono::milliseconds
#include <unistd.h>     //sleep(1)
#include <signal.h>

#include "../include/hello.h"
#include "../include/higplat.h"
#include "../include/qbdtype.h"

int main()
{
	printf("size = %d\n", sizeof(MyStruct));

	int conngplat;
	conngplat = connectgplat("127.0.0.1", 8777);

	MyStruct myStruct;
	myStruct.a = 1;
	myStruct.b = 2;
	myStruct.c = 1.01;

	unsigned int error;

	//readq(conngplat, "MyStruct1", &myStruct, sizeof(myStruct), &error); // 接收数据
	//readq(conngplat, "MyStruct1", &myStruct, sizeof(myStruct), &error); // 接收数据
	//readq(conngplat, "MyStruct1", &myStruct, sizeof(myStruct), &error); // 接收数据
	//readq(conngplat, "MyStruct1", &myStruct, sizeof(myStruct), &error); // 接收数据
	//readq(conngplat, "MyStruct1", &myStruct, sizeof(myStruct), &error); // 接收数据
	//readq(conngplat, "MyStruct1", &myStruct, sizeof(myStruct), &error); // 接收数据
	//readq(conngplat, "MyStruct1", &myStruct, sizeof(myStruct), &error); // 接收数据
	//readq(conngplat, "MyStruct1", &myStruct, sizeof(myStruct), &error); // 接收数据
	//readq(conngplat, "MyStruct1", &myStruct, sizeof(myStruct), &error); // 接收数据
	//readq(conngplat, "MyStruct1", &myStruct, sizeof(myStruct), &error); // 接收数据

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

		//sleep(0.5);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	
    return 0;
}