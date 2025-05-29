
#ifndef __NGX_C_SLOGIC_H__
#define __NGX_C_SLOGIC_H__

#include <sys/socket.h>
#include "ngx_c_socket.h"

#include "CSubscribe.h"

//处理逻辑和通讯的子类
class CLogicSocket : public CSocekt   //继承自父类CScoekt
{
public:
	CLogicSocket();                                                         //构造函数
	virtual ~CLogicSocket();                                                //释放函数
	virtual bool Initialize();                                              //初始化函数

public:
	//各种业务逻辑相关函数都在之类
	bool _HandleRegister(lpngx_connection_t pConn,LPSTRUC_MSG_HEADER pMsgHeader,char *pPkgBody,unsigned short iBodyLength);
	bool _HandleLogIn(lpngx_connection_t pConn,LPSTRUC_MSG_HEADER pMsgHeader,char *pPkgBody,unsigned short iBodyLength);

	//gyb
	bool noop(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgHeader, unsigned short iBodyLength);
	bool HandleReadQ(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgHeader, unsigned short iBodyLength);
	bool HandleWriteQ(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgHeader, unsigned short iBodyLength);
	bool HandleClearQ(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgHeader, unsigned short iBodyLength);
	bool HandleReadB(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgHeader, unsigned short iBodyLength);
	bool HandleWriteB(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgHeader, unsigned short iBodyLength);
	bool HandleReadBString(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgHeader, unsigned short iBodyLength);
	bool HandleWriteBString(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgHeader, unsigned short iBodyLength);
	bool HandleSubscribe(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgHeader, unsigned short iBodyLength);
	bool HandleCreateItem(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgHeader, unsigned short iBodyLength);
	bool HandlePostWait(lpngx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char* pPkgHeader, unsigned short iBodyLength);

	void NotifySubscriber(std::string tagName);
	void NotifyTimerSubscriber(std::string timerName);
	void CancelSubscribe(lpngx_connection_t pConn, const std::list<std::string>& tagList);

public:
	virtual void threadRecvProcFunc(char *pMsgBuf);

//gyb
	CSubscribe m_subscriber;
};

#endif
