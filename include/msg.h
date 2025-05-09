#ifndef MSG_H_
#define MSG_H_

#include <OAIDL.H>

#define MAXMSGLEN 2048   //必须与dq.h中的TYPEMAXSIZE一致

enum MSGID
{
	SUCCEED=1,
	FAIL,
	CONNECT,
	RECONNECT,
	DISCONNECT,
	OPEN,
	OPENQ,        //OPENB
	CLOSEQ,       //CLOSEB
	CLEARQ,
	ISEMPTYQ,
	ISFULLQ,
	READQ,
	PEEKQ,
	WRITEQ,
	READB,
	READBSTRING,
	WRITEB,
	QDATA,
	READHEAD,
	MULREADQ,
	SETPTRQ,
	WATCHDOG,
	SELECTTB,
	CLEARTB,
	INSERTTB,
	REFRESHTB,
	READTYPE,
	CREATEITEM,
	CREATETABLE,
	DELETEITEM,
	DELETETABLE,
	READHEADB,
	READHEADDB,
	ACK,
	POPARECORDQ,	//mark
	WRITEBSTRING,	//mark
	WRITETOL1,		//mark
	SUBSCRIBE,		//mark
	CANCELSUBSCRIBE,//mark
	POST,			//mark
	POSTWAIT,		//mark
	PASSTOSERVER,	//mark 内部使用
	CLEARB,			//mark
	CLEARDB			//mark
};

#pragma pack( push, enter_MSG_H_, 1)

typedef struct {
	int    id;
	TCHAR  qname[32];
	TCHAR  itemname[32];
	int    qbdtype;
	int    datasize;
	int    datatype;
	time_t timestamp;
	int    start;
	int    count;
	int    recsize;
	int    bodysize;
	int    readptr;
	int    writeptr;
	DWORD  error;
	int    subsize;
	int    offset;
	TCHAR  ip[16];
	int    arraysize;	// used for plc array write
	int	   eventid;
	int    eventarg;
} MSGHEAD, *PMSGHEAD;

typedef struct {
	MSGHEAD head;
	BYTE    body[MAXMSGLEN];
} MSGSTRUCT, *PMSGSTRUCT;

//struct BODY_WATCHDOG
//{
//    int      a;
//};

#pragma pack( pop, enter_MSG_H_ )

#endif	/*MSG_H_*/