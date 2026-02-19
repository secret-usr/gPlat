# gPlat 产品文档

## 产品概述

**gPlat** 是一个高性能的实时数据交换平台，专为工业自动化、过程控制和分布式系统设计。它提供了队列、公告板和数据库三种核心数据结构，支持进程间通信（IPC）、事件驱动编程和实时数据共享。

### 核心价值
- **低延迟通信**: 基于内存映射文件的零拷贝技术，实现微秒级数据访问
- **高可靠性**: 数据持久化 + CRC32 校验 + 主从进程架构
- **简单易用**: 直观的 C API，无需复杂配置即可快速集成
- **实时性保障**: 事件驱动架构 + 发布订阅模式，消除轮询开销

---

## 目标用户

### 主要用户群体
1. **工业自动化工程师**
   - SCADA/HMI 系统集成
   - PLC 与上位机数据交换
   - 设备状态监控和控制

2. **嵌入式系统开发者**
   - 实时操作系统（RTOS）应用开发
   - 边缘计算设备数据处理
   - IoT 网关开发

3. **过程控制工程师**
   - 分布式控制系统（DCS）
   - 多模块协调与数据共享
   - 批处理数据缓冲

4. **系统集成商**
   - 微服务架构中的 IPC
   - 事件驱动系统设计
   - 分布式应用数据同步

### 技术要求
- Linux 系统基础知识
- C/C++ 编程能力
- 进程间通信概念理解
- 网络编程经验

---

## 解决的核心问题

### 1. 进程间通信复杂性
**问题**: 传统 IPC 方法（共享内存、管道、消息队列）需要手动处理同步、序列化和错误处理。

**解决方案**: gPlat 提供统一的高层 API，内置同步机制、类型安全和错误处理。
```c
// 简单的队列读写
writeq(sockfd, "sensor_data", &temperature, sizeof(temperature), &error);
readq(sockfd, "sensor_data", &temperature, sizeof(temperature), &error);
```

### 2. 实时数据共享
**问题**: 多个进程需要访问最新的传感器数据或控制参数，传统数据库性能不足。

**解决方案**: 内存映射的公告板（Board）实现零拷贝、低延迟的数据访问。
```c
// 读写实时数据
writeb(sockfd, "temperature", &value, sizeof(value), &error);
readb(sockfd, "temperature", &value, sizeof(value), &error, &timestamp);
```

### 3. 事件驱动协调
**问题**: 轮询方式检测数据变化浪费 CPU 资源。

**解决方案**: 发布订阅机制实现事件驱动，数据变化时自动通知。
```c
// 订阅数据变化
subscribe(sockfd, "alarm_status", &error);
waitpostdata(sockfd, tagname, timeout, &error);  // 阻塞等待事件
```

### 4. 数据持久化与容错
**问题**: 进程崩溃导致数据丢失。

**解决方案**: 内存映射文件自动持久化，进程重启后数据完整恢复。

---

## 核心功能特性

### 1. 队列（Queue）

**应用场景**: 有序消息传递、任务队列、日志缓冲

**核心功能**:
- FIFO 顺序保证
- 固定大小记录
- 支持二进制和 ASCII 数据
- 满队列处理策略（阻塞/覆盖最旧数据）

**API 示例**:
```c
// 创建队列
CreateQ("task_queue", sizeof(Task), 1000, DATATYPE_BIN, SHIFT_MODE, NULL, 0);

// 写入任务
Task task = {...};
writeq(sockfd, "task_queue", &task, sizeof(task), &error);

// 读取任务
Task task;
readq(sockfd, "task_queue", &task, sizeof(task), &error);

// 清空队列
clearq(sockfd, "task_queue", &error);
```

**特性**:
- 记录级元数据（创建时间、源 IP、确认标志）
- 循环缓冲区高效实现
- 并发安全

### 2. 公告板（Board）

**应用场景**: 实时数据共享、配置参数、状态变量

**核心功能**:
- 键值存储（tag-value）
- 动态创建/删除标签
- 时间戳跟踪数据新鲜度
- 支持字符串和二进制数据

**API 示例**:
```c
// 创建标签
createtag(sockfd, "robot_position", sizeof(Position), NULL, 0, &error);

// 写入数据
Position pos = {x: 100.5, y: 200.3};
writeb(sockfd, "robot_position", &pos, sizeof(pos), &error);

// 读取数据（带时间戳）
Position pos;
timespec timestamp;
readb(sockfd, "robot_position", &pos, sizeof(pos), &error, &timestamp);

// 字符串操作
writeb_string(sockfd, "device_status", "Running", &error);
char status[64];
readb_string(sockfd, "device_status", status, sizeof(status), &error, &timestamp);
```

**特性**:
- 哈希索引快速查找
- 细粒度锁（64 个锁）提升并发性能
- 支持最多 7177 个标签

### 3. 数据库（Database）

**应用场景**: 结构化数据存储、历史记录、批量数据处理

**核心功能**:
- 多表支持
- 固定大小记录
- 表级管理

**API 示例**:
```c
// 创建表
createtable(sockfd, "alarm_history", sizeof(AlarmRecord), 10000, NULL, 0, &error);

// 插入记录
AlarmRecord record = {...};
inserttb(sockfd, "alarm_history", &record, sizeof(record), &error);

// 查询记录
AlarmRecord records[100];
int count = selecttb(sockfd, "alarm_history", 0, 100, records, sizeof(records), &error);

// 清空表
cleartb(sockfd, "alarm_history", &error);
```

### 4. 发布订阅（Pub/Sub）

**应用场景**: 事件通知、数据变化监听、模块间解耦

**核心功能**:
- 标签订阅
- 延迟发布（定时触发）
- 边缘触发（上升沿/下降沿）
- 阻塞等待事件

**API 示例**:
```c
// 订阅标签变化
subscribe(sockfd, "emergency_stop", &error);

// 延迟发布（5秒后触发）
subscribedelaypost(sockfd, "auto_save", "save_event", 5000, &error);

// 等待事件（带超时）
std::string tagname;
if (waitpostdata(sockfd, tagname, 10000, &error)) {
    printf("Event triggered: %s\n", tagname.c_str());
}

// 主动发布事件
post(sockfd, "alarm_cleared", &error);
```

**高级特性**:
- 支持多个订阅者
- 事件优先级（隐式支持）
- 超时控制

### 5. 定时器管理

**应用场景**: 周期性任务、延迟操作、超时检测

**核心功能**:
- 周期定时器
- 一次性定时器
- 毫秒级精度

**示例**:
```cpp
// 创建周期定时器（每 1000ms 触发）
TimerManager timer;
int timerId = timer.CreateTimer(1000, [](void* arg) {
    printf("Periodic task executed\n");
}, false);  // false 表示周期性

// 创建一次性定时器
int oneShotId = timer.CreateTimer(5000, callback, true);

// 删除定时器
timer.DeleteTimer(timerId);
```

---

## 产品优势

### 与竞品对比

| 特性 | gPlat | Redis/Memcached | RabbitMQ/ZeroMQ | POSIX SHM | OPC UA/DDS |
|------|-------|----------------|----------------|-----------|------------|
| **本地 IPC 延迟** | 微秒级 | 毫秒级（网络栈） | 毫秒级 | 微秒级 | 毫秒级 |
| **数据持久化** | ✅ 自动 | ✅ 需配置 | ❌ 默认无 | ❌ 无 | ❌ 无 |
| **网络透明** | ✅ 同一 API | ✅ | ✅ | ❌ 仅本地 | ✅ |
| **类型安全** | ✅ 原生 C 结构体 | ❌ 序列化开销 | ❌ 序列化开销 | ✅ | ⚠️ IDL 定义 |
| **部署复杂度** | 低（单进程） | 中（独立服务） | 中（Broker） | 低 | 高（标准复杂） |
| **实时性** | ✅ 确定性 | ⚠️ 不确定 | ⚠️ 不确定 | ✅ | ✅ |
| **资源占用** | 低 | 中 | 中 | 低 | 高 |
| **工业应用** | ✅ 优化 | ⚠️ 通用 | ⚠️ 通用 | ❌ 原始 | ✅ 标准 |

### 关键差异化优势

1. **混合架构**: 内存性能 + 文件持久化
2. **零学习曲线**: 简单的 C 函数调用，无需学习复杂的中间件
3. **工业级可靠性**: Nginx 架构验证 + 主从进程模型
4. **嵌入式友好**: 低资源占用，适合资源受限环境
5. **Linux 深度优化**: 利用 epoll、timerfd、mmap 等内核特性

---

## 典型应用场景

### 场景 1: SCADA 系统数据交换

```
┌─────────────┐      ┌─────────────┐      ┌─────────────┐
│  PLC Driver │      │   gPlat     │      │  HMI Client │
│             ├─────>│   Server    │<─────┤             │
│ writeb()    │ 写入 │             │ 读取 │  subscribe()│
└─────────────┘ 数据 │  Queue +    │ 订阅 └─────────────┘
                     │  Board      │ 事件
                     └─────────────┘
```

**实现**:
- PLC 驱动程序将采集数据写入队列
- HMI 订阅关键参数变化，实时显示
- 历史数据存储到数据库表

### 场景 2: 边缘计算数据聚合

```
┌──────────┐  ┌──────────┐  ┌──────────┐
│ Sensor 1 │  │ Sensor 2 │  │ Sensor N │
└────┬─────┘  └────┬─────┘  └────┬─────┘
     │             │             │
     └─────────────┼─────────────┘
                   ▼
            ┌─────────────┐
            │   gPlat     │
            │  (Gateway)  │
            └──────┬──────┘
                   │
            ┌──────▼──────┐
            │ Cloud/Local │
            │  Analytics  │
            └─────────────┘
```

**实现**:
- 多个传感器进程写入各自队列
- 数据聚合进程读取队列，处理后上传
- 公告板存储最新状态供本地监控

### 场景 3: 分布式控制系统模块协调

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  Controller │<--->│   gPlat     │<--->│   Monitor   │
│   Module    │     │   Server    │     │   Module    │
└─────────────┘     └──────┬──────┘     └─────────────┘
                           │
                    ┌──────▼──────┐
                    │   Safety    │
                    │   Module    │
                    └─────────────┘
```

**实现**:
- 控制器写入控制指令到公告板
- 监控模块订阅关键参数，异常时发布报警
- 安全模块监听紧急停止事件，触发保护

### 场景 4: 批处理任务队列

```
┌─────────────┐      ┌─────────────┐      ┌─────────────┐
│   Task      │      │   gPlat     │      │   Worker    │
│  Generator  ├─────>│   Queue     │<─────┤   Pool      │
└─────────────┘      └─────────────┘      └─────────────┘
                            │
                     (持久化到文件)
```

**实现**:
- 任务生成器写入任务到队列
- Worker 进程从队列读取任务并执行
- 队列满时自动覆盖最旧任务（SHIFT 模式）

---

## 快速开始

### 1. 部署服务端

**安装**:
```bash
# 创建数据目录
mkdir -p qbdfile

# 配置文件 nginx.conf
[Socket]
ListenPort = 8777
WorkerProcesses = 1

[Proc]
Daemon = 1
ProcMsgRecvWorkThreadCount = 4
```

**启动服务**:
```bash
./gplat
# 或前台运行（调试）
./gplat -c nginx.conf
```

### 2. 创建数据结构

**创建队列**:
```bash
# 创建传感器数据队列（记录大小 64 字节，容量 1000）
./createq sensor_queue 64 1000

# 创建二进制队列（SHIFT 模式）
./createq task_queue 256 500 -b -s
```

**创建公告板**:
```bash
# 创建 1MB 公告板
./createb device_status 1048576
```

### 3. 客户端编程

**C++ 示例**:
```cpp
#include "higplat.h"

int main() {
    unsigned int error;

    // 连接服务器
    int sockfd = connectgplat("127.0.0.1", 8777);
    if (sockfd < 0) {
        printf("Connection failed\n");
        return -1;
    }

    // 创建标签
    int temperature = 0;
    createtag(sockfd, "temp_sensor", sizeof(int), NULL, 0, &error);

    // 写入数据
    temperature = 25;
    writeb(sockfd, "temp_sensor", &temperature, sizeof(int), &error);

    // 订阅变化
    subscribe(sockfd, "temp_sensor", &error);

    // 等待事件
    std::string tagname;
    while (true) {
        if (waitpostdata(sockfd, tagname, 5000, &error)) {
            int value;
            readb(sockfd, tagname.c_str(), &value, sizeof(value), &error, NULL);
            printf("Temperature changed: %d\n", value);
        }
    }

    return 0;
}
```

**编译**:
```bash
g++ -o myapp myapp.cpp -lhigplat -lpthread
```

---

## 性能指标

### 吞吐量
- **本地队列读写**: > 100,000 ops/sec
- **网络队列读写**: > 50,000 ops/sec
- **公告板读写**: > 200,000 ops/sec（并发访问不同 tag）

### 延迟
- **本地公告板读取**: < 1 μs（内存映射）
- **网络队列读写**: < 100 μs（局域网）
- **事件通知延迟**: < 1 ms

### 容量
- **单队列最大记录数**: 受限于文件大小和内存
- **单公告板最大标签数**: 7177
- **单数据库最大表数**: 7177
- **最大消息大小**: 128 KB

### 并发
- **单 Worker 最大连接数**: 可配置（建议 < 10000）
- **Worker 进程数**: 可配置（建议等于 CPU 核心数）
- **消息处理线程数**: 可配置（默认 4）

---

## 配置参数

### 服务端配置（nginx.conf）

```ini
[Socket]
ListenPort = 8777                      # 监听端口
WorkerProcesses = 1                    # Worker 进程数

[Proc]
Daemon = 1                             # 守护进程模式（0/1）
ProcMsgRecvWorkThreadCount = 4         # 消息处理线程数
```

### 队列创建参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| recordSize | 记录大小（字节） | 必填 |
| recordNum | 记录数量 | 必填 |
| dataType | 数据类型（ASCII/BINARY） | BINARY |
| operateMode | 操作模式（NORMAL/SHIFT） | NORMAL |

### 公告板创建参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| size | 公告板总大小（字节） | 必填 |

---

## 运维管理

### 监控

**查看进程状态**:
```bash
ps aux | grep gplat
# 输出示例:
# nginx: master process ./gplat
# nginx: worker process
```

**日志查看**:
```bash
tail -f error.log
```

### 维护

**重新加载配置**:
```bash
kill -HUP <master_pid>
```

**优雅关闭**:
```bash
kill -TERM <master_pid>
```

**快速关闭**:
```bash
kill -QUIT <master_pid>
```

### 备份与恢复

**备份数据**:
```bash
# 备份所有 QBD 文件
tar -czvf qbdfile_backup.tar.gz qbdfile/
```

**恢复数据**:
```bash
# 解压到原目录
tar -xzvf qbdfile_backup.tar.gz
```

---

## 故障排查

### 常见问题

**1. 连接失败**
```
原因: 服务端未启动或端口不匹配
解决:
- 检查进程: ps aux | grep gplat
- 检查端口: netstat -tlnp | grep 8777
- 检查配置: cat nginx.conf
```

**2. 数据写入失败**
```
原因: 队列/公告板未创建或已满
解决:
- 检查 QBD 文件: ls -lh qbdfile/
- 创建数据结构: ./createq / ./createb
- 检查日志: tail error.log
```

**3. 订阅无响应**
```
原因: 标签未创建或数据未变化
解决:
- 确认标签存在: createtag()
- 确认数据写入: writeb() 返回值
- 检查网络连接: socket 未断开
```

---

## 最佳实践

### 1. 数据结构选择

| 场景 | 推荐结构 | 原因 |
|------|---------|------|
| 日志收集 | Queue | 有序、FIFO |
| 实时状态 | Board | 快速读写、时间戳 |
| 历史记录 | Database | 结构化、批量查询 |
| 任务分发 | Queue (SHIFT) | 防止阻塞 |
| 配置参数 | Board | 多进程共享 |

### 2. 性能优化

- **批量操作**: 使用数据库表的批量插入/查询
- **避免频繁创建**: 预先创建所有队列和公告板
- **合理分片**: 大数据集拆分为多个队列/公告板
- **使用订阅**: 避免轮询，使用事件驱动

### 3. 可靠性保障

- **定期备份**: 定时备份 qbdfile 目录
- **错误检查**: 始终检查 API 返回值和 error 参数
- **心跳检测**: 客户端实现重连机制
- **日志监控**: 定期查看 error.log

---

## 许可与支持

### 开源许可
查看项目根目录的 LICENSE 文件

### 技术支持
- 文档: 查看 `Doc/` 和 `readme/` 目录
- 示例代码: 参考 `testapp/` 项目
- 工具: 使用 `Tool/` 目录下的实用工具

---

## 版本信息

当前版本: 参考项目构建配置
兼容性: Linux x86_64, ARM64
最低内核版本: 2.6.27
C++ 标准: C++17

---

## 总结

gPlat 是一个专为实时系统设计的 IPC 平台，核心优势在于:

1. **性能卓越**: 微秒级延迟 + 零拷贝技术
2. **可靠性高**: 数据持久化 + 主从架构
3. **使用简单**: 清晰的 C API + 丰富的工具
4. **灵活适配**: 三种数据结构满足多种场景

适用于工业自动化、边缘计算、分布式控制等对实时性和可靠性要求严格的应用场景。
