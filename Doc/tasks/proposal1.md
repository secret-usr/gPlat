## 增加PLC数据读写功能

### 需求描述: 

- 项目@s7ioserver负责从西门子PLC里读写数据
- 西门子PLC数据类型包括:
· BOOL(BITS)
· WORD (unsigned 16 bit integer)
· INT (signed 16 bit integer)
· DWORD (unsigned 32 bit integer)
· DINT (signed 32 bit integer)
· REAL (32 bit floating point number)
· S7 String
- 西门字数据块类型有：
· DB：Data Block (数据块)
· EB：Inputs (输入，即英标的 I)
· AB：Outputs (输出，即英标的 Q)
· MK：Merkers (中间寄存器，即英标的 M)
- s7ioserver通过snap7通信库访问西门子PLC,项目@snap7是snap7通信库的源代码，项目@snap7.demo.cpp是snap7通信库的示例程序
- 通过配置文件定义要从西门子PLC里读取的每个变量的属性，包括自定义变量名、在PLC的哪个块区(DB/I/Q/M),起始字节偏移量,大小（由变量的数据类型决定）
- 所有要读取的变量事先通过createtag函数在内存QBD的BOARD里创建好对应的tag
- s7ioserver启动后读取配置文件，然后轮询读取所有的变量值，当数据改变时通过writeb函数写入BOARD
- 客户端通过write_plc_bool，write_plc_short，write_plc_ushort，write_plc_int，write_plc_uint，write_plc_float，write_plc_string函数写PLC
- s7ioserver通过registertag和waitpostdata函数注册和监听客户端写PLC事件，然后根据tag名和值调用snap7通信库写PLC

### 场景 1: 

```
           ┌─────────────┐      ┌─────────────┐       ┌─────────────┐
           │  PLC Driver │      │   gPlat     │       │   Client    │
PLC ─────> │             ├─────>│   Server    │<─────>│             │
    读取   │ writeb()    │ 写入 │             │ 读取   │  subscribe()│
           └─────────────┘ 数据 │    Board    │ 订阅   └─────────────┘
                                │             │ 事件
                                └─────────────┘
```

### 场景 2: 

```
           ┌────────────────┐      ┌─────────────┐       ┌─────────────┐
           │  PLC Driver    │      │   gPlat     │       │   Client    │
PLC <───── │ registertag()  │<─────│   Server    │<──────│             │
     写入  │  waitpostdata()│ 写入  │             │ 写PLC │write_plc_*()│
           └────────────────┘ 数据 │    Board    │       └─────────────┘
                                   └─────────────┘
```