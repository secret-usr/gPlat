
1.  Main Thread
    1. 管理程序生命周期
    2. 输入 `q` 将设置全局标志 `g_running = false`，通知所有子线程安全退出，并使用 `join` 等待它们结束。

2.  订阅/接收者线程 `threadFunction1`
    1. 作为数据消费者连接到位于 127.0.0.1:8777 的本地 gPlat 服务。
    2. 订阅事件 with tagname `int1`, `string1`
    3. 订阅定时器 with tagname `timer_500ms`, `timer_1s`, `timer_2s`, `timer_3s`, `timer_5s`。
    4. 使用 `waitpostdata` 阻塞等待事件。
    5. 收到事件后，根据事件名调用 `readb` (读取整数) 或 `readb_string2` (读取字符串) 获取数据并打印。

3.  整数发生器 `threadFunction2`
    1. 模拟低频数据生产。
    2. 每隔 2s 递增一个整数计数器，并使用 `writeb` 将数据发布到 `int1` tagname。

4. 字符串发生器 `threadFunction3`
    1. 模拟高频大量数据生产。
    2. 每隔 50ms 构造一个包含计数后缀的长字符串 ("hello world, gyb 777... loop=N")。
    3. 使用 `writeb_string` 将数据发布到 `string1` tagname。
    4. 随后立即尝试回读验证。

## 依赖

gPlat 的服务端程序已启动并在 `127.0.0.1:8777` 监听。

```bash
cd bin/
./TestApplication
```

服务器的 `threadfunction.cpp` 负责具体的工作线程逻辑实现。

服务器应用层协议实现：`gplat/ngx_c_slogic.h`，入口函数 `threadRecvProcFunc` 作为路由，从 nginx 模块接收完整的消息报文，根据报文头的 ID 分发到各个处理函数进行业务逻辑处理。

服务器的订阅机制实现：`std::map<std::string, std::list<EventNode>> m_mapSubject`，key 为主题名 (tagname)，value 为订阅该主题的事件节点链表 (EventNode 包含事件句柄和回调函数指针)。 

```cpp
struct EventNode  
{  
    void* subscriber;       // 连接对象 (lpngx_connection_t)，订阅者
    char eventname[40];     // 触发后通知给客户端的事件名
    EVENTID eventid;        // 订阅类型，立刻触发(DEFAULT)/延时触发(POST_DELAY)
    int eventarg;           // 额外参数，对于延时订阅，这里存的是延迟毫秒数
};
```

客户端调用 `subscribe` 函数时，会向服务器发送订阅请求，服务器则在对连接加锁后调用 `Attach(tagname, observer)` 将新的 EventNode 添加到对应主题的链表中。

在客户端调用 `writeb` 进行写操作的时候（对应服务器 `HandleWriteB` 函数），服务器会查找该 tagname 的订阅者列表，并遍历调用每个订阅者的事件通知函数，将数据推送给它们（DEFAULT）或等待指定时间后再推送（POST_DELAY）。

当连接断开时，在清理连接资源的过程中，服务器会调用 `Detach` 函数，将该连接对应的所有订阅节点从各个主题链表中移除，确保不会再向已断开的客户端发送数据。