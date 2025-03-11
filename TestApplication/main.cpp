#include <cstdio>
#include <chrono>
#include <thread>

#include "hello.h"

int main()
{
	printf("sizeof(char)=%d\n", sizeof(char));
    for (int i = 0; i < 10; i++) {
        printf("%s 向你问好! %d\n", "TestApplication", i);
        hello();
        hello(i);
        hello2();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 休眠 100 毫秒
    }
	int a = add(88, 9);
    printf("88+9=%d\n", a);
    printf("9+99=%d\n", add(9, 99));
    return 0;
}