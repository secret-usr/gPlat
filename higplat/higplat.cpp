#include <cstdio>

void hello() {
	printf("hello world GYB!\n");
}

void hello(int a) {
	printf("%d hello world GYB!\n", a);
}

void hello2() {
	printf("hello world GYB!----------------\n");
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
extern "C" { CreateQ(LPCTSTR  lpFileName,
	int recordSize,
	int recordNum,
	int dateType,
	int operateMode,
	VOID* pType,
	int typeSize)
{
	if (recordSize > MAXMSGLEN || typeSize > TYPEMAXSIZE)
	{
		errorCode = ERROR_PARAMETER_SIZE;
		return FALSE;
	}

	usereason = 0;

	// 检查文件名长度是否超过定义长度
	if (lstrlen(lpFileName) > MAXDQNAMELENTH)
	{
		errorCode = ERROR_FILENAME_TOO_LONG;
		return FALSE;
	}

	// 形成文件路径全名
	TCHAR dqFileName[100];
	wcscpy(dqFileName, dataQuePath);
	wcscat(dqFileName, lpFileName);

	// 先查找同名文件删除之
	HANDLE hFileHand;
	WIN32_FIND_DATA findFileData;
	hFileHand = FindFirstFile(dqFileName, &findFileData);
	if (hFileHand != INVALID_HANDLE_VALUE)
	{
		if (!DeleteFile(dqFileName))
		{
			errorCode = ERROR_FILE_IN_USE;
			FindClose(hFileHand);
			return FALSE;
		}
		else
		{
			FindClose(hFileHand);
		}
	}

	// 创建文件
	HANDLE hFile;
	hFile = CreateFile(dqFileName,
		GENERIC_READ | GENERIC_WRITE,			// open for reading and writing
		FILE_SHARE_READ | FILE_SHARE_WRITE,		// share for reading and writing
		NULL,									// no security 
		CREATE_NEW,								// create if not exist 
		FILE_ATTRIBUTE_NORMAL,					// normal file 
		NULL);									// no attr. template 
	if (hFile == INVALID_HANDLE_VALUE)
	{
		errorCode = ERROR_FILE_CREATE_FAILSURE;
		return FALSE;
	}

	// 计算文件大小
	int fileSize;
	fileSize = recordNum * (recordSize + RECORDHEADSIZE) + QUEUEHEADSIZE + TYPEMAXSIZE;

	// 将文件扩展成指定大小
	HANDLE hMapFile;
	hMapFile = CreateFileMapping(hFile,						// Current file handle. 
		NULL,                           // Default security. 
		PAGE_READWRITE,                 // Read/write permission. 
		0,                              // Size of file.
		fileSize,                       // Size of file. 
		lpFileName);					// Name of mapping object. 
	if (hMapFile == NULL)
	{
		errorCode = ERROR_CREATE_FILEMAPPINGOBJECT;
		CloseHandle(hFile);
		return FALSE;
	}

	// 写文件头，初始化队列头和数据区
	LPVOID lpMapAddress;
	lpMapAddress = MapViewOfFile(hMapFile,				// Handle to mapping object. 
		FILE_MAP_ALL_ACCESS,	// Read/write permission. 
		0,						// offset where mapping is to begin. 
		0,						// offset where mapping is to begin.
		0);						// Map entire file. 
	if (lpMapAddress == NULL)
	{
		errorCode = ERROR_MAPVIEWOFFILE;
		CloseHandle(hMapFile);
		CloseHandle(hFile);
		return FALSE;
	}
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
		CopyMemory((BYTE*)lpMapAddress + recordNum * (recordSize + RECORDHEADSIZE) + QUEUEHEADSIZE,
			pType,
			typeSize
		);
		pDqHead->typesize = typeSize;
	}
	else
	{
		pDqHead->typesize = 0;
	}
	ZeroMemory((BYTE*)lpMapAddress + QUEUEHEADSIZE, recordNum * (recordSize + RECORDHEADSIZE));	//初始化数据区
	RECORD_HEAD* pRecordHead;
	for (int i = 0; i < recordNum; i++)
	{
		pRecordHead = (RECORD_HEAD*)((BYTE*)lpMapAddress + QUEUEHEADSIZE + i * (recordSize + RECORDHEADSIZE));
		pRecordHead->ack = 0;
		pRecordHead->index = i;
	}

	UnmapViewOfFile(lpMapAddress);
	CloseHandle(hMapFile);
	CloseHandle(hFile);

	return TRUE;
}
}