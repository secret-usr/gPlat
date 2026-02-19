// 消息处理函数模板
// 用途: 添加新的消息类型处理函数
// 使用: 复制此模板，替换 COMMAND_NAME 为实际命令名

// 1. 在 include/msg.h 添加 MSGID 枚举
/*
enum MSGID {
    // ...
    NEW_COMMAND_NAME = 100,  // 选择未使用的 ID
};
*/

// 2. 在 gplat/ngx_c_slogic.h 声明处理函数
/*
class CLogicSocket : public CSocekt {
    // ...
    void HandleCommandName(
        lpngx_connection_t pConn,
        MSGHEAD* pMsgHeader,
        char* pPkgBody,
        unsigned short bodyLength
    );
};
*/

// 3. 在 gplat/ngx_c_slogic.cxx 实现处理函数
void CLogicSocket::HandleCommandName(
    lpngx_connection_t pConn,
    MSGHEAD* pMsgHeader,
    char* pPkgBody,
    unsigned short bodyLength
) {
    CMemory* p_memory = CMemory::GetInstance();

    // 解析请求数据
    if (bodyLength > 0 && pPkgBody != nullptr) {
        // TODO: 解析 pPkgBody
    }

    // 执行业务逻辑
    // TODO: 调用 higplat API 或其他逻辑
    unsigned int error = 0;
    bool result = false;

    // 示例: 读取队列
    // char* buffer = (char*)p_memory->AllocMemory(pMsgHeader->datasize, false);
    // result = readq_inner(pMsgHeader->qname, buffer, pMsgHeader->datasize, &error);

    // 构造响应
    MSGHEAD tmpMsgHeader;
    memcpy(&tmpMsgHeader, pMsgHeader, sizeof(MSGHEAD));
    tmpMsgHeader.error = error;

    if (result) {
        // 成功响应（带数据）
        // tmpMsgHeader.bodysize = dataSize;
        // char* pSendData = (char*)p_memory->AllocMemory(sizeof(MSGHEAD) + dataSize, false);
        // memcpy(pSendData, &tmpMsgHeader, sizeof(MSGHEAD));
        // memcpy(pSendData + sizeof(MSGHEAD), buffer, dataSize);
        // msgSend(pConn, pSendData, sizeof(MSGHEAD) + dataSize);

        // 成功响应（无数据）
        SendNoBodyPkgToClient(&tmpMsgHeader, MSGID::SUCCEED);
    } else {
        // 失败响应
        SendNoBodyPkgToClient(&tmpMsgHeader, MSGID::FAIL);
    }

    // 清理
    // if (buffer) p_memory->FreeMemory(buffer);
}

// 4. 在 ThreadRecvProcFunc 添加消息分发
/*
void CLogicSocket::ThreadRecvProcFunc(char* pMsgBuf) {
    // ...
    switch (pMsgHeader->id) {
        // ...
        case MSGID::NEW_COMMAND_NAME:
            HandleCommandName(pConn, pMsgHeader, pPkgBody, bodyLength);
            break;
    }
}
*/
