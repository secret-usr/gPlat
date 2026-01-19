#ifndef HIGPLAT_LINUX_H_INCLUDED_
#define HIGPLAT_LINUX_H_INCLUDED_

#include <chrono>
#include <mutex>
#include <ctime>

// Error Codes
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

// Constants matching higplat.cpp/qbd.h
#define MAXDQNAMELENTH 40
#define INDEXSIZE     9973
#define MUTEXSIZE	  64

#define SHIFT_MODE		1
#define NORMAL_MODE		0
#define ASCII_TYPE		1
#define BINARY_TYPE		0
#define QUEUEHEADSIZE   sizeof(QUEUE_HEAD)
#define RECORDHEADSIZE  sizeof(RECORD_HEAD)

enum {
    QUEUE_T,
    BOARD_T,
    DATABASE_T
};

#pragma pack( push, enter_dataqueue_linux_h_, 8)

struct QUEUE_HEAD
{
	int  qbdtype;
	int  dataType;			// 1: ASCII, 0: BINARY
	int  operateMode;		// 1: SHIFT, 0: NORMAL
	int  num;				// Record count
	int  size;				// Record size
	int  readPoint;
	int  writePoint;
	char createDate[20];
	int  typesize;
	int  reserved;
};

struct RECORD_HEAD
{
	char createDate[20];
	char remoteIp[16];
	int  ack;
	int  index;
	int  reserve;
};

struct BOARD_INDEX_STRUCT
{
	char  itemname[MAXDQNAMELENTH];
	int    startpos;
	int    itemsize;
	bool   erased;
	struct timespec timestamp;
	int	   typeaddr;
	int	   typesize;
};

struct BOARD_HEAD
{
	int qbdtype;
	int counter;
	int totalsize;
	int typesize;
	int nextpos;
	int nexttypepos;
	int remain;
	int typeremain;
	int indexcount;
	std::mutex mutex_rw;
	std::mutex mutex_rw_tag[MUTEXSIZE];
	BOARD_INDEX_STRUCT index[INDEXSIZE];
};

struct DB_INDEX_STRUCT
{
	char  tablename[MAXDQNAMELENTH];
	int    startpos;
	int    recordsize;
	int    maxcount;
	int    currcount;
	long   mutexaccess;
	bool   erased;
	struct timespec timestamp;
	int	   typeaddr;
	int	   typesize;
};

struct DB_HEAD
{
	int qbdtype;
	int counter;
	int totalsize;
	int typesize;
	int nextpos;
	int nexttypepos;
	int remain;
	int typeremain;
	int indexcount;
	std::mutex mutex_rw;
	std::mutex mutex_rw_tag[MUTEXSIZE];
	DB_INDEX_STRUCT index[INDEXSIZE];
};

#pragma pack( pop, enter_dataqueue_linux_h_ )

// C API Declaration
extern "C" {

	unsigned int GetLastErrorQ();
	
	// Queue Operations
	bool CreateQ(const char* lpFileName, int recordSize, int recordNum, int dateType, int operateMode, void* pType = nullptr, int typeSize = 0);
	bool LoadQ(const char* lpDqName);
	bool UnloadQ(const char* lpDqName);
	bool UnloadAll(void);
	bool FlushQFile(void);
	
	bool ReadHead(const char* lpDqName, void* lpHead);
	bool ReadQ(const char* lpDqName, void* lpRecord, int actSize, char* remoteIp = nullptr);
	bool PopJustRecordFromQueue(const char* lpDqName);
	bool MulReadQ(const char* lpDqName, void* lpRecord, int start, int* count, int* pRecordSize = nullptr);
	bool MulReadQ2(const char* lpDqName, void** lppRecords, int start, int* count, int* pRecordSize);
	
	bool WriteQ(const char* lpDqName, void* lpRecord, int actSize = 0, const char* remoteIp = nullptr);
	bool ClearQ(const char* lpDqName);
	bool SetPtrQ(const char* lpDqName, int readPtr, int writePtr);
	bool PeekQ(const char* lpDqName, void* lpRecord, int actSize);
	bool IsEmptyQ(const char* lpDqName);
	bool IsFullQ(const char* lpDqName);

	// Bulletin Operations
	bool CreateB(const char* lpFileName, int size);
	bool ReadInfoB(const char* lpBulletinName, int* pTotalSize, int* pDataSize, int* pLeftSize, int* pItemNum, int* buffSize, char ppBuff[][24]);
	bool ReadB(const char* lpBulletinName, const char* lpItemName, void* lpItem, int actSize, struct timespec* timestamp = nullptr);
	bool ReadB_String(const char* lpBulletinName, const char* lpItemName, void* lpItem, int actSize, struct timespec* timestamp = nullptr);
	
	bool WriteB(const char* lpBulletinName, const char* lpItemName, void* lpItem, int actSize, void* lpSubItem = nullptr, int actSubSize = 0);
	bool WriteB_String(const char* lpBulletinName, const char* lpItemName, void* lpItem, int actSize, void* lpSubItem = nullptr, int actSubSize = 0);
	bool WriteBOffSet(const char* lpBulletinName, const char* lpItemName, void* lpItem, int actSize, int offSet, int actSubSize);
	
	bool ClearB(const char* lpBoardName);
	bool DeleteItem(const char* lpBoardName, const char* lpItemName);
	
	// DB Operations
	bool CreateTable(const char* lpDBName, const char* lpTableName, int recordSize, int maxCount, void* pType = nullptr, int typeSize = 0);
	bool ReadHeadDB(const char* lpDBName, void* lpHead);
	bool ClearTable(const char* lpDBName, const char* lpTableName);
	bool UpdateTable(const char* lpDBName, const char* lpTableName, int rowid, int recordSize, void* lpRecord);
	bool InsertTable(const char* lpDBName, const char* lpTableName, int count, int recordSize, void* lpRecords);
	bool RefreshTable(const char* lpDBName, const char* lpTableName, int count, int recordSize, void* lpRecords);
	bool SelectTable(const char* lpDBName, const char* lpTableName, void** lppRecords, int* pCount, int* pRecordSize, struct timespec* pWriteTime = nullptr);
	bool DeleteTable(const char* lpDBName, const char* lpTableName);
	bool ClearDB(const char* lpDbName);

}

#endif // HIGPLAT_LINUX_H_INCLUDED_
