# s7ioserver 实现计划

## Context

s7ioserver 是一个独立的 Linux 进程，用于桥接西门子 S7 PLC 和 gPlat Board（共享内存键值存储）。gPlat 服务端和客户端的 PLC 基础设施已完整实现（`HandleRegisterPlcServer`、`HandleWriteBPlc`、`NotifyPlcIoSever`、`registertag`、`waitpostdata`、`write_plc_*` 等）。当前 s7ioserver 仅有骨架代码：`main.cpp` 有线程结构但有 bug，`threadWritePlc.cpp` 有部分 gPlat 连接逻辑，`threadReadPlc.cpp` 完全为空，没有配置解析器，snap7 未链接。

本计划的目标是完成 s7ioserver 的完整实现。

---

## 设计决策摘要

| 决策项 | 选择 |
|---|---|
| 配置文件格式 | INI 风格，单文件 |
| 多 PLC 支持 | 支持多 PLC |
| 读取策略 | 按区域合并读取 |
| BOOL 地址表示 | offset.bit 格式 |
| Tag 创建 | 手动预创建（s7ioserver 不创建 tag） |
| 轮询间隔 | 每 PLC 可配 |
| 错误恢复 | 自动重连 |
| 线程模型 | 每 PLC 一个读线程 + 一个共享写线程 |
| 配置解析器 | 全新解析器（CConfig 不支持 section） |
| S7 String | 第一版即支持 |
| Board 使用 | 所有 PLC 的 tag 写入同一个 Board |

---

## 配置文件格式 (`s7ioserver.ini`)

```ini
[general]
gplat_server = 127.0.0.1
gplat_port = 8777
board_name = BOARD
reconnect_interval = 3000

[PLC_Line1]
ip = 192.168.1.10
rack = 0
slot = 1
poll_interval = 200

# tag定义格式: tag.名称 = 区域, DB号, 偏移量, 数据类型 [, 最大长度]
# 区域: DB, I, Q, M
# 数据类型: BOOL, INT, WORD, DINT, DWORD, REAL, STRING
# 偏移量: 字节.位 (位仅用于BOOL)
tag.motor_run      = DB, 1, 0.0,  BOOL
tag.motor_speed    = DB, 1, 2,    INT
tag.temperature    = DB, 1, 4,    REAL
tag.alarm_word     = DB, 1, 8,    WORD
tag.counter        = DB, 1, 10,   DINT
tag.status_dw      = DB, 1, 14,   DWORD
tag.recipe_name    = DB, 1, 18,   STRING, 32
tag.input_0_0      = I,  0, 0.0,  BOOL
tag.output_word    = Q,  0, 0,    WORD
tag.merker_real    = M,  0, 100,  REAL

[PLC_Line2]
ip = 192.168.1.11
rack = 0
slot = 1
poll_interval = 500

tag.pressure       = DB, 100, 0,   REAL
tag.valve_open     = DB, 100, 4.0, BOOL
```

- 以 `tag.` 为前缀的键是 tag 定义，其余为 PLC 参数
- Board 中的 tag 名称为 `tag.` 后面的部分
- 所有 PLC 的 tag 写入同一个 Board，tag 名称必须全局唯一

---

## 数据结构 (`s7config.h`)

```cpp
enum class S7DataType { BOOL, INT, WORD, DINT, DWORD, REAL, STRING };
enum class S7AreaType { DB, I, Q, M };

struct TagConfig {
    std::string tagname;      // Board中的tag名
    S7AreaType  area;
    int         dbnumber;     // I/Q/M时为0
    int         byte_offset;
    int         bit_offset;   // 0-7，仅BOOL使用
    S7DataType  datatype;
    int         maxlen;       // 仅STRING用，默认254
    int         byte_size;    // PLC中占用字节数（计算值）
    std::vector<uint8_t> last_raw_value; // 变化检测用
    bool        first_read;
};

struct ReadGroup {
    S7AreaType  area;
    int         dbnumber;
    int         start_byte;
    int         total_bytes;
    std::vector<uint8_t> buffer;
    std::vector<TagConfig*> tags;
};

struct PlcConfig {
    std::string name;         // section名 如 "PLC_Line1"
    std::string ip;
    int         rack;
    int         slot;
    int         poll_interval; // ms
    std::vector<TagConfig> tags;
    std::vector<ReadGroup> read_groups; // 由tags计算
};

struct AppConfig {
    std::string gplat_server;
    int         gplat_port;
    std::string board_name;
    int         reconnect_interval; // ms
    std::vector<PlcConfig> plcs;
};
```

`byte_size` 计算：BOOL=1, INT/WORD=2, DINT/DWORD/REAL=4, STRING=maxlen+2

---

## 实现步骤

### 步骤 1: 配置解析器 (`s7config.h` + `s7config.cpp` - 新建)

- 全新 INI 解析器（不复用 CConfig，因其不支持 section）
- 逐行读取，识别 `[section]`、`key = value`、注释行
- `[general]` 解析全局参数
- `[PLC_xxx]` 解析 PLC 连接参数 + `tag.` 前缀的 tag 定义
- tag 值按逗号分割：`area, dbnumber, offset[.bit], datatype[, maxlen]`
- `BuildReadGroups()`: 按 (area, dbnumber) 分组 → 按 byte_offset 排序 → 间隔 ≤ 32 字节的 tag 合并为一个 ReadGroup
- 验证 tag 名称全局唯一，报错退出

### 步骤 2: 读 PLC 线程 (`threadReadPlc.cpp` - 重写)

函数签名：`void threadReadPlc(PlcConfig* plc, AppConfig* config)`

每个 PLC 一个读线程，流程：
1. 连接 gPlat (`connectgplat`)
2. 创建 snap7 客户端，连接 PLC (`Cli_Create` + `Cli_ConnectTo`)
3. 主循环：
   - 对每个 ReadGroup 调用 `Cli_ReadArea` 读取一块数据
   - 对每个 tag 从 buffer 中提取原始字节，与 `last_raw_value` 比较
   - 若数据变化或首次读取：
     - **字节序转换**（大端→小端）：16位用 `__builtin_bswap16`，32位用 `__builtin_bswap32`
     - BOOL：提取字节中的指定位，存为 `bool`
     - STRING：提取 S7 格式（byte0=maxlen, byte1=actuallen, byte2+=data），转为 C 字符串
     - 调用 `writeb()` 或 `writeb_string()` 写入 Board
   - `usleep(poll_interval * 1000)` 等待
4. 连接丢失时自动重连（snap7 和 gPlat 各自重连）

### 步骤 3: 写 PLC 线程 (`threadWritePlc.cpp` - 重写)

函数签名：`void threadWritePlc(AppConfig* config)`

所有 PLC 共用一个写线程：
1. 连接 gPlat，订阅 `timer_500ms`（用于退出检测）
2. 为每个 PLC 创建独立的 snap7 客户端并连接
3. 构建 `std::map<string, TagLookup>` 映射：tag名 → {TagConfig*, PlcConfig*, S7Object}
4. 对所有 tag 调用 `registertag()` 注册
5. 主循环：
   - `waitpostdata()` 阻塞等待写请求
   - 收到 `timer_500ms` 时 continue
   - 查找 tag → 确定目标 PLC + 地址信息
   - 值转换（小端→大端）
   - BOOL：**读-改-写**（先 `Cli_ReadArea` 读整个字节，修改位，再 `Cli_WriteArea` 写回）
   - STRING：构建 S7 格式后 `Cli_WriteArea`
   - 其他类型：字节序交换后 `Cli_WriteArea`
6. snap7/gPlat 断开时自动重连；gPlat 重连后需重新 registertag + subscribe

### 步骤 4: 主程序 (`main.cpp` - 重写)

- 加载配置文件（路径可通过命令行参数指定，默认 `s7ioserver.ini`）
- 注册 SIGINT/SIGTERM 信号处理（设 `g_running = false`）
- 每个 PLC 启动一个读线程
- 启动一个共享写线程
- 主线程等待 `q` 命令或信号
- 设 `g_running = false` 后 join 所有线程
- 修复原 bug：添加 `threadReadPlc` 前向声明

### 步骤 5: 项目文件 (`s7ioserver.vcxproj` - 修改)

- 源文件更新：移除 `threadfunction.cpp`，添加 `threadReadPlc.cpp`、`threadWritePlc.cpp`、`s7config.cpp`、`../snap7.demo.cpp/snap7.cpp`
- 头文件添加：`s7config.h`、`../snap7.demo.cpp/snap7.h`
- C++ 标准：`c++11` → `c++17`
- 链接库：添加 `snap7`（现有仅 `higplat`）

### 步骤 6: 示例配置文件 (`s7ioserver.ini` - 新建)

创建包含注释说明的示例配置文件。

---

## 关键技术细节

### 字节序转换

PLC 使用大端（Big-Endian），Linux x86_64 使用小端（Little-Endian）。

```cpp
// 16位值 (INT, WORD)
uint16_t swap16(uint16_t val) { return __builtin_bswap16(val); }

// 32位值 (DINT, DWORD, REAL)
uint32_t swap32(uint32_t val) { return __builtin_bswap32(val); }

// REAL (float) - 交换4个原始字节后重新解释为float
float readReal(const uint8_t* buf) {
    uint32_t raw;
    memcpy(&raw, buf, 4);
    raw = swap32(raw);
    float result;
    memcpy(&result, &raw, 4);
    return result;
}
```

### BOOL 处理

**读方向（PLC → Board）：**
```cpp
bool extractBool(const uint8_t* buf, int byteOffset, int groupStart, int bitOffset) {
    uint8_t byte_val = buf[byteOffset - groupStart];
    return (byte_val >> bitOffset) & 0x01;
}
// Board中存为1字节 bool (sizeof(bool) == 1)
```

**写方向（Board → PLC）：读-改-写**
```cpp
void writeBoolToPLC(S7Object client, int area, int dbnumber,
                    int byteOffset, int bitOffset, bool value) {
    uint8_t currentByte;
    Cli_ReadArea(client, area, dbnumber, byteOffset, 1, S7WLByte, &currentByte);
    if (value) currentByte |= (1 << bitOffset);
    else       currentByte &= ~(1 << bitOffset);
    Cli_WriteArea(client, area, dbnumber, byteOffset, 1, S7WLByte, &currentByte);
}
```

单写线程设计保证同一字节不会被并发修改。

### S7 String 处理

S7 String 内存布局：byte0=最大长度, byte1=实际长度, byte2+=字符数据

**读方向：**
```cpp
void extractS7String(const uint8_t* raw, int maxlen, char* out, int outBufSize) {
    uint8_t actual_len = raw[1];
    if (actual_len > raw[0]) actual_len = raw[0]; // 防护
    int copy_len = (actual_len < outBufSize - 1) ? actual_len : outBufSize - 1;
    memcpy(out, raw + 2, copy_len);
    out[copy_len] = '\0';
}
// 转为C字符串后用 writeb_string() 写入Board
```

**写方向：**
```cpp
void buildS7String(const char* str, int maxlen, uint8_t* out) {
    int actual_len = strlen(str);
    if (actual_len > maxlen) actual_len = maxlen;
    out[0] = (uint8_t)maxlen;
    out[1] = (uint8_t)actual_len;
    memcpy(out + 2, str, actual_len);
    if (actual_len < maxlen) memset(out + 2 + actual_len, 0, maxlen - actual_len);
}
// 然后 Cli_WriteArea(..., byteOffset, maxlen+2, S7WLByte, out)
```

### 区域合并读取 (BuildReadGroups)

1. 按 (area, dbnumber) 分组所有 tag
2. 每组内按 byte_offset 升序排列
3. 遍历 tag，间隔 ≤ GAP_THRESHOLD (32字节) 的合并为同一 ReadGroup
4. ReadGroup.total_bytes = max(tag.byte_offset + tag.byte_size) - start_byte
5. 每个 ReadGroup 一次 `Cli_ReadArea` 调用

### 自动重连

**snap7 重连：**
```cpp
bool reconnectSnap7(S7Object client, const PlcConfig& plc, int interval) {
    Cli_Disconnect(client);
    while (g_running) {
        printf("[%s] snap7 reconnecting to %s...\n", plc.name.c_str(), plc.ip.c_str());
        if (Cli_ConnectTo(client, plc.ip.c_str(), plc.rack, plc.slot) == 0) return true;
        // 分段 sleep 以便响应 g_running 变化
        for (int i = 0; i < interval / 100 && g_running; i++)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}
```

**gPlat 重连：** 类似模式，reconnect 后写线程需重新 `registertag` + `subscribe`。

**连接丢失检测：** `Cli_ReadArea`/`Cli_WriteArea` 返回非零时检查 `Cli_GetConnected`，未连接则触发重连。`writeb`/`registertag` 失败时检查 socket 状态。

---

## 需修改/新建的文件

| 文件 | 操作 |
|---|---|
| `s7ioserver/s7config.h` | 新建 - 数据结构 + LoadConfig 声明 |
| `s7ioserver/s7config.cpp` | 新建 - INI 解析器 + BuildReadGroups |
| `s7ioserver/main.cpp` | 重写 - 多 PLC 线程管理 |
| `s7ioserver/threadReadPlc.cpp` | 重写 - 完整读 PLC 实现 |
| `s7ioserver/threadWritePlc.cpp` | 重写 - 完整写 PLC 实现 |
| `s7ioserver/s7ioserver.vcxproj` | 修改 - 源文件/链接库/C++标准 |
| `s7ioserver/s7ioserver.ini` | 新建 - 示例配置文件 |

参考文件（只读）：
- `snap7.demo.cpp/snap7.h` - snap7 API 定义
- `snap7.demo.cpp/snap7.cpp` - snap7 C 封装
- `include/higplat.h` - gPlat 客户端 API
- `include/msg.h` - 协议定义

---

## 边界情况与注意事项

1. **同一字节内多个 BOOL tag**：字节变化时需检查该字节内所有 BOOL tag 并逐个更新
2. **S7 String 的 actual_len > maxlen**：需防御性 clamp
3. **Tag 名称冲突**：配置解析时校验全局唯一性
4. **Board tag 大小**：Board 中 tag 须预创建正确大小（BOOL=1, INT/WORD=2, DINT/DWORD/REAL=4, STRING 按需）
5. **每线程独立连接**：每个读线程和写线程各自独立的 gPlat socket 和 snap7 S7Object
6. **启动时 PLC 离线**：初始连接失败时直接进入重连循环
7. **waitpostdata 阻塞与退出**：订阅 `timer_500ms` 定期唤醒以检查 `g_running`
8. **内存对齐**：多字节值用 `memcpy` 提取，避免指针强转导致的对齐问题

---

## 验证方法

1. **编译验证**：在 VS2022 中编译 s7ioserver 项目，确保无编译错误
2. **配置解析测试**：启动 s7ioserver 加载示例配置，通过 printf 输出确认解析正确
3. **PLC 读取测试**：连接真实 PLC 或 Snap7 Server 模拟器，观察 Board 中 tag 值更新
4. **PLC 写入测试**：客户端调用 `write_plc_int` 等函数，观察 PLC 对应地址写入
5. **断线重连测试**：运行中断开 PLC 网络，确认自动重连后恢复正常
6. **多 PLC 测试**：配置多个 PLC，验证各 PLC 独立轮询和写入
