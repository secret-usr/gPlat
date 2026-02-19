# CLAUDE.md - gplat 项目 AI 开发指南

本文档为 Claude AI 助手提供项目上下文和开发指南，确保高效协作和代码质量。

---

## 项目概览

### 项目名称
**gplat** - 高性能实时数据交换平台

### 项目定位
工业自动化、过程控制和分布式系统的进程间通信（IPC）解决方案

### 技术栈
- **语言**: C++17, C
- **平台**: Linux (Kernel >= 2.6.27)
- **架构**: Nginx 启发的主从进程模型
- **核心技术**: Epoll, Memory-mapped files, POSIX threads
- **构建工具**: CMake, Visual Studio 2022

### 核心组件
1. **gplat/**: 服务端程序（主从进程架构）
2. **higplat/**: 客户端共享库（gplat数据管理库）
3. **testapp/**: 测试客户端
4. **Tool/**: 命令行工具（createq, createb, toolgplat）

---

## 项目架构快速参考

### 目录结构
```
gPlat/
├── gplat/              # 核心服务器
│   ├── nginx.cxx       # 主入口
│   ├── ngx_process_cycle.cxx  # 进程管理
│   ├── ngx_c_socket*.cxx      # 网络层
│   ├── ngx_c_slogic.cxx       # 业务逻辑层
│   └── ngx_c_conf.cxx         # 配置管理
├── higplat/            # gplat 数据管理库
│   ├── higplat.cpp     # gplat 功能实现
│   └── qbd.h           # 数据结构定义
├── include/            # 公共头文件
│   ├── higplat.h       # gplat客户端API
│   ├── msg.h           # 消息协议
│   └── timer_manager.h # 定时器管理
├── Tool/               # 工具集
├── Doc/                # 文档
├── .claude/            # Claude 配置目录
└── qbdfile/            # QBD数据文件（运行时加载）
```

### 关键数据结构
```cpp
// Queue: FIFO 消息队列
struct QUEUE_HEAD {
    int qbdtype, dataType, operateMode;
    int num, size;  // 记录数、记录大小
    int readPoint, writePoint;  // 读写指针
};

// Board: 键值存储（公告板）
struct BOARD_HEAD {
    BOARD_INDEX_STRUCT index[7177];  // 哈希索引
    int counter, totalsize, nextpos;
    std::mutex mutex_rw;
    std::mutex mutex_rw_tag[64];  // 细粒度锁
};

// Database: 表存储
struct DB_HEAD {
    DB_INDEX_STRUCT index[7177];
    // 类似 BOARD_HEAD
};
```

### 通信协议
```cpp
struct MSGHEAD {
    int id;                   // MSGID 枚举
    char qname[40];           // 队列/公告板名称
    char itemname[40];        // 标签名称
    int datasize;             // 数据大小
    timespec timestamp;       // 时间戳
    unsigned int error;       // 错误码
    // ... 更多字段
};
```

---

## 开发规范

### 代码风格

#### 命名约定
```cpp
// 类名: 大驼峰 + C 前缀
class CSocket { };
class CLogicSocket { };

// 成员变量: m_ 前缀 + 小驼峰
int m_workerProcesses;
std::list<lpngx_connection_t> m_connectionList;

// 函数名: 大驼峰或下划线分隔
void Initialize();
void ngx_process_events_and_timers();

// 宏定义: 全大写下划线分隔
#define NGX_MAX_ERROR_STR 2048
#define DATATYPE_BIN 1
```

#### 文件命名
- **头文件**: `.h` (如 `ngx_c_conf.h`)
- **实现文件**: `.cxx` 或 `.cpp` (如 `ngx_c_conf.cxx`)
- **前缀约定**:
  - `ngx_`: Nginx 风格的核心模块
  - `C`: 类名前缀

### 内存管理
```cpp
// 使用自定义内存池
CMemory* p_memory = CMemory::GetInstance();
void* ptr = p_memory->AllocMemory(size, true);
p_memory->FreeMemory(ptr);

// 连接对象池
lpngx_connection_t p_Conn = GetConnection(sockfd);
// 使用后回收
inRecyConnectQueue(p_Conn);
```

### 线程安全
```cpp
// 使用互斥锁保护共享数据
pthread_mutex_lock(&m_connectionMutex);
// 临界区代码
pthread_mutex_unlock(&m_connectionMutex);

// 使用原子操作
std::atomic<int> m_connectionCounter;

// 消息队列使用信号量
sem_post(&m_semEventSendQueue);
sem_wait(&m_semEventSendQueue);
```

### 错误处理
```cpp
// API 通过 error 参数返回错误码
unsigned int error = 0;
bool result = readq(sockfd, "queue_name", buffer, size, &error);
if (!result) {
    // 检查 error 获取错误信息
    switch (error) {
        case ERROR_QUEUE_NOT_EXIST:
            // 处理队列不存在
            break;
        // ...
    }
}

// 服务端日志记录
ngx_log_error_core(NGX_LOG_ERR, err, "Error message: %s", detail);
```

### 日志规范
```cpp
// 日志级别
NGX_LOG_STDERR    // 标准错误
NGX_LOG_EMERG     // 紧急
NGX_LOG_ALERT     // 警报
NGX_LOG_CRIT      // 严重
NGX_LOG_ERR       // 错误
NGX_LOG_WARN      // 警告
NGX_LOG_NOTICE    // 注意
NGX_LOG_INFO      // 信息
NGX_LOG_DEBUG     // 调试

// 使用方式
ngx_log_stderr(errno, "bind() failed");
ngx_log_error_core(NGX_LOG_INFO, 0, "Server started on port %d", port);
```

---

## 开发工作流

### 构建流程
```bash
# 方式 1: CMake 构建

# 方式 2: Visual Studio
# 打开 gPlat.sln，选择配置（Debug/Release），生成解决方案

# 输出目录
# x64/Debug/ 或 x64/Release/
```

### 测试流程
```bash
# 1. 启动服务端
./gplat

# 2. 运行测试客户端
./testapp
```

### 调试技巧
```bash
# 前台运行（查看日志）
./gplat  # Daemon=0 时

# 查看进程
ps aux | grep gplat

# 查看日志
tail -f error.log

# GDB 调试
gdb ./gplat
(gdb) run
(gdb) bt  # 查看堆栈

# 远程调试（Visual Studio Remote Development）
# 在VS中设置远程目标，F5启动调试
```

---

## Claude 开发指南

### 代码修改原则

#### 1. 理解优先
- **必读文件**: `ARCHITECTURE.md`, `PRODUCT.md`
- **关键代码**:
  - `gplat/nginx.cxx`: 程序入口
  - `gplat/ngx_process_cycle.cxx`: 进程模型
  - `gplat/ngx_c_socket.cxx`: 网络层基础
  - `higplat/higplat.cpp`: 数据管理核心

#### 2. 保持一致性
- **参考现有代码**: 修改前查找类似功能的实现
- **遵循命名**: 使用项目现有的命名风格
- **保持架构**: 不破坏主从进程模型和事件驱动架构

#### 3. 最小化改动
- 优先修改现有代码而非重写
- 避免引入外部依赖
- 保持与 Nginx 风格的一致性

### 常见任务模板

#### 添加新的消息类型
```cpp
// 1. 在 include/msg.h 添加 MSGID 枚举
enum MSGID {
    // ...
    NEW_COMMAND = 100,
};

// 2. 在 ngx_c_slogic.cxx 添加处理函数
void CLogicSocket::HandleNewCommand(
    lpngx_connection_t pConn,
    MSGHEAD* pMsgHeader,
    char* pPkgBody,
    unsigned short bodyLength
) {
    // 实现业务逻辑
}

// 3. 在消息分发函数中添加分支
void CLogicSocket::ThreadRecvProcFunc(char* pMsgBuf) {
    // ...
    switch (pMsgHeader->id) {
        // ...
        case MSGID::NEW_COMMAND:
            HandleNewCommand(pConn, pMsgHeader, pPkgBody, bodyLength);
            break;
    }
}

// 4. 在 include/higplat.h 添加客户端 API
extern "C" bool new_command(
    int sockfd,
    const char* param,
    unsigned int* error
);

// 5. 在 higplat/higplat.cpp 实现客户端函数
bool new_command(int sockfd, const char* param, unsigned int* error) {
    // 构造消息
    // 发送到服务器
    // 接收响应
}
```

#### 添加新的配置项
```cpp
// 1. 在 gplat/ngx_c_conf.h 添加成员变量
class CConfig {
    int m_newConfigItem;
};

// 2. 在 gplat/ngx_c_conf.cxx 读取配置
bool CConfig::Load(const char* pconfName) {
    // ...
    p_tmpstr = GetString("NewConfigItem=");
    m_newConfigItem = atoi(p_tmpstr);
}

// 3. 在 nginx.conf 添加配置项
[Section]
NewConfigItem = 100
```

### 性能优化建议

#### 1. 避免锁竞争
```cpp
// 使用细粒度锁而非全局锁
// BOARD_HEAD 使用 64 个 mutex 而非 1 个
std::mutex mutex_rw_tag[64];
int lockIndex = hash(tagname) % 64;
std::lock_guard<std::mutex> lock(mutex_rw_tag[lockIndex]);
```

#### 2. 减少内存拷贝
```cpp
// 使用内存映射文件而非 read/write
void* data = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

// 使用引用传递而非值传递
void ProcessData(const DataStruct& data);  // 好
void ProcessData(DataStruct data);         // 差
```

---

## 关键接口文档

### 服务端核心类

#### CSocekt (网络层基类)
```cpp
class CSocekt {
    bool Initialize();                    // 初始化监听套接字
    void ngx_epoll_process_events(int timer);  // epoll 事件处理
    lpngx_connection_t ngx_get_connection(int isock);  // 获取连接
    void inRecyConnectQueue(lpngx_connection_t pConn); // 回收连接
};
```

#### CLogicSocket (业务逻辑层)
```cpp
class CLogicSocket : public CSocekt {
    void SendNoBodyPkgToClient(MSGHEAD* pMsgHeader, unsigned short msgCode);
    static void* ServerSendQueueThread(void* threadData);  // 发送线程
    static void* ServerRecvQueueThread(void* threadData);  // 接收线程
    static void* ThreadRecvProcFunc(void* threadData);     // 消息处理线程

    // 业务处理函数
    void HandleConnect(/* ... */);
    void HandleReadQ(/* ... */);
    void HandleWriteQ(/* ... */);
    // ... 更多处理函数
};
```

### 客户端核心 API

#### 连接管理
```cpp
int connectgplat(const char* server, int port);
void disconnectgplat(int sockfd);
```

#### 队列操作
```cpp
bool readq(int sockfd, const char* qname, void* record, int actsize, unsigned int* error);
bool writeq(int sockfd, const char* qname, void* record, int actsize, unsigned int* error);
bool clearq(int sockfd, const char* qname, unsigned int* error);
bool isemptyq(int sockfd, const char* qname, unsigned int* error);
bool isfullq(int sockfd, const char* qname, unsigned int* error);
```

#### 公告板操作
```cpp
bool readb(int sockfd, const char* tagname, void* value, int actsize, unsigned int* error, timespec* timestamp);
bool writeb(int sockfd, const char* tagname, void* value, int actsize, unsigned int* error);
bool readb_string(int sockfd, const char* tagname, char* value, int buffersize, unsigned int* error, timespec* timestamp);
bool writeb_string(int sockfd, const char* tagname, const char* value, unsigned int* error);
```

#### 订阅机制
```cpp
bool subscribe(int sockfd, const char* tagname, unsigned int* error);
bool subscribedelaypost(int sockfd, const char* tagname, const char* eventname, int delaytime, unsigned int* error);
bool waitpostdata(int sockfd, std::string& tagname, int timeout, unsigned int* error);
bool post(int sockfd, const char* tagname, unsigned int* error);
```

---

## 常见问题与解决方案

### 运行问题

**问题**: `bind() failed: Address already in use`
```bash
# 解决: 端口被占用
netstat -tlnp | grep 8777
kill -9 <PID>
# 或修改配置文件端口
```

### 性能问题

**问题**: 高并发下性能下降
```bash
# 解决:
# 1. 增加 Worker 进程数
WorkerProcesses = 4

# 2. 增加消息处理线程数
ProcMsgRecvWorkThreadCount = 8

# 3. 调整连接数限制
# 修改 m_worker_connections
```

---

## 项目资源

### 重要文件清单
- `ARCHITECTURE.md`: 技术架构文档
- `PRODUCT.md`: 产品功能文档
- `CONTRIBUTING.md`: 贡献指南
- `nginx.conf`: 配置文件示例
- `include/higplat.h`: 完整 API 参考
- `include/msg.h`: 协议定义

### 参考资料
- Nginx 源码: 理解主从进程模型
- Linux epoll 文档: 理解事件驱动
- POSIX 线程文档: 理解多线程编程

### 版本控制
```bash
# 提交规范
git commit -m "type(scope): description"
# type: feat, fix, docs, refactor, test, chore
# 例如: feat(socket): add connection timeout handling
```

---

## 开发检查清单

### 代码提交前
- [ ] 代码已编译通过（Debug 和 Release）
- [ ] 已运行测试客户端验证功能
- [ ] 已检查内存泄漏（valgrind）
- [ ] 已更新相关文档
- [ ] 已遵循项目代码风格
- [ ] 日志级别适当（生产环境不使用 DEBUG）
- [ ] 错误处理完整
- [ ] 线程安全（使用锁保护共享数据）

### 新功能开发
- [ ] 设计方案已评审
- [ ] API 接口已定义
- [ ] 协议已扩展（如需要）
- [ ] 客户端和服务端都已实现
- [ ] 已添加示例代码
- [ ] 已更新 ARCHITECTURE.md 和 PRODUCT.md

### Bug 修复
- [ ] 已定位根本原因
- [ ] 已添加日志辅助调试
- [ ] 已验证修复效果
- [ ] 已检查是否影响其他功能
- [ ] 已添加测试用例防止回归

---

## 联系与协作

### Git 工作流
- `master`: 稳定版本，用于发布
- `develop`: 开发分支，日常开发
- `feature/*`: 功能分支
- `bugfix/*`: Bug 修复分支

### 代码审查要点
1. 架构一致性
2. 性能影响
3. 线程安全
4. 内存管理
5. 错误处理
6. 日志完整性
7. 文档更新

---

## 项目状态

**最后更新**: 2026-02-13
**版本**: 开发版
**构建状态**: ✅ 正常
**测试覆盖**: 手动测试

---

**注意**: 本文档将持续更新，请定期查阅最新版本。
