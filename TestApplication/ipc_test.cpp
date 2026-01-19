#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include "../include/higplat_linux.h"

// 简单的辅助打印
#define LOG(role, msg) std::cout << "[" << role << "] " << msg << std::endl

int main() {
    const char* qName = "verifyQ";
    const int defSize = 32; 
    const int recSize = 64; 
    const int recNum = 10;
    
    // 1. 先由父进程创建队列（ASCII模式，便于查看文件）
    // 参数: name, size, num, type(1=ASCII), mode(0=NORMAL), typePtr, typeSize
    if (!CreateQ(qName, defSize, recNum, 1, 0, nullptr, 0)) {
        std::cerr << "创建队列失败" << std::endl;
        return -1;
    }
    LOG("Main", "队列创建成功 (File created on disk)");

    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "Fork failed" << std::endl;
        return -1;
    }

    if (pid > 0) {
        // ================= 父进程 (Writer) =================
        sleep(1); 
        
        if (!LoadQ(qName)) {
            LOG("Parent", "LoadQ 失败");
            return -1;
        }

        const char* msg = "Message_From_Parent";
        LOG("Parent", "正在写入数据: " << msg);

        // 构造一个固定大小的缓冲区
        char writeBuf[recSize];
        memset(writeBuf, 0, recSize);
        strcpy(writeBuf, msg);
        
        if (WriteQ(qName, (void*)writeBuf, recSize, nullptr)) {
            LOG("Parent", "WriteQ 成功");
        } else {
            // 打印一下错误码以便调试
            LOG("Parent", "WriteQ 失败, Error: " << GetLastErrorQ());
        }

        LOG("Parent", "任务完成，等待子进程...");
        wait(NULL); // 等待子进程结束
        UnloadQ(qName);
    } 
    else {
        // ================= 子进程 (Reader) =================
        sleep(3); // 确保父进程已经执行了 WriteQ 操作

        LOG("Child", "尝试加载同一个队列...");
        if (!LoadQ(qName)) {
            LOG("Child", "LoadQ 失败");
            return -1;
        }

        LOG("Child", "尝试读取数据...");
        char buffer[128] = {0};
        char ip[32] = {0};

        // 尝试读取
        // 如果 MAP_PRIVATE 生效，子进程映射的是磁盘上的原始文件
        // 而原始文件并没有被父进程更新
        bool result = ReadQ(qName, buffer, recSize, ip);

        if (result) {
            std::cout << "\033[1;32m[Child] 读到了数据: " << buffer << std::endl;
        } else {
            unsigned int err = GetLastErrorQ();
            std::cout << "\033[1;31m[Child]未读到数据! 错误码: " << err << "\033[0m" << std::endl; 
            //std::cout << "[Child] 结论验证成功: 父进程的写入对子进程不可见。" << std::endl;
        }

        UnloadQ(qName);
    }

    return 0;
}