#include <cstdio>
#include <chrono>
#include <thread>       //std::this_thread::sleep_for   std::chrono::milliseconds
#include <unistd.h>     //sleep(1)
#include <signal.h>

#include "..//include//hello.h"
#include "..//include//higplat.h"
#include "..//include//qbdtype.h"

int main()
{
	//printf("size = %d\n", sizeof(MyStruct));

	int conngplat;
	conngplat = connectgplat("127.0.0.1", 8777);

	MyStruct myStruct;
	myStruct.a = 1;
	myStruct.b = 2;

	unsigned int error;
	writeq(conngplat, "MyStruct1", & myStruct, sizeof(myStruct), &error); // 发送数据

    return 0;
}