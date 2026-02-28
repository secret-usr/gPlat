#include <cstdio>
#include <chrono>
#include <thread>       //std::this_thread::sleep_for   std::chrono::milliseconds
#include <atomic>
#include <signal.h>
#include <iostream>
#include <cassert>
#include <list>
#include <string.h>

#include "../include/higplat.h"
#include "../include/user_types.h"

int main()
{
    int h;
    unsigned int  err;
    bool   ret = 0;

    h = connectgplat("127.0.0.1", 8777);
    if (h < 0)
    {
		printf("连接失败，error=%d\n", h);
        return 0;
    }
    printf("连接成功\n");

	SensorData sensorData;
	sensorData.alarm = true;
	sensorData.humidity = 80;
	sensorData.pressure = 1013.25;
	sensorData.temperature = 25;
	sensorData.location = "车间1-区域A";

	ret = writeb(h, "sensor1", &sensorData, sizeof(sensorData), &err);
	assert(ret);

	printf("exit\n");

	return 0;
}