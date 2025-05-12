#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/msg.h"
#include "qbd.h"

namespace fs = std::filesystem;

thread_local  unsigned int errorCode = 0;
char dataQuePath[100];
struct TABLE_MSG table[TABLESIZE];
int tabCounter = 0;
std::mutex mutex_rw;

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
Function: inserttab

Summary:  insert a record into global hash table.

Args:     const struct TABLE_MSG &tabmsg
record to be inserted

Returns:  bool
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
bool inserttab(const struct TABLE_MSG& tabmsg)
{
	int loc, c;
	loc = hash1(tabmsg.dqname);
	c = hash2(tabmsg.dqname);

	//WaitForSingleObject(hTabMutex, INFINITE);
	//std::unique_lock<std::mutex> lock(mutex_rw);

	while (strcmp(table[loc].dqname, "\0") && !table[loc].erased)
	{
		loc = (loc + c) % TABLESIZE;
	}

	if (table[loc].erased)
	{
		strcpy(table[loc].dqname, tabmsg.dqname);
		table[loc].hFile = tabmsg.hFile;
		table[loc].lpMapAddress = tabmsg.lpMapAddress;
		table[loc].hMapFile = tabmsg.hMapFile;
		table[loc].hMutex = tabmsg.hMutex;
		table[loc].pmutex_rw = tabmsg.pmutex_rw;
		table[loc].erased = false;
		table[loc].count++;
		//ReleaseMutex(hTabMutex);
		return true;
	}
	else
	{
		if (tabCounter == TABLESIZE - 1)
		{
			errorCode = ERROR_TABLE_OVERFLOW;
			//ReleaseMutex(hTabMutex);
			return false;
		}
		else
		{
			tabCounter++;
			strcpy(table[loc].dqname, tabmsg.dqname);
			table[loc].hFile = tabmsg.hFile;
			table[loc].lpMapAddress = tabmsg.lpMapAddress;
			table[loc].hMapFile = tabmsg.hMapFile;
			table[loc].hMutex = tabmsg.hMutex;
			table[loc].pmutex_rw = tabmsg.pmutex_rw;
			table[loc].erased = false;
			table[loc].count++;
			//printf("%d count is %d\n", loc, table[loc].count);
			//ReleaseMutex(hTabMutex);
			return true;
		}
	}
}


/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
Function: fetchtab

Summary:  fetch a record from global hash table.

Args:     const char *dqname
queue name
struct TABLE_MSG &tabmsg
record to be fetched

Returns:  bool
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
bool fetchtab(const char* dqname, struct TABLE_MSG& tabmsg)
{
	int loc, c;
	loc = hash1(dqname);
	c = hash2(dqname);

	////WaitForSingleObject(hTabMutex, INFINITE);
	std::lock_guard<std::mutex> lock(mutex_rw);

	while (strcmp(table[loc].dqname, "\0") && strcmp(table[loc].dqname, dqname))
	{
		loc = (loc + c) % TABLESIZE;
	}

	if (!strcmp(table[loc].dqname, "\0") || table[loc].erased)
	{
		errorCode = ERROR_RECORD_NOT_EXIST;
		//ReleaseMutex(hTabMutex);
		return false;
	}

	strcpy(tabmsg.dqname, table[loc].dqname);
	tabmsg.hFile = table[loc].hFile;
	tabmsg.lpMapAddress = table[loc].lpMapAddress;
	tabmsg.hMapFile = table[loc].hMapFile;
	tabmsg.hMutex = table[loc].hMutex;
	tabmsg.pmutex_rw = table[loc].pmutex_rw;
	//ReleaseMutex(hTabMutex);
	return true;
}


/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
Function: fetchtab1

Summary:  Fetch a record from global hash table, The difference between
fetchtab and fetchtab1 is, when using fetchtab1, if the record
already existed in hash table, its count will be increased by 1.

Args:     const char *dqname
queue name
struct TABLE_MSG &tabmsg
record to be fetched

Returns:  bool
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
bool fetchtab1(const char* dqname, struct TABLE_MSG& tabmsg)
{
	int loc, c;
	loc = hash1(dqname);
	c = hash2(dqname);

	//WaitForSingleObject(hTabMutex, INFINITE);
	//std::lock_guard<std::mutex> lock(mutex_rw);

	while (strcmp(table[loc].dqname, "\0") && strcmp(table[loc].dqname, dqname))
	{
		loc = (loc + c) % TABLESIZE;
	}

	if (!strcmp(table[loc].dqname, "\0") || table[loc].erased)
	{
		errorCode = ERROR_RECORD_NOT_EXIST;
		//ReleaseMutex(hTabMutex);
		return false;
	}

	strcpy(tabmsg.dqname, table[loc].dqname);
	tabmsg.hFile = table[loc].hFile;
	tabmsg.lpMapAddress = table[loc].lpMapAddress;
	tabmsg.hMapFile = table[loc].hMapFile;
	tabmsg.hMutex = table[loc].hMutex;
	tabmsg.pmutex_rw = table[loc].pmutex_rw;
	table[loc].count++;	//gyb
	//ReleaseMutex(hTabMutex);
	return true;
}


/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
Function: deletetab

Summary:  Delete a record from global hash table, Decrease the record's
count first, the record is really deleted if the count is zero.

Args:     const char *dqname
queue name
struct TABLE_MSG &tabmsg
record to be deleted

Returns:  bool
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
bool deletetab(const char* dqname, struct TABLE_MSG& tabmsg)
{
	int loc, c;
	loc = hash1(dqname);
	c = hash2(dqname);

	//WaitForSingleObject(hTabMutex, INFINITE);
	//std::unique_lock<std::mutex> lock(mutex_rw);

	while (strcmp(table[loc].dqname, "\0") && strcmp(table[loc].dqname, dqname))
	{
		loc = (loc + c) % TABLESIZE;
	}

	if (!strcmp(table[loc].dqname, "\0") || table[loc].erased)
	{
		errorCode = ERROR_RECORD_NOT_EXIST;
		//ReleaseMutex(hTabMutex);
		return false;
	}

	if (--table[loc].count < 1)
	{
		strcpy(table[loc].dqname, "CT0D9GG_~$59");	 //不是必需的，只是确保安全
		table[loc].erased = true;
		table[loc].count = 0;
		tabmsg.hFile = table[loc].hFile;
		tabmsg.lpMapAddress = table[loc].lpMapAddress;
		tabmsg.hMapFile = table[loc].hMapFile;
		tabmsg.hMutex = table[loc].hMutex;
		tabmsg.pmutex_rw = table[loc].pmutex_rw;
		//ReleaseMutex(hTabMutex);
		return true;
	}
	//ReleaseMutex(hTabMutex);
	return false;
}


/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
Function: hash1, hash2

Summary:  Get hash key value.

Args:     const char *s
key string

Returns:  int
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
inline int hash1(const char* s)
{
	int k = 0;
	while (*s)
	{
		k += (int)*s;
		s++;
	}
	return (k % TABLESIZE);
}


inline int hash2(const char* s)
{
	int k = 0, remain;
	while (*s)
	{
		k += (int)*s;
		s++;
	}

	remain = k % (TABLESIZE - 2);
	return remain == 0 ? 1 : remain;
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

Returns:  bool
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
	void* lpMapAddress = mmap(nullptr, fileSize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
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

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
Function: LoadQ

Summary:  Close mutex, file-mapping object and file.

Args:     LPCTSTR  lpDqName

Returns:  bool
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
extern "C" bool LoadQ(const char* lpDqName)
{
	//这里就不锁定了，因为LoadQ肯定在单线程上运行，和OpenQ不一样，另外下面的是fetchtab，函数里也要锁定的，这样会死锁的
	//std::unique_lock<std::mutex> lock(mutex_rw);

	struct TABLE_MSG tabmsg;
	if (fetchtab(lpDqName, tabmsg))
	{
		errorCode = ERROR_ALREADY_LOAD;
		return true;
	}

	// 形成文件全名
	char dqFileName[100];
	strcpy(dqFileName, dataQuePath);
	strcat(dqFileName, lpDqName);

	// 打开文件
	int fd = open(dqFileName, O_CREAT | O_RDWR, 0644);
	if (fd == -1) {
		std::cerr << "打开文件失败: " << strerror(errno) << std::endl;
		return false;
	}

	// 2. 获取文件大小
	struct stat sb;
	if (fstat(fd, &sb) == -1) {
		std::cerr << "获取文件大小失败: " << strerror(errno) << std::endl;
		close(fd);
		return false;
	}
	off_t file_size = sb.st_size;

	// 3. 创建内存映射
	void* lpMapAddress = mmap(nullptr, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (lpMapAddress == MAP_FAILED) {
		std::cerr << "内存映射失败: " << strerror(errno) << std::endl;
		close(fd);
		return false;
	}

	//创建读写锁
	if (*(int*)lpMapAddress == BOARD_T)
	{
		BOARD_HEAD* pHead = (BOARD_HEAD*)lpMapAddress;
		std::mutex* pSharedMutex = new (&pHead->mutex_rw) std::mutex();
		tabmsg.pmutex_rw = pSharedMutex;
		for (int i = 0; i < MUTEXSIZE; i++)
		{
			pSharedMutex = new (&pHead->mutex_rw_tag[i]) std::mutex();
		}
	}
	else if (*(int*)lpMapAddress == DATABASE_T)
	{
		DB_HEAD* pHead = (DB_HEAD*)lpMapAddress;
		std::mutex* pSharedMutex = new (&pHead->mutex_rw) std::mutex();
		tabmsg.pmutex_rw = pSharedMutex;
	}
	else if (*(int*)lpMapAddress == QUEUE_T)
	{
		tabmsg.pmutex_rw = nullptr;
	}

	//将队列名、文件句柄、映射文件对象句柄、互斥量对象句柄保存在哈希表中。
	strcpy(tabmsg.dqname, lpDqName);
	tabmsg.hFile = fd;
	tabmsg.hMapFile = 0;				  //linux下不需要
	tabmsg.lpMapAddress = lpMapAddress;   //为将内存文件映射数据定期写回文件而增加
	//tabmsg.hMutex = hMutex;
	pthread_mutex_init(&tabmsg.hMutex, NULL);
	if (inserttab(tabmsg))
	{
		return true;
	}
	//CloseHandle(hMapFile);
	close(fd);
	pthread_mutex_destroy(&tabmsg.hMutex);
	return false;
}