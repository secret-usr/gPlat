# API 快速参考

## 连接管理

### connectgplat
```cpp
int connectgplat(const char* server, int port);
```
- **功能**: 连接到 gPlat 服务器
- **返回**: socket 文件描述符（失败返回 -1）
- **示例**: `int sockfd = connectgplat("127.0.0.1", 8777);`

### disconnectgplat
```cpp
void disconnectgplat(int sockfd);
```
- **功能**: 断开连接

---

## 队列操作 (Queue)

### CreateQ
```cpp
bool CreateQ(const char* lpFileName, int recordSize, int recordNum,
             int dateType, int operateMode, void* pType, int typeSize);
```
- **参数**:
  - `dateType`: DATATYPE_ASCII (0) 或 DATATYPE_BIN (1)
  - `operateMode`: NORMAL_MODE (0) 或 SHIFT_MODE (1)

### readq
```cpp
bool readq(int sockfd, const char* qname, void* record,
           int actsize, unsigned int* error);
```
- **功能**: 读取队列记录（FIFO）
- **错误码**: ERROR_QUEUE_NOT_EXIST, ERROR_QUEUE_EMPTY

### writeq
```cpp
bool writeq(int sockfd, const char* qname, void* record,
            int actsize, unsigned int* error);
```
- **功能**: 写入队列记录
- **错误码**: ERROR_QUEUE_NOT_EXIST, ERROR_QUEUE_FULL

### clearq
```cpp
bool clearq(int sockfd, const char* qname, unsigned int* error);
```
- **功能**: 清空队列

### isemptyq / isfullq
```cpp
bool isemptyq(int sockfd, const char* qname, unsigned int* error);
bool isfullq(int sockfd, const char* qname, unsigned int* error);
```

---

## 公告板操作 (Board)

### CreateB
```cpp
bool CreateB(const char* lpFileName, int size);
```
- **参数**: `size` - 公告板总大小（字节）

### createtag
```cpp
bool createtag(int sockfd, const char* tagname, int tagsize,
               void* type, int typesize, unsigned int* error);
```
- **功能**: 创建标签

### readb
```cpp
bool readb(int sockfd, const char* tagname, void* value, int actsize,
           unsigned int* error, timespec* timestamp);
```
- **功能**: 读取标签值
- **参数**: `timestamp` - 可选，返回数据时间戳

### writeb
```cpp
bool writeb(int sockfd, const char* tagname, void* value,
            int actsize, unsigned int* error);
```
- **功能**: 写入标签值

### readb_string / writeb_string
```cpp
bool readb_string(int sockfd, const char* tagname, char* value,
                  int buffersize, unsigned int* error, timespec* timestamp);
bool writeb_string(int sockfd, const char* tagname, const char* value,
                   unsigned int* error);
```
- **功能**: 字符串专用读写

---

## 数据库操作 (Database)

### createtable
```cpp
bool createtable(int sockfd, const char* tablename, int recordsize,
                 int maxcount, void* type, int typesize, unsigned int* error);
```

### inserttb
```cpp
bool inserttb(int sockfd, const char* tablename, void* record,
              int actsize, unsigned int* error);
```

### selecttb
```cpp
int selecttb(int sockfd, const char* tablename, int start, int count,
             void* records, int buffersize, unsigned int* error);
```
- **返回**: 实际读取的记录数

### cleartb
```cpp
bool cleartb(int sockfd, const char* tablename, unsigned int* error);
```

---

## 订阅机制 (Pub/Sub)

### subscribe
```cpp
bool subscribe(int sockfd, const char* tagname, unsigned int* error);
```
- **功能**: 订阅标签变化

### subscribedelaypost
```cpp
bool subscribedelaypost(int sockfd, const char* tagname,
                        const char* eventname, int delaytime,
                        unsigned int* error);
```
- **参数**: `delaytime` - 延迟时间（毫秒）

### waitpostdata
```cpp
bool waitpostdata(int sockfd, std::string& tagname,
                  int timeout, unsigned int* error);
```
- **功能**: 阻塞等待事件
- **参数**: `timeout` - 超时时间（毫秒）
- **返回**: `tagname` - 触发事件的标签名

### post
```cpp
bool post(int sockfd, const char* tagname, unsigned int* error);
```
- **功能**: 主动发布事件

---

## 错误码 (部分)

```cpp
#define ERROR_QUEUE_NOT_EXIST    1001
#define ERROR_QUEUE_EMPTY        1002
#define ERROR_QUEUE_FULL         1003
#define ERROR_BOARD_NOT_EXIST    2001
#define ERROR_TAG_NOT_EXIST      2002
#define ERROR_INVALID_PARAM      9001
```

---

## 数据类型

```cpp
#define DATATYPE_ASCII  0
#define DATATYPE_BIN    1

#define NORMAL_MODE     0
#define SHIFT_MODE      1  // 满时覆盖最旧数据
```

---

## 常用模式

### 基本读写
```cpp
int sockfd = connectgplat("127.0.0.1", 8777);
unsigned int error = 0;

int value = 100;
writeb(sockfd, "temperature", &value, sizeof(value), &error);
readb(sockfd, "temperature", &value, sizeof(value), &error, NULL);

disconnectgplat(sockfd);
```

### 事件驱动
```cpp
subscribe(sockfd, "alarm", &error);
while (true) {
    std::string tagname;
    if (waitpostdata(sockfd, tagname, 5000, &error)) {
        printf("Event: %s\n", tagname.c_str());
        // 处理事件
    }
}
```

### 批量处理
```cpp
struct Record { int id; char data[64]; };
Record records[100];
int count = selecttb(sockfd, "history", 0, 100, records, sizeof(records), &error);
for (int i = 0; i < count; i++) {
    // 处理记录
}
```
