#include <chrono>
#include <mutex>

#define MAXDQNAMELENTH 32

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

#define SHIFT_MODE		1
#define NORMAL_MODE		0
#define ASCII_TYPE		1
#define BINARY_TYPE		0
#define QUEUEHEADSIZE   sizeof(QUEUE_HEAD)
#define RECORDHEADSIZE  sizeof(RECORD_HEAD)

enum{
	QUEUE_T,
	BOARD_T,
	DATABASE_T
};
#define MUTEXSIZE	  64	// 一个BOARD或DB中读写锁的数量，必须是2的n次幂 mark，必须和dataqueue.h中的定义一致
#define TABLESIZE     211	// 必须为质数
#define INDEXSIZE     9973	// 必须为质数，必须和dataqueue.h中的定义一致
#define TYPEMAXSIZE   2048  // 数据类型的最大序列化长度   必须与msg.h中的MAXMSGLEN一致	//mark 与QbdServer项目中MyIOCP::HandleSUBSCRIBE里的缓冲区大小有矛盾，似乎没必要那么大
#define TYPEAVGSIZE	  128	// 数据类型的平均序列化长度	mark

#pragma pack( push, enter_dq_h_, 8)

/*
struct TABLE_MSG
{
	char dqname[MAXDQNAMELENTH];
	union
	{
		HANDLE hFile;
		LPVOID lpMapAddress;
	};
	HANDLE hMapFile;
	HANDLE hMutex;
	BOOL erased;
	int count;
};
*/


//为将内存文件映射数据定期写回文件而修改
//struct TABLE_MSG
//{
//	char dqname[MAXDQNAMELENTH];
//	HANDLE hFile;
//	LPVOID lpMapAddress;
//	HANDLE hMapFile;
//	HANDLE hMutex;
//	BOOL erased;
//	int count;
//};

struct TABLE_MSG
{
	char dqname[MAXDQNAMELENTH];
	int  hFile;
	void* lpMapAddress;
	int hMapFile;
	std::mutex hMutex;
	std::mutex * pmutex_rw;
	bool erased;
	int count;
};

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

bool inserttab(const struct TABLE_MSG &tabmsg);
bool fetchtab(const char* dqname, struct TABLE_MSG &tabmsg);
bool fetchtab1(const char* dqname, struct TABLE_MSG &tabmsg);
bool deletetab(const char* dqname, struct TABLE_MSG &tabmsg);
inline int  hash1(const char* s);
inline int  hash2(const char* s);
inline void gettime(const char* timebuf);

#pragma pack( pop, enter_dq_h_ )