#include <cstdio>
#include <chrono>
#include <thread>       //std::this_thread::sleep_for   std::chrono::milliseconds
#include <unistd.h>     //sleep(1)
#include <signal.h>

#include "..//include//hello.h"

int main()
{
	//signal(SIGHUP, SIG_IGN);  // 忽略 SIGHUP 信号
	printf("sizeof(char)=%d\n", sizeof(char));
    for (int i = 0; i < 200; i++) {
        printf("%s 向你问好啦!!! %d\n", "TestApplication", i);
        hello();
        hello(i);
        hello2();
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 休眠 100 毫秒
		sleep(1);  // 休眠 1 秒
    }
	int a = add(88, 9);
    printf("88+9=%d\n", a);
    printf("9+99=%d\n", add(9, 99));
    return 0;
}