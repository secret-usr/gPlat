
//内存分配有关

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ngx_c_memory.h"

//类静态成员赋值
CMemory* CMemory::m_instance = NULL;

//分配内存
//memCount：分配的字节大小
//ifmemset：是否要把分配的内存初始化为0；
void* CMemory::AllocMemory(int memCount, bool ifmemset)
{
	void* tmpData = (void*)new char[memCount]; //不判断结果，如果new失败，程序根本不应该继续运行，就让它崩溃以方便排错
	if (ifmemset)
	{
		memset(tmpData, 0, memCount);
	}
	return tmpData;
}

//内存释放函数
void CMemory::FreeMemory(void* point)
{
	delete[]((char*)point); //new的时候是char *，这里转换回char *，以免出现编译警告：warning: deleting ‘void*’ is undefined [-Wdelete-incomplete]
}

