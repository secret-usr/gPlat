#if !defined(HIGPLAT_H_INCLUDED_)
#define HIGPLAT_H_INCLUDED_

//由于time_t类型在VS.net中默认是64位，VC是32位，所以必须在VS中定义预处理器_USE_32BIT_TIME_T，以便LIB文件可在VC和VS中通用
#include <chrono>
#include <mutex>

#define DllImport   __declspec( dllimport )

#define ERROR_DQFILE_NOT_FOUND			0x20000000L
#define ERROR_DQ_NOT_OPEN				0x20000001L
#define ERROR_DQ_EMPTY					0x20000002L
#define ERROR_DQ_FULL					0x20000003L
#define ERROR_FILENAME_TOO_LONG			0x20000004L
#define ERROR_FILE_IN_USE				0x20000005L
#define ERROR_FILE_CREATE_FAILSURE		0x20000006L
#define ERROR_FILE_OPEN_FAILSURE		0x20000007L
#define ERROR_CREATE_FILEMAPPINGOBJECT	0x20000008L
#define ERROR_OPEN_FILEMAPPINGOBJECT	0x20000009L
#define ERROR_MAPVIEWOFFILE				0x2000000AL
#define ERROR_CREATE_MUTEX				0x2000000BL
#define ERROR_OPEN_MUTEX				0x2000000CL
#define ERROR_RECORDSIZE				0x2000000DL
#define ERROR_STARTPOSITION				0x2000000EL
#define ERROR_RECORD_ALREAD_EXIST		0x2000000FL
#define ERROR_TABLE_OVERFLOW			0x20000010L
#define ERROR_RECORD_NOT_EXIST			0x20000011L
#define ERROR_OPERATE_PROHIBIT			0x20000012L
#define ERROR_ALREADY_OPEN				0x20000013L
#define ERROR_ALREADY_CLOSE				0x20000014L
#define ERROR_ALREADY_LOAD				0x20000015L
#define ERROR_ALREADY_UNLOAD			0x20000016L
#define ERROR_NO_SPACE			        0x20000017L
#define ERROR_TABLE_NOT_EXIST			0x20000018L
#define ERROR_TABLE_ALREADY_EXIST		0x20000019L
#define ERROR_TABLE_ROWID				0x2000001AL
#define ERROR_ITEM_NOT_EXIST			0x2000001BL
#define ERROR_ITEM_ALREADY_EXIST		0x2000001CL
#define ERROR_ITEM_OVERFLOW				0x2000001DL
#define ERROR_SOCKET_NOT_CONNECTED      0x2000001EL
#define ERROR_MSGSIZE			        0x2000001FL
#define ERROR_BUFFER_SIZE		        0x20000020L
#define ERROR_PARAMETER_SIZE	        0x20000021L
#define CODE_QEMPTY						0x20000022L
#define CODE_QFULL						0x20000023L
#define STRING_TOO_LONG					0x20000024L
#define BUFFER_TOO_SMALL				0x20000025L
#define ERROR_SEND_PIPE_MSG				0x20000026L

#define MAXDQNAMELENTH 40

#define INDEXSIZE     9973	// 必须为质数，必须和dq.h中的定义一致

#define MUTEXSIZE	  64	// 一个BOARD或DB中读写锁的数量，必须是2的n次幂，必须和dq.h中的定义一致

#pragma pack( push, enter_dataqueue_h_, 8)

struct QUEUE_HEAD
{
	int  qbdtype;
	int  dataType;			// 数据队列的类型，1为ASCII型；0为BINARY型
	int  operateMode;		// 1为移位队列，不判断溢出；0为通用队列
	int  num;				// 记录数
	int  size;				// 记录大小
	int  readPoint;			// 读指针
	int  writePoint;		// 写指针
	char createDate[20];	// 创建日期
	int  typesize;			// 类型序列化长度
	int  reserved;
};

struct RECORD_HEAD
{
	char createDate[20];
	char remoteIp[16];
	int  ack;				// 确认标志 0未确认1已确认
	int  index;				// 位置索引（0开始）
	int  reserve;			// 预留
};

//clock_gettime(CLOCK_REALTIME, &ts);
//printf("秒: %ld, 纳秒: %ld\n", ts.tv_sec, ts.tv_nsec);
struct BOARD_INDEX_STRUCT
{
	char  itemname[MAXDQNAMELENTH];
	int    startpos;		// reference to the beginning of date part.
	int    itemsize;
	bool   erased;			// 表删除标志
	timespec timestamp;		// write time
	int	   typeaddr;		// 类型起始地址
	int	   typesize;		// 类型序列化长度
};

struct BOARD_HEAD
{
	int qbdtype;
	int counter;
	int totalsize;		// BOARD_HEAD和后面数据区大小的和，不包括最后面的类型区
	int typesize;		// 最后面的类型区的大小	mark
	int nextpos;		// reference to the beginning of unused date part.
	int nexttypepos;	// reference to the beginning of unused type part. mark
	int remain;
	int typeremain;		// 类型区剩余大小 mark
	int indexcount;
	std::mutex mutex_rw;
	std::mutex mutex_rw_tag[MUTEXSIZE];
	BOARD_INDEX_STRUCT index[INDEXSIZE];
};

struct DB_INDEX_STRUCT
{
	char  tablename[MAXDQNAMELENTH];
	int    startpos;		// reference to the beginning of data.
	int    recordsize;		// 记录大小
	int    maxcount;		// 最大记录数
	int    currcount;		// 当前记录数
	long   mutexaccess;     // 控制互斥访问的变量 //mark 未使用
	bool   erased;			// 表删除标志
	timespec timestamp;	// last write time
	int	   typeaddr;		// 类型起始地址 mark
	int	   typesize;		// 类型序列化长度
};

struct DB_HEAD
{
	int qbdtype;
	int counter;
	int totalsize;
	int typesize;		// 最后面的类型区的大小	mark
	int nextpos;		// reference to the beginning of unused data part.
	int nexttypepos;	// reference to the beginning of unused type part. mark
	int remain;
	int typeremain;		// 类型区剩余大小 mark
	int indexcount;
	std::mutex mutex_rw;
	std::mutex mutex_rw_tag[MUTEXSIZE];	//mark 尚未实现
	DB_INDEX_STRUCT index[INDEXSIZE];
};

#pragma pack( pop, enter_dataqueue_h_ )

#define SHIFT_MODE		1
#define NORMAL_MODE		0
#define ASCII_TYPE		1
#define BINARY_TYPE		0
#define QUEUEHEADSIZE   sizeof(QUEUE_HEAD)
#define RECORDHEADSIZE  sizeof(RECORD_HEAD)

//DllImport BOOL __cdecl GetQueuePath(LPTSTR lpPath, size_t count);
//DllImport BOOL __cdecl CreateB(LPCTSTR  lpFileName, int size);
//DllImport BOOL __cdecl CreateItem( LPCTSTR lpBoardName, LPCTSTR lpItemName, int itemSize, VOID * pType=0, int typeSize=0 );
//DllImport BOOL __cdecl CreateItem( HANDLE hServer, LPCTSTR lpBoardName, LPCTSTR lpItemName, int itemSize, VOID * pType, int typeFileSize, DWORD * error );
//DllImport BOOL __cdecl DeleteItem( LPCTSTR lpBoardName, LPCTSTR lpItemName );
//DllImport BOOL __cdecl DeleteItem( HANDLE hServer, LPCTSTR lpBoardName, LPCTSTR lpItemName, DWORD * error );
extern "C" bool CreateQ(const char* lpFileName, int recordSize, int recordNum, int dateType, int operateMode, void* pType = 0, int typeSize = 0);
//DllImport BOOL __cdecl LoadQ( LPCTSTR  lpDqName );
//DllImport BOOL __cdecl UnloadQ(void);
//DllImport BOOL __cdecl UnloadQ(LPCTSTR  lpDqName);
//DllImport BOOL __cdecl FlushQFile(void);
//DllImport BOOL __cdecl OpenQbd( LPCTSTR  lpQbdName, int * pType );
//DllImport BOOL __cdecl OpenQbd( HANDLE hServer, LPCTSTR  lpQbdName, int * pType, DWORD * error );
//DllImport BOOL __cdecl OpenQ( LPCTSTR  lpDqName );
//DllImport BOOL __cdecl CloseQ( LPCTSTR  lpDqName );
//DllImport BOOL __cdecl CloseQ(void);
//DllImport BOOL __cdecl ReadHead( LPCTSTR  lpDqName, VOID  *lpHead );
//DllImport BOOL __cdecl ReadQ( LPCTSTR  lpDqName, VOID  *lpRecord, int actSize, LPTSTR remoteIp=0 );
//DllImport BOOL __cdecl MulReadQ( LPCTSTR lpDqName, VOID *lpRecord, int start, int *count, int *pRecordSize=0 );
//DllImport BOOL __cdecl MulReadQ( LPCTSTR lpDqName, VOID  **lppRecords, int start, int *count, int *pRecordSize );
//DllImport BOOL __cdecl WriteQ( LPCTSTR  lpDqName, VOID  *lpRecord, int actSize=0, LPCTSTR remoteIp=0 );
//DllImport BOOL __cdecl Acknowledge( LPCTSTR  lpDqName, int index );
//DllImport BOOL __cdecl Acknowledge( HANDLE hServer, LPCTSTR  lpDqName, int index, DWORD * error );
//DllImport BOOL __cdecl SetPtrQ( LPCTSTR  lpDqName, int readPtr, int writePtr );
//DllImport BOOL __cdecl ClearQ( LPCTSTR  lpDqName );
//DllImport BOOL __cdecl PeekQ( LPCTSTR  lpDqName, VOID  *lpRecord, int actSize );
//DllImport BOOL __cdecl IsEmptyQ( LPCTSTR  lpDqName );
//DllImport BOOL __cdecl IsFullQ( LPCTSTR  lpDqName );
//DllImport BOOL __cdecl LoadB( LPCTSTR  lpBoardName );
//DllImport BOOL __cdecl UnloadB( LPCTSTR  lpBoardName );
//DllImport BOOL __cdecl UnloadB();
//DllImport BOOL __cdecl OpenB( LPCTSTR  lpBoardName );
//DllImport BOOL __cdecl CloseB( LPCTSTR  lpBoardName );
//DllImport BOOL __cdecl CloseB(void);
//DllImport BOOL __cdecl ClearB(LPCTSTR  lpDqName);
//DllImport BOOL __cdecl ClearDB(LPCTSTR  lpDqName);
//DllImport BOOL __cdecl ClearB(HANDLE hServer, LPCTSTR  lpDqName, DWORD* error);
//DllImport BOOL __cdecl ClearDB(HANDLE hServer, LPCTSTR  lpDbName, DWORD* error);
//DllImport BOOL __cdecl LockB( LPCTSTR lpBulletinName );
//DllImport BOOL __cdecl UnlockB( LPCTSTR lpBulletinName );
//DllImport BOOL __cdecl ReadInfoB( LPCTSTR lpBulletinName, int* pTotalSize, int* pDataSize, int* pLeftSize, int* pItemNumint, int* buffSize, TCHAR ppBuff[][24]);
//DllImport BOOL __cdecl ReadB( LPCTSTR lpBoardName, LPCTSTR lpItemName, VOID  *lpItem, int actSize, time_t *timestamp=0 );
//DllImport BOOL __cdecl ReadB_String(LPCTSTR lpBulletinName, LPCTSTR lpItemName, VOID  *lpItem, int actSize, time_t *timestamp=0);
//DllImport BOOL __cdecl ReadB_String(HANDLE hServer, LPCTSTR lpBulletinName, LPCTSTR lpItemName, VOID  *lpItem, int actSize, DWORD * error, time_t *timestamp=0);
//DllImport BOOL __cdecl ReadB( VOID * pBulletin, LPCTSTR lpItemName, VOID  *lpItem, int actSize, time_t *timestamp=0 );
//DllImport BOOL __cdecl WriteB( LPCTSTR lpBulletinName, LPCTSTR lpItemName, VOID  *lpItem, int actSize, VOID  *lpSubItem=0, int actSubSize=0 );
//DllImport BOOL __cdecl WriteB_String(LPCTSTR lpBulletinName, LPCTSTR lpItemName, VOID  *lpItem, int actSize, VOID  *lpSubItem = 0, int actSubSize = 0);
//DllImport BOOL __cdecl WriteB_String(HANDLE hServer, LPCTSTR lpBulletinName, LPCTSTR lpItemName, VOID  *lpItem, int actSize, DWORD * error);
//DllImport BOOL __cdecl WriteB_ReadBack( LPCTSTR lpBulletinName, LPCTSTR lpItemName, VOID  *lpItem, int actSize, VOID  *lpSubItem=0, int actSubSize=0 );
//DllImport BOOL __cdecl WriteBOffSet( LPCTSTR lpBulletinName, LPCTSTR lpItemName, VOID  *lpItem, int actSize, int offSet, int actSubSize );
//DllImport BOOL __cdecl WriteBOffSet_ReadBack( LPCTSTR lpBulletinName, LPCTSTR lpItemName, VOID  *lpItem, int actSize, int offSet, int actSubSize );
//DllImport BOOL __cdecl GetCopyB( LPCTSTR lpBulletinName, BYTE * pBulletin, unsigned int * pCounter );
//DllImport BOOL __cdecl RefreshB( LPCTSTR lpBulletinName, BYTE* pBulletin, unsigned int * pCounter=0 );
//DllImport DWORD __cdecl GetLastErrorQ( VOID );
//DllImport BOOL __cdecl ReadLastWrTimeQ( LPCTSTR  lpDqName, BYTE* buf );
//DllImport BOOL __cdecl ReConnectServer(HANDLE hServer);
//DllImport HANDLE __cdecl ConnectServer( LPCTSTR  lpServerName );
//DllImport VOID __cdecl DisconnServer( HANDLE hServer );
//DllImport BOOL __cdecl OpenQ( HANDLE hServer, LPCTSTR  lpDqName, DWORD * error );
//DllImport BOOL __cdecl OpenQ( LPCTSTR  lpServerName, LPCTSTR  lpDqName, DWORD * error );
//DllImport BOOL __cdecl OpenB( LPCTSTR  lpServerName, LPCTSTR  lpBulletinName, DWORD * error );
//DllImport BOOL __cdecl CloseQ( HANDLE hServer, LPCTSTR  lpDqName, DWORD * error );
//DllImport BOOL __cdecl CloseQ( LPCTSTR  lpServerName, LPCTSTR  lpDqName, DWORD * error );
//DllImport BOOL __cdecl CloseB( LPCTSTR  lpServerName, LPCTSTR  lpBulletinName, DWORD * error );
//DllImport BOOL __cdecl ClearQ( HANDLE hServer, LPCTSTR  lpDqName, DWORD * error );
//DllImport BOOL __cdecl ClearQ( LPCTSTR  lpServerName, LPCTSTR  lpDqName, DWORD * error );
//DllImport BOOL __cdecl IsEmptyQ( HANDLE hServer, LPCTSTR  lpDqName, DWORD * error );
//DllImport BOOL __cdecl IsEmptyQ( LPCTSTR  lpServerName, LPCTSTR  lpDqName, DWORD * error );
//DllImport BOOL __cdecl IsFullQ( HANDLE hServer, LPCTSTR  lpDqName, DWORD * error );
//DllImport BOOL __cdecl IsFullQ( LPCTSTR  lpServerName, LPCTSTR  lpDqName, DWORD * error );
//DllImport BOOL __cdecl SetPtrQ( HANDLE hServer, LPCTSTR  lpDqName, int readPtr, int writePtr, DWORD * error );
//DllImport BOOL __cdecl SetPtrQ( LPCTSTR lpMachineName, LPCTSTR  lpDqName, int readPtr, int writePtr, DWORD * error );
//DllImport BOOL __cdecl ReadHead( HANDLE hServer, LPCTSTR  lpDqName, VOID  *lpHead, DWORD * error );
//DllImport BOOL __cdecl ReadHead( LPCTSTR lpMachineName, LPCTSTR  lpDqName, VOID  *lpHead, DWORD * error );
//DllImport BOOL __cdecl ReadQ( HANDLE hServer, LPCTSTR  lpDqName, VOID  *lpRecord, int actSize, DWORD * error );
//DllImport BOOL __cdecl ReadQ( LPCTSTR  lpServerName, LPCTSTR  lpDqName, VOID  *lpRecord, int actSize, DWORD * error );
//DllImport BOOL __cdecl PopJustRecordFromQueue(LPCTSTR lpDqName);
//DllImport BOOL __cdecl PopJustRecordFromQueue(HANDLE hServer, LPCTSTR lpDqName, DWORD * error);
//DllImport BOOL __cdecl MulReadQ( HANDLE hServer, LPCTSTR lpDqName, VOID *lpRecord, int start, int *count, int *pRecordSize, DWORD * error );
//DllImport BOOL __cdecl MulReadQ( HANDLE hServer, LPCTSTR lpDqName, VOID  **lppRecords, int start, int *count, int *pRecordSize, DWORD * error );
//DllImport BOOL __cdecl MulReadQ( LPCTSTR  lpServerName, LPCTSTR lpDqName, VOID *lpRecord, int start, int *count, int *pRecordSize, DWORD * error );
//DllImport BOOL __cdecl PeekQ( HANDLE hServer, LPCTSTR  lpDqName, VOID  *lpRecord, int actSize, DWORD * error );
//DllImport BOOL __cdecl PeekQ( LPCTSTR  lpServerName, LPCTSTR  lpDqName, VOID  *lpRecord, int actSize, DWORD * error );
//DllImport BOOL __cdecl WriteQ( HANDLE hServer, LPCTSTR  lpDqName, VOID  *lpRecord, int actSize, DWORD * error );
//DllImport BOOL __cdecl WriteQ( LPCTSTR  lpServerName, LPCTSTR  lpDqName, VOID  *lpRecord, int actSize, DWORD * error );
//DllImport BOOL __cdecl ReadHeadB( LPCTSTR  lpBulletinName, VOID  *lpHead );
//DllImport BOOL __cdecl ReadHeadB( HANDLE hServer, LPCTSTR  lpBulletinName, VOID  *lpHead, DWORD * error );
//DllImport BOOL __cdecl ReadB( HANDLE hServer, LPCTSTR lpBulletinName, LPCTSTR lpItemName, VOID  *lpItem, int actSize, DWORD * error, time_t *timestamp=0 );
//DllImport BOOL __cdecl ReadB( LPCTSTR  lpServerName, LPCTSTR lpBulletinName, LPCTSTR lpItemName, VOID  *lpItem, int actSize, DWORD * error, time_t *timestamp=0 );
//DllImport BOOL __cdecl WriteB( HANDLE hServer, LPCTSTR lpBulletinName, LPCTSTR lpItemName, VOID  *lpItem, int actSize, int offset, int subSize, DWORD * error );
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, std::string& str, DWORD * error);
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, char*, int buffSize, DWORD * error);	//L1不支持UNICODE字符读写，所以只能是char*
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, byte value[], int arraySize, DWORD * error);
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, std::string& str, int strLength, DWORD * error);
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, bool& value, DWORD * error);
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, short& value, DWORD * error);
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, unsigned short& value, DWORD * error);
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, int& value, DWORD * error);
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, unsigned int& value, DWORD * error);
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, long& value, DWORD * error);
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, unsigned long& value, DWORD * error);
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, float& value, DWORD * error);
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, short value[], int arraySize, DWORD * error);
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, unsigned short value[], int arraySize, DWORD * error);
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, int value[], int arraySize, DWORD * error);
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, unsigned int value[], int arraySize, DWORD * error);
//DllImport BOOL __cdecl ReadL1Tag(HANDLE hServer, LPCTSTR lpItemName, float value[], int arraySize, DWORD * error);
//DllImport BOOL __cdecl WriteToL1(HANDLE hServer, LPCTSTR lpItemName, std::string& str, DWORD * error);
//DllImport BOOL __cdecl WriteToL1(HANDLE hServer, LPCTSTR lpItemName, LPCTSTR str, DWORD * error);
//DllImport BOOL __cdecl WriteToL1ByteArray(HANDLE hServer, LPCTSTR lpItemName, std::string& str, DWORD * error);
//DllImport BOOL __cdecl WriteToL1ByteArray(HANDLE hServer, LPCTSTR lpItemName, LPCTSTR str, DWORD * error);
//DllImport BOOL __cdecl WriteToL1(HANDLE hServer, LPCTSTR lpItemName, bool value, DWORD * error);
//DllImport BOOL __cdecl WriteToL1(HANDLE hServer, LPCTSTR lpItemName, short value, DWORD * error);
//DllImport BOOL __cdecl WriteToL1(HANDLE hServer, LPCTSTR lpItemName, unsigned short value, DWORD * error);
//DllImport BOOL __cdecl WriteToL1(HANDLE hServer, LPCTSTR lpItemName, int value, DWORD * error);
//DllImport BOOL __cdecl WriteToL1(HANDLE hServer, LPCTSTR lpItemName, unsigned int value, DWORD * error);
//DllImport BOOL __cdecl WriteToL1(HANDLE hServer, LPCTSTR lpItemName, long value, DWORD * error);
//DllImport BOOL __cdecl WriteToL1(HANDLE hServer, LPCTSTR lpItemName, unsigned long value, DWORD * error);
//DllImport BOOL __cdecl WriteToL1(HANDLE hServer, LPCTSTR lpItemName, float value, DWORD * error);
//DllImport BOOL __cdecl WriteToL1(HANDLE hServer, LPCTSTR lpItemName, short value[], int arraySize, DWORD * error);
//DllImport BOOL __cdecl WriteToL1(HANDLE hServer, LPCTSTR lpItemName, unsigned short value[], int arraySize, DWORD * error);
//DllImport BOOL __cdecl WriteToL1(HANDLE hServer, LPCTSTR lpItemName, int value[], int arraySize, DWORD * error);
//DllImport BOOL __cdecl WriteToL1(HANDLE hServer, LPCTSTR lpItemName, unsigned value[], int arraySize, DWORD * error);
//DllImport BOOL __cdecl WriteToL1(HANDLE hServer, LPCTSTR lpItemName, float value[], int arraySize, DWORD * error);
//DllImport BOOL __cdecl WriteToL1(HANDLE hServer, LPCTSTR lpItemName, VOID  *lpItem, int actSize, SHORT dataType, int arraySize, DWORD * error);
//DllImport BOOL __cdecl WriteB( LPCTSTR  lpServerName, LPCTSTR lpBulletinName, LPCTSTR lpItemName, VOID  *lpItem, int actSize, int offset, int subSize, DWORD * error );
//DllImport BOOL __cdecl CreateDB(LPCTSTR  lpFileName, int size);
//DllImport BOOL __cdecl LoadDB( LPCTSTR  lpDBName );
//DllImport BOOL __cdecl UnloadDB( LPCTSTR  lpDBName );
//DllImport BOOL __cdecl OpenDB( LPCTSTR  lpDBName );
//DllImport BOOL __cdecl CloseDB( LPCTSTR  lpDBName );
//DllImport BOOL __cdecl ReadHeadDB( LPCTSTR  lpBulletinName, VOID  *lpHead );
//DllImport BOOL __cdecl ReadHeadDB( HANDLE hServer, LPCTSTR  lpDBName, VOID  *lpHead, DWORD * error );
//DllImport BOOL __cdecl CreateTable( LPCTSTR lpDBName, LPCTSTR lpTableName, int recordSize, int maxCount, VOID * pType=0, int typeSize=0 );
//DllImport BOOL __cdecl CreateTable( HANDLE hServer, LPCTSTR lpDBName, LPCTSTR lpTableName, int recordSize, int maxCount, VOID * pType, int typeFileSize, DWORD * error );
//DllImport BOOL __cdecl LockTable( LPCTSTR lpDBName, LPCTSTR lpTableName );
//DllImport BOOL __cdecl UnlockTable( LPCTSTR lpDBName, LPCTSTR lpTableName );
//DllImport BOOL __cdecl ClearTable( LPCTSTR lpDBName, LPCTSTR lpTableName );
//DllImport BOOL __cdecl ClearTable( HANDLE hServer, LPCTSTR lpDBName, LPCTSTR lpTableName, DWORD * error );
//DllImport BOOL __cdecl ClearTable( LPCTSTR  lpServerName, LPCTSTR lpDBName, LPCTSTR lpTableName, DWORD * error );
//DllImport BOOL __cdecl InsertTable( LPCTSTR lpDBName, LPCTSTR lpTableName, int count, int recordSize, VOID  *lpRecords );
//DllImport BOOL __cdecl InsertTable( HANDLE hServer, LPCTSTR lpDBName, LPCTSTR lpTableName, int count, int recordSize, VOID  *lpRecords, DWORD * error );
//DllImport BOOL __cdecl InsertTable( LPCTSTR  lpServerName, LPCTSTR lpDBName, LPCTSTR lpTableName, int count, int recordSize, VOID  *lpRecords, DWORD * error );
//DllImport BOOL __cdecl SelectTable( LPCTSTR lpDBName, LPCTSTR lpTableName, VOID  **lppRecords, int *pCount, int *pRecordSize, time_t *pWriteTime=0 );
//DllImport BOOL __cdecl SelectTable( HANDLE hServer, LPCTSTR lpDBName, LPCTSTR lpTableName, VOID  **lppRecords, int *pCount, int *pRecordSize, DWORD * error, time_t *pWriteTime=0 );
//DllImport BOOL __cdecl SelectTable( LPCTSTR  lpServerName, LPCTSTR lpDBName, LPCTSTR lpTableName, VOID  **lppRecords, int *pCount, int *pRecordSize, DWORD * error, time_t *pWriteTime=0 );
//DllImport BOOL __cdecl DeleteTable( LPCTSTR lpDBName, LPCTSTR lpTableName );
//DllImport BOOL __cdecl DeleteTable( HANDLE hServer, LPCTSTR lpDBName, LPCTSTR lpTableName, DWORD * error );
//DllImport BOOL __cdecl RefreshTable(HANDLE hServer, LPCTSTR lpDBName, LPCTSTR lpTableName, int count, int recordSize, VOID  *lpRecords, DWORD * error);
//DllImport BOOL __cdecl RefreshTable(LPCTSTR lpDBName, LPCTSTR lpTableName, int count, int recordSize, VOID  *lpRecords);
//DllImport BOOL __cdecl ReadType( LPCTSTR  lpDqName, LPCTSTR lpItemName, VOID  *inBuff, int buffSize, int *pTypeSize );
//DllImport BOOL __cdecl ReadType( HANDLE hServer, LPCTSTR  lpDqName, LPCTSTR lpItemName, VOID  *inBuff, int buffSize, int *pTypeSize, DWORD * error );
//DllImport BOOL __cdecl ReadType( LPCTSTR  lpServerName, LPCTSTR  lpDqName, LPCTSTR lpItemName, VOID  *inBuff, int buffSize, int *pTypeSize, DWORD * error );
//DllImport BOOL __cdecl CancelSubscribe(HANDLE hServer, DWORD * error);
//DllImport BOOL __cdecl Subscribe(HANDLE hServer, LPCTSTR  lpTagName, DWORD * error);
//DllImport BOOL __cdecl Subscribe(HANDLE hServer, LPCTSTR lpTagName, VOID** lppType, int& typeSize, DWORD* error);
//DllImport BOOL __cdecl SubscribeForDelayPost(HANDLE hServer, LPCTSTR lpTagName, LPCTSTR lpEventName, int delayDueTime, DWORD * error);
//DllImport BOOL __cdecl SubscribeForRisingEdge(HANDLE hServer, LPCTSTR lpTagName, LPCTSTR lpEventName, DWORD * error);
//DllImport BOOL __cdecl SubscribeForFallingEdge(HANDLE hServer, LPCTSTR lpTagName, LPCTSTR lpEventName, DWORD * error);
//DllImport BOOL __cdecl SubscribeForRisingEdgeDelay(HANDLE hServer, LPCTSTR lpTagName, LPCTSTR lpEventName, int delayDueTime, DWORD * error);
//DllImport BOOL __cdecl SubscribeForFallingEdgeDelay(HANDLE hServer, LPCTSTR lpTagName, LPCTSTR lpEventName, int delayDueTime, DWORD * error);
//DllImport DWORD __cdecl WaitForPostedData(HANDLE hServer, std::wstring& tagName, VOID  **lppData, int& dataSize, DWORD  dwMilliseconds, DWORD * error);

#endif