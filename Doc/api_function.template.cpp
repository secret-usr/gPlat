// 客户端 API 函数模板
// 用途: 添加新的客户端 API
// 使用: 复制此模板，替换 function_name 为实际函数名

// 1. 在 include/higplat.h 声明 API
/*
extern "C" bool function_name(
    int sockfd,
    const char* param1,
    void* param2,
    int param3,
    unsigned int* error
);
*/

// 2. 在 higplat/higplat.cpp 实现 API
bool function_name(
    int sockfd,
    const char* param1,
    void* param2,
    int param3,
    unsigned int* error
) {
    // 参数验证
    if (sockfd < 0 || param1 == nullptr || error == nullptr) {
        if (error) *error = ERROR_INVALID_PARAM;
        return false;
    }

    // 构造请求消息
    MSGHEAD msgHead;
    memset(&msgHead, 0, sizeof(MSGHEAD));
    msgHead.id = MSGID::NEW_COMMAND_NAME;  // 对应的消息 ID

    // 填充消息头
    strncpy(msgHead.qname, param1, sizeof(msgHead.qname) - 1);
    msgHead.datasize = param3;
    clock_gettime(CLOCK_REALTIME, &msgHead.timestamp);

    // 计算消息体大小
    int bodySize = 0;
    if (param2 != nullptr && param3 > 0) {
        bodySize = param3;
    }
    msgHead.bodysize = bodySize;

    // 发送请求
    int totalSize = sizeof(MSGHEAD) + bodySize;
    char* sendBuffer = new char[totalSize];
    memcpy(sendBuffer, &msgHead, sizeof(MSGHEAD));
    if (bodySize > 0) {
        memcpy(sendBuffer + sizeof(MSGHEAD), param2, bodySize);
    }

    bool sendResult = sendmsg(sockfd, sendBuffer, totalSize, error);
    delete[] sendBuffer;

    if (!sendResult) {
        return false;
    }

    // 接收响应
    MSGHEAD recvMsgHead;
    char* recvBody = nullptr;
    int recvBodySize = 0;

    bool recvResult = recvmsg(sockfd, &recvMsgHead, &recvBody, &recvBodySize, error);
    if (!recvResult) {
        return false;
    }

    // 处理响应
    *error = recvMsgHead.error;
    bool result = (recvMsgHead.id == MSGID::SUCCEED);

    // 如果需要返回数据
    if (result && recvBodySize > 0 && recvBody != nullptr) {
        // TODO: 复制返回数据到输出参数
        // memcpy(outputParam, recvBody, recvBodySize);
    }

    // 清理
    if (recvBody) {
        delete[] recvBody;
    }

    return result;
}

// 3. 使用示例
/*
int main() {
    unsigned int error = 0;
    int sockfd = connectgplat("127.0.0.1", 8777);

    if (sockfd < 0) {
        printf("Connect failed\n");
        return -1;
    }

    bool result = function_name(sockfd, "param1", data, dataSize, &error);
    if (!result) {
        printf("Operation failed, error: %u\n", error);
    }

    disconnectgplat(sockfd);
    return 0;
}
*/
