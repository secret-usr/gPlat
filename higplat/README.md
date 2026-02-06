# gPlat

本文档详细描述了 `higplat` 库的设计、内部数据结构及函数接口。该库旨在提供高性能的队列（Queue）和公告板（Bulletin/Board）数据存储与访问机制。

## 架构设计

### 1. 内存映射文件
gPlat 的核心机制是利用操作系统的内存映射文件功能，Linux 下为 `mmap`。
*   数据存储：所有队列和公告板数据均以文件形式持久化存储在磁盘上。
*   数据访问：文件被直接映射到进程的虚拟地址空间。对数据的读写操作直接转换为对内存的读写，极大地提高了性能，避免了用户态与内核态之间频繁的数据拷贝。

### 2. 单进程加载模型
约束：在当前设计中，只有一个进程，即服务端进程负责加载（`LoadQ`/`LoadB`）和创建（`CreateQ`/`CreateB`）资源。
*   此进程将文件映射到其内存空间。
*   本地访问：该进程使用 Local API（ `ReadQ`, `WriteQ`）直接操作内存。
*   远程访问：其他进程不能直接映射同一文件，而是通过 TCP socket 连接该管理进程，使用 Client API（ `readq`, `writeq`）发送请求。
*   并发控制：由于只有可以通过单进程的内存空间访问数据，库使用进程内的 `std::mutex` 即可保证线程安全，无需使用开销更大的跨进程同步原语。

### 3. 全局资源管理表
管理进程维护一个全局静态哈希表 `table`，用于记录当前已加载的资源。
*   结构：`TABLE_MSG table[TABLESIZE]`
*   查找：使用双重哈希算法 (`hash1`, `hash2`) 解决冲突，通过队列/公告板名称快速定位资源。
*   内容：每个表项记录了文件的句柄、内存映射的首地址、以及用于同步的互斥锁 (`std::mutex`)。

## 内部数据结构

### 1. TABLE_MSG
用于在内存中管理已加载的队列或公告板。
```cpp
struct TABLE_MSG
{
    char dqname[MAXDQNAMELENTH]; // 资源名称
    int  hFile;                  // 文件描述符
    void* lpMapAddress;          // 内存映射首地址 (mmap返回的指针)
    int hMapFile;                // (Linux下未使用，保留兼容性)
    pthread_mutex_t hMutex;      // (保留)
    std::mutex * pmutex_rw;      // 读写互斥锁，保护该资源的并发访问
    bool erased;                 // 删除标记
    int count;                   // 引用计数
    long filesize;               // 文件大小
};
```

### 2. QUEUE_HEAD
队列文件的头部结构，位于映射内存的起始位置。
```cpp
struct QUEUE_HEAD
{
    int  qbdtype;       // 类型标识
    int  dataType;      // 1: ASCII, 0: BINARY
    int  operateMode;   // 1: Shift Mode (覆盖), 0: Normal Mode (阻塞/报错)
    int  num;           // 最大记录数
    int  size;          // 单条记录大小
    int  readPoint;     // 读指针 (索引)
    int  writePoint;    // 写指针 (索引)
    // ... 其他元数据
};
```
*   运作方式：环形缓冲区。`writePoint` 指向下一个写入位置，`readPoint` 指向下一个读取位置。读写时需操作 `pmutex_rw` 锁。

### 3. BOARD_HEAD & BOARD_INDEX_STRUCT
公告板类似于基于内存的 Key-Value 数据库。
*   `BOARD_HEAD`：位于文件头部，包含一个内部哈希索引数组 `index[INDEXSIZE]`。
*   `BOARD_INDEX_STRUCT`：索引项，记录了 Tag 名称 (`itemname`) 到数据偏移量 (`startpos`) 的映射。
*   运作方式：通过 Tag 名称在头部索引区查找偏移量，然后直接访问数据区。

## 函数接口说明

函数分为两类：
1.  Local API：首字母大写 (例如 `ReadQ`)。由加载资源的进程调用，直接操作内存。
2.  Client/Network API：全小写 (例如 `readq`)。由客户端调用，通过网络发送请求。

### 一、 Local API

#### 1. 资源生命周期管理 (服务端操作)

*   CreateQ
    *   `bool CreateQ(const char* lpFileName, int recordSize, int recordNum, int dataType, int operateMode, ...)`
    *   作用：创建一个新的队列文件，并初始化 `QUEUE_HEAD`。
    *   参数：
        *   `recordSize`: 单条记录字节数。
        *   `recordNum`: 队列容量。
        *   `operateMode`: 1 为移位模式（满时覆盖旧数据），0 为通用模式。
    *   实现：创建文件 -> `ftruncate` 设置大小 -> 写入初始化的 Header。

*   CreateB
    *   `bool CreateB(const char* lpFileName, int size)`
    *   作用：创建一个新的公告板文件。
    *   实现：同上，初始化 `BOARD_HEAD` 和空的索引区。

*   LoadQ
    *   `bool LoadQ(const char* lpDqName)`
    *   作用：加载指定的队列/公告板文件到内存。
    *   实现：
        1.  检查全局 Hash 表是否已存在。
        2.  `open` 打开文件。
        3.  `mmap` 映射文件到内存。
        4.  创建 `std::mutex` 并存储在 `TABLE_MSG` 中。
        5.  将信息存入全局 Hash 表。

*   CreateItem
    *   `bool CreateItem(const char* lpBoardName, const char* lpItemName, int itemSize, ...)`
    *   作用：在已加载的公告板中创建一个新的 Tag 数据项。
    *   实现：
        1.  在 `BOARD_HEAD` 的索引区查找空位。
        2.  计算数据区的偏移量。
        3.  更新索引信息 (`itemname`, `startpos`, `itemsize`)。
*   ClearQ / ClearB
    *   `bool ClearQ(const char* lpDqName)`
    *   作用：清空队列（重置读写指针）或公告板。
    *   线程安全：需获取资源的 `pmutex_rw` 锁。

#### 2. 数据读写 (内存操作)

*   ReadQ
    *   `bool ReadQ(const char* lpDqName, void* lpRecord, int actSize, char* remoteIp)`
    *   作用：从队列头部读取一条记录。
    *   实现：
        1.  在 Hash 表查找资源，获取内存指针。
        2.  加锁。
        3.  检查队列是否为空 (`readPoint == writePoint` 且非满状态)。
        4.  根据 `readPoint` 计算内存偏移，`memcpy` 数据到用户缓冲区。
        5.  更新 `readPoint`（环形递增）。
        6.  解锁。

*   WriteQ
    *   `bool WriteQ(const char* lpDqName, void* lpRecord, int actSize, const char* remoteIp)`
    *   作用：向队列尾部写入一条记录。
    *   实现：与 `ReadQ` 类似，操作 `writePoint`。如果是移位模式且队列已满，会自动覆盖最旧数据并推进 `readPoint`。

*   ReadB / WriteB
    *   包含：`ReadB`, `WriteB`, `ReadB_String`, `WriteB_String`, `ReadB_String2` 等。
    *   作用：读/写公告板中的指定 Tag。
    *   变体：分别支持定长数据 (`void*`)、C 风格字符串 (`char*`) 和 C++ 字符串 (`std::string`) 的读写。
    *   实现：通过 Tag 名在 Header 索引中查找偏移量，然后直接读写对应内存区域。支持时间戳更新。

*   GetLastErrorQ
    *   `unsigned int GetLastErrorQ()`
    *   作用：获取当前线程最近一次错误的错误码（存储在 `thread_local` 变量中）。

### 二、 Client/Network API (客户端/网络操作)

此类函数封装了 TCP 通信细节，将请求发送给服务端。

#### 1. 连接管理

*   connectgplat
    *   `int connectgplat(const char* server, int port)`
    *   作用：连接到 gPlat 服务端。
    *   实现：
        *   创建 socket。
        *   设置 `TCP_NODELAY` 禁用 Nagle 算法以降低小包延迟。
        *   执行非阻塞 `connect` 带超时控制。
        *   连接成功后设置回阻塞模式。

#### 2. 远程数据操作

bool readq(int sockfd, const char* qname, void* record, int actsize, unsigned int* error)
- param
- return
- brief
- detail

*   readq / writeq
    *   `bool readq(int sockfd, const char* qname, void* record, int actsize, unsigned int* error)`
    *   作用：发送读/写队列请求。
    *   协议：
        1.  构造 `MSGSTRUCT` 消息包，填充 `MSGHEAD` (包含 ID, qname, size 等)。
        2.  `send_all` 发送头部（写入时紧接着发送数据体）。
        3.  `readn` 等待并读取响应头部。
        4.  根据响应头中的 `error` 判断成功与否，并读取返回的数据。

*   clearq
    *   `bool clearq(int sockfd, const char* qname, unsigned int* error)`
    *   作用：发送清空指令，重置指定队列或公告板的状态。
    *   对应的 Local API：`ClearQ`。

*   readb / writeb 及其 String 变体
    *   包含：`readb`, `writeb`, `readb_string`, `writeb_string`, `readb_string2`, `writeb_string2`。
    *   作用：针对公告板（Board）的 Tag 数据进行读写。
    *   变体说明：
        *   `readb`/`writeb`: 处理定长二进制数据（结构体等）。
        *   `readb_string`/`writeb_string`: 处理 `char*` 类型的 C 风格字符串。
        *   `readb_string2`/`writeb_string2`: 处理 `std::string` 类型，简化内存管理。
    *   协议细节：消息头中包含 `itemname` (Tag 名) 用于区分不同数据项。

*   subscribe
    *   `bool subscribe(int sockfd, const char* eventname, unsigned int* error)`
    *   作用：向服务端发起订阅请求，监听指定事件的变化。
    *   机制：发送 ID 为 `SUBSCRIBE` 且 EventID 为 `DEFAULT` 的消息。服务端一旦检测到该事件触发，会立即推送通知给客户端。
    *   参数：
        *   `eventname`: 要订阅的事件名称。

*   subscribedelaypost
    *   `bool subscribedelaypost(int sockfd, const char* tagname, const char* eventname, int delaytime, unsigned int* error)`
    *   作用：注册一个延迟触发的订阅。
    *   机制：发送 ID 为 `SUBSCRIBE` 且 EventID 为 `POST_DELAY` 的消息。当 `tagname` 对应的数据发生变化或触发条件满足时，服务端不会立即通知，而是启动一个时长为 `delaytime` 的定时器。定时结束后，服务端发送名为 `eventname` 的事件通知。
    *   参数：
        *   `tagname`: 被监控的源数据项名称。
        *   `eventname`: 延迟结束后，客户端收到的事件名称（由 `waitpostdata` 接收）。
        *   `delaytime`: 延迟时间。

*   waitpostdata
    *   `bool waitpostdata(int sockfd, std::string& tagname, int timeout, unsigned int* error)`
    *   作用：阻塞等待服务端推送的数据，与 `subscribe` 配合使用。
    *   参数：
        *   `timeout`: 超时时间（毫秒）。
        *   `tagname`: 输出参数，返回收到数据的事件名称。

waitpostdata + subscribe <=> readb
writeb 保留

*   createtag
    *   `bool createtag(int sockfd, const char* tagname, int tagsize, void* type, int typesize, unsigned int* error)`
    *   作用：客户端请求在服务端创建新的 Tag（公告板模式）。
    *   对应的 Local API：`CreateItem`。
    *   参数：
        *   `tagname`: 新建 Tag 的名称。
        *   `tagsize`: 数据最大字节数。

### 三、函数签名一览表

```cpp
extern "C" int connectgplat(const char* server, int port);
extern "C" bool readq(int sockfd, const char* qname, void* record, int actsize, unsigned int* error);
extern "C" bool writeq(int sockfd, const char* qname, void* record, int actsize, unsigned int* error);
extern "C" bool clearq(int sockfd, const char* qname, unsigned int* error);
extern "C" bool readb(int sockfd, const char* tagname, void* value, int actsize, unsigned int* error, timespec* timestamp = 0);
extern "C" bool writeb(int sockfd, const char* tagname, void* value, int actsize, unsigned int* error);
extern "C" bool subscribe(int sockfd, const char* tagname, unsigned int* error);
extern "C" bool subscribedelaypost(int sockfd, const char* tagname, const char* eventname, int delaytime, unsigned int* error);
extern "C" bool createtag(int sockfd, const char* tagname, int tagsize, void* type, int typesize, unsigned int* error);
extern "C" bool waitpostdata(int sockfd, std::string& tagname, int timeout, unsigned int* error);
extern "C" bool readb_string(int sockfd, const char* tagname, char* value, int buffersize, unsigned int* error, timespec*timestamp=0);
extern "C" bool writeb_string(int sockfd, const char* tagname, const char* value, unsigned int* error);
extern "C" bool readb_string2(int sockfd, const char* tagname, std::string& value, unsigned int* error, timespec* timestamp = 0);
extern "C" bool writeb_string2(int sockfd, const char* tagname, std::string& value, unsigned int* error);
extern "C" bool CreateB(const char* lpFileName, int size);
extern "C" bool CreateItem(const char* lpBoardName, const char* lpItemName, int itemSize, void* pType = 0, int typeSize = 0);
extern "C" bool CreateQ(const char* lpFileName, int recordSize, int recordNum, int dateType, int operateMode, void* pType = 0, int typeSize = 0);
extern "C" bool LoadQ(const char* lpDqName );
extern "C" bool ReadQ(const char* lpDqName, void  *lpRecord, int actSize, char* remoteIp=0 );
extern "C" bool WriteQ(const char* lpDqName, void  *lpRecord, int actSize=0, const char* remoteIp=0 );
extern "C" bool ClearQ(const char* lpDqName );
extern "C" bool ReadB(const char* lpBoardName, const char* lpItemName, void* lpItem, int actSize, timespec* timestamp = 0);
extern "C" bool ReadB_String(const char* lpBulletinName, const char* lpItemName, void*lpItem, int actSize, timespec*timestamp=0);
extern "C" bool ReadB_String2(const char* lpBulletinName, const char* lpItemName, void* lpItem, int actSize, int& strLength, timespec* timestamp);
extern "C" bool WriteB(const char* lpBulletinName, const char* lpItemName, void* lpItem, int actSize, void* lpSubItem = 0, int actSubSize = 0);
extern "C" bool WriteB_String(const char* lpBulletinName, const char* lpItemName, void *lpItem, int actSize, void *lpSubItem = 0, int actSubSize = 0);
extern "C" bool GetLastErrorQ();
```

## 线程与进程安全

1.  进程间安全：
    *   采用内存映射。
    *   客户端进程不直接接触共享内存，完全依赖 Socket 与服务端通信。

2.  线程间安全：
    *   Hash 表保护：全局 `mutex_rw` 保护资源表的插入/查找操作。
    *   资源保护：每个资源 (`TABLE_MSG`) 拥有独立的互斥锁指针 `pmutex_rw`。
    *   粒度：锁粒度为单个队列/公告板级别。读写操作期间全程持有锁。