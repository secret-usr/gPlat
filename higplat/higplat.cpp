#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/msg.h"
#include "qbd.h"

namespace fs = std::filesystem;

thread_local  unsigned int errorCode = 0;
char dataQuePath[100];

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
Function: gettime

Summary:  Get current time.

Args:     char *timebuf
buffer to save time

Returns:  void
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
inline void gettime(char * timebuf)
{
	struct tm* now;
	time_t ltime;
	time(&ltime);
	now = localtime(&ltime);
	strftime(timebuf, 20, "%Y-%m-%d %H:%M:%S", now);
}

extern "C" {
	int add(int a, int b) {
		return a + b;
	}
}

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
Function: CreateQ

Summary:  Create queue file according the specified size and tye.

Args:     LPCTSTR  lpFileName
queue/file name
int recordSize
record size
int recordNum
total number of record in queue
int dataType
1 ASCII, 0 BINARY
int operateMode
1 shift queue, 0 general queue

Returns:  BOOL
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
extern "C" bool CreateQ(const char* lpFileName,
	int recordSize,
	int recordNum,
	int dateType,
	int operateMode,
	void* pType,
	int typeSize)
{
	if (recordSize > MAXMSGLEN || typeSize > TYPEMAXSIZE)
	{
		errorCode = ERROR_PARAMETER_SIZE;
		return false;
	}

	// 检查文件名长度是否超过定义长度
	if (strlen(lpFileName) > MAXDQNAMELENTH)
	{
		errorCode = ERROR_FILENAME_TOO_LONG;
		return false;
	}

	// 形成文件路径全名
	char dqFileName[100];
	strcpy(dqFileName, dataQuePath);
	strcat(dqFileName, lpFileName);

	// 先查找同名文件删除之
	if (fs::exists(dqFileName))
	{
		if (!fs::remove(dqFileName)) {		//C++17
			errorCode = ERROR_FILE_IN_USE;
			return false;
		}
	}

	// 创建文件
	try {
		// 创建父目录（如果不存在）
		fs::path path(dqFileName);
		if (path.has_parent_path() && !fs::exists(path.parent_path())) {
			fs::create_directories(path.parent_path());
		}

		// 创建文件
		std::ofstream newfile(dqFileName);
		if (newfile.is_open()) {
			newfile.close();
		}
		else {
			errorCode = ERROR_FILE_CREATE_FAILSURE;
			return false;
		}
	}
	catch (const std::exception& e) {
		std::cerr << "create file error: " << e.what() << std::endl;
		errorCode = ERROR_FILE_CREATE_FAILSURE;
		return false;
	}

	// 计算文件大小
	long fileSize;
	fileSize = recordNum * (recordSize + RECORDHEADSIZE) + QUEUEHEADSIZE + TYPEMAXSIZE;

	// 1. 创建或打开文件（O_CREAT | O_RDWR 表示不存在则创建，存在则打开可读写）
	int fd = open(dqFileName, O_CREAT | O_RDWR, 0644);
	if (fd == -1) {
		std::cerr << "打开/创建文件失败: " << strerror(errno) << std::endl;
		return false;
	}

	// 2. 将文件扩展至目标大小
	if (ftruncate(fd, fileSize) == -1) {
		std::cerr << "调整文件大小失败: " << strerror(errno) << std::endl;
		close(fd);
		return false;
	}

	// 3. 内存映射文件
	void* lpMapAddress = mmap(nullptr, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (lpMapAddress == MAP_FAILED) {
		std::cerr << "内存映射失败: " << strerror(errno) << std::endl;
		close(fd);
		return false;
	}

	// 4. 使用映射内存（示例：填充零）
	memset(lpMapAddress, 0, fileSize);

	QUEUE_HEAD* pDqHead;
	pDqHead = (QUEUE_HEAD*)lpMapAddress;
	pDqHead->qbdtype = QUEUE_T;
	pDqHead->dataType = dateType;
	pDqHead->operateMode = operateMode;
	pDqHead->num = recordNum;
	pDqHead->size = recordSize;
	pDqHead->writePoint = recordNum - 1;
	pDqHead->readPoint = (operateMode == NORMAL_MODE) ? (recordNum - 1) : 0;
	gettime(pDqHead->createDate);
	if (pType != 0 && typeSize != 0 && typeSize < TYPEMAXSIZE)
	{
		memcpy((char*)lpMapAddress + recordNum * (recordSize + RECORDHEADSIZE) + QUEUEHEADSIZE,
			pType,
			typeSize
		);
		pDqHead->typesize = typeSize;
	}
	else
	{
		pDqHead->typesize = 0;
	}
	//ZeroMemory((BYTE*)lpMapAddress + QUEUEHEADSIZE, recordNum * (recordSize + RECORDHEADSIZE));	//初始化数据区
	memset((char*)lpMapAddress + QUEUEHEADSIZE, 0, recordNum * (recordSize + RECORDHEADSIZE));
	RECORD_HEAD* pRecordHead;
	for (int i = 0; i < recordNum; i++)
	{
		pRecordHead = (RECORD_HEAD*)((char*)lpMapAddress + QUEUEHEADSIZE + i * (recordSize + RECORDHEADSIZE));
		pRecordHead->ack = 0;
		pRecordHead->index = i;
	}

	// 5. 同步数据到磁盘（可选）
	if (msync(lpMapAddress, fileSize, MS_SYNC) == -1) {
		std::cerr << "同步到磁盘失败: " << strerror(errno) << std::endl;
	}

	// 6. 解除映射并关闭文件
	munmap(lpMapAddress, fileSize);
	close(fd);

	return true;
}