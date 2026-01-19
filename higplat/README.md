# higplat

本库在内存映射文件上实现了两类数据结构：Queue（简称 Q）与 Bulletin（简称 B），以及基于 B 的简单表式存储（Database）。统一用 Load 载入映射，再调用读写接口。

## 数据结构
- Queue：线性顺序存储，先进先出，文件内容依次为 `QUEUE_HEAD`、若干条 `RECORD_HEAD+payload` 记录、可选的类型描述区，读写指针循环前进。
- Bulletin：线性存储，文件内容为 `BOARD_HEAD`、数据区、类型区；`BOARD_INDEX_STRUCT` 数组位于头部，提供 item 名到数据偏移与类型偏移的哈希索引，按名称随机读写。
- Database：与 Bulletin 结构类似，头部为 `DB_HEAD` 与 `DB_INDEX_STRUCT` 数组，索引表名到表空间，表内记录定长顺序存储。
- 全局加载表：`TABLE_MSG table[TABLESIZE]` 持有已加载文件的句柄、映射地址与同步原语，供各接口快速定位。

## 数据类型
- `TABLE_MSG`：保存数据队列/公告板/数据库的名称、文件描述符、映射地址、同步互斥量、引用计数、文件大小。
- `QUEUE_HEAD`：队列元数据，包含类型标记 `qbdtype`、数据类型 `dataType`（ASCII/BINARY）、模式 `operateMode`（NORMAL/SHIFT）、记录数 `num`、单条长度 `size`、读写指针、创建时间、类型区大小。
- `RECORD_HEAD`：队列中每条记录的头，包含写入时间、远端 IP、确认标记、序号。
- `BOARD_INDEX_STRUCT`：Bulletin 索引项，含 item 名、数据起始偏移与长度、删除标记、写入时间、类型区偏移与长度。
- `BOARD_HEAD`：Bulletin 头部，含类型标记、写计数、总大小、类型区大小、下一个数据/类型写入位置、剩余空间、索引数量、索引数组与互斥组。
- `DB_INDEX_STRUCT`：Database 索引项，含表名、数据起始偏移、单条大小、最大/当前记录数、删除标记、写入时间、类型区信息。
- `DB_HEAD`：Database 头部，布局与 Bulletin 类似，记录总空间、类型空间、剩余空间、索引数量与互斥组。

## note

LoadQ 将指定名字的 lpDqName 通过 mmap 加载到内存中，在全局哈希表内找到一个位置，并获取在哈希表中的 metadata 区的对应 metadata 的引用。该区域保存文件描述符、内存映射地址、读写锁等。

mmap 地址 lpMapAddress，模式 MAP_PRIVATE，需要 msync 与文件同步。

QUEUE 排布：QUEUE_HEAD + (RECORD_HEAD + payload)*num

```C++
struct QUEUE_HEAD
{
	int  qbdtype;
	int  dataType;			// 数据队列的类型，1为ASCII型；0为BINARY型
	int  operateMode;		// 1为移位队列，不判断溢出；0为通用队列
	int  num;				// 记录数
	int  size;				// 记录大小
	int  readPoint;			// 读指针
	int  writePoint;		// 写指针
	char createDate[20];	// 创建日期
	int  typesize;			// 类型序列化长度
	int  reserved;
};


struct RECORD_HEAD
{
	char createDate[20];
	char remoteIp[16];
	int  ack;				// 确认标志 0未确认1已确认
	int  index;				// 位置索引（0开始）
	int  reserve;			// 预留
};
```

ReadQ

## 函数说明

```cpp
/**
 * @brief 返回最近一次库内错误码。
 * @return 错误码值。
 */
unsigned int GetLastErrorQ();

/**
 * @brief 求和测试函数。
 * @param a 被加数。
 * @param b 加数。
 * @return a+b。
 */
int add(int a, int b);

/**
 * @brief 插入加载记录到全局哈希表。
 * @param tabmsg 需插入的记录，包含名称与句柄。
 * @return 成功返回 true，表满或其他错误返回 false 并设置错误码。
 */
bool inserttab(const TABLE_MSG &tabmsg);

/**
 * @brief 查找加载表，命中后复制记录。
 * @param dqname 队列/公告板/数据库名称。
 * @param tabmsg 输出，填充找到的记录。
 * @return 命中返回 true，未找到返回 false 并设置错误码。
 */
bool fetchtab(const char *dqname, TABLE_MSG &tabmsg);

/**
 * @brief 查找加载表并递增引用计数。
 * @param dqname 队列名称。
 * @param tabmsg 输出记录。
 * @return 命中返回 true，否则返回 false。
 */
bool fetchtab1(const char *dqname, TABLE_MSG &tabmsg);

/**
 * @brief 从加载表删除记录，引用计数减一后为零时才真正删除。
 * @param dqname 队列名称。
 * @param tabmsg 输出被删除的记录。
 * @return 删除返回 true，若记录不存在返回 false 并设置错误码。
 */
bool deletetab(const char *dqname, TABLE_MSG &tabmsg);

/**
 * @brief 字符串哈希函数，提供索引初值。
 * @param s 键字符串。
 * @return 位置索引值。
 */
int hash1(const char *s);

/**
 * @brief 字符串哈希函数，提供再散列步长。
 * @param s 键字符串。
 * @return 步长值，非零。
 */
int hash2(const char *s);

/**
 * @brief 创建基于文件的队列。
 * @param lpFileName 队列文件名（不含路径，长度受限）。
 * @param recordSize 单条记录长度。
 * @param recordNum 记录条数。
 * @param dateType 数据类型，ASCII_TYPE 或 BINARY_TYPE。
 * @param operateMode 队列模式，NORMAL_MODE 或 SHIFT_MODE。
 * @param pType 可选类型描述数据指针。
 * @param typeSize 类型描述长度。
 * @return 成功返回 true，参数非法或文件操作失败返回 false 并设置错误码。
 */
bool CreateQ(const char *lpFileName, int recordSize, int recordNum, int dateType, int operateMode, void *pType, int typeSize);

/**
 * @brief 载入队列/公告板/数据库文件到映射，并登记全局表。
 * @param lpDqName 名称（用于查找文件及作为哈希键）。
 * @return 已加载返回 true 且错误码设为 ERROR_ALREADY_LOAD；首次成功加载返回 true；失败返回 false 并设置错误码。
 */
bool LoadQ(const char *lpDqName);

/**
 * @brief 卸载指定名称的映射，销毁互斥量并移出全局表。
 * @param lpDqName 名称。
 * @return 找到并处理返回 true，未找到也返回 true；失败时设置错误码。
 */
bool UnloadQ(const char *lpDqName);

/**
 * @brief 卸载当前进程加载的所有对象。
 * @return 始终返回 true。
 */
bool UnloadAll();

/**
 * @brief 将所有已映射对象的数据写回文件。
 * @return 始终返回 true，失败项会在标准错误输出提示。
 */
bool FlushQFile();

/**
 * @brief 读取队列头部。
 * @param lpDqName 队列名称。
 * @param lpHead 目标缓冲，长度需不少于 QUEUEHEADSIZE。
 * @return 成功返回 true，否则返回 false 并设置错误码。
 */
bool ReadHead(const char *lpDqName, void *lpHead);

/**
 * @brief 从队列读出一条记录并前移读指针。
 * @param lpDqName 队列名称。
 * @param lpRecord 目标缓冲。
 * @param actSize 调用方认为的长度，ASCII 模式可为 0 以使用实际字符串长度。
 * @param remoteIp 可选输出写入时记录的 IP。
 * @return 成功返回 true，长度不符或队列空返回 false 并设置错误码。
 */
bool ReadQ(const char *lpDqName, void *lpRecord, int actSize, char *remoteIp);

/**
 * @brief 仅移动队列读指针，丢弃一条记录。
 * @param lpDqName 队列名称。
 * @return 成功返回 true，队列空时返回 false 并设置错误码。
 */
bool PopJustRecordFromQueue(const char *lpDqName);

/**
 * @brief 批量读取多条记录到调用方提供的缓冲（不加锁）。
 * @param lpDqName 队列名称。
 * @param lpRecord 目标缓冲，需能容纳 count*(recordSize+RECORDHEADSIZE)。
 * @param start 起始位置。
 * @param count 输入为期望条数，输出为实际读取条数。
 * @param pRecordSize 可选输出单条长度。
 * @return 成功返回 true，起始位置越界返回 false 并设置错误码。
 */
bool MulReadQ(const char *lpDqName, void *lpRecord, int start, int *count, int *pRecordSize);

/**
 * @brief 批量读取多条记录到新分配的缓冲（内部加锁）。
 * @param lpDqName 队列名称。
 * @param lppRecords 输出缓冲指针，需由调用方释放。
 * @param start 起始位置。
 * @param count 输入期望条数，输出实际条数。
 * @param pRecordSize 输出单条长度。
 * @return 成功返回 true，失败返回 false 并清空输出参数。
 */
bool MulReadQ2(const char *lpDqName, void **lppRecords, int start, int *count, int *pRecordSize);

/**
 * @brief 写入一条记录到队列。
 * @param lpDqName 队列名称。
 * @param lpRecord 源数据。
 * @param actSize 数据长度，ASCII 模式为 0 时按字符串长度+1 计算。
 * @param remoteIp 写入远端 IP 标记，空指针则写入 127.0.0.1。
 * @return 成功返回 true，长度非法或队列满返回 false 并设置错误码。
 */
bool WriteQ(const char *lpDqName, void *lpRecord, int actSize, const char *remoteIp);

/**
 * @brief 清空队列，重置读写指针。
 * @param lpDqName 队列名称。
 * @return 成功返回 true。
 */
bool ClearQ(const char *lpDqName);

/**
 * @brief 设置队列读写指针。
 * @param lpDqName 队列名称。
 * @param readPtr 读指针位置，-1 表示保持不变。
 * @param writePtr 写指针位置，-1 表示保持不变。
 * @return 成功返回 true，位置越界返回 false。
 */
bool SetPtrQ(const char *lpDqName, int readPtr, int writePtr);

/**
 * @brief 查看下一条记录但不移动指针（仅 NORMAL_MODE）。
 * @param lpDqName 队列名称。
 * @param lpRecord 目标缓冲。
 * @param actSize 数据长度。
 * @return 成功返回 true，队列空或模式不支持返回 false 并设置错误码。
 */
bool PeekQ(const char *lpDqName, void *lpRecord, int actSize);

/**
 * @brief 判断队列是否为空。
 * @param lpDqName 队列名称。
 * @return 空返回 true，非空返回 false，查找失败返回 false 并设置错误码。
 */
bool IsEmptyQ(const char *lpDqName);

/**
 * @brief 判断队列是否已满（仅 NORMAL_MODE 判断写指针下一位是否为读指针）。
 * @param lpDqName 队列名称。
 * @return 满返回 true，未满返回 false，查找失败返回 false 并设置错误码。
 */
bool IsFullQ(const char *lpDqName);

/**
 * @brief 创建公告板文件。
 * @param lpFileName 公告板文件名。
 * @param size 数据区大小（不含头，类型区按平均大小预留）。
 * @return 成功返回 true，失败返回 false 并设置错误码。
 */
bool CreateB(const char *lpFileName, int size);

/**
 * @brief 读取公告板元信息及可选的 item 名列表。
 * @param lpBulletinName 公告板名称。
 * @param pTotalSize 输出总大小（头+数据）。
 * @param pDataSize 输出数据区大小。
 * @param pLeftSize 输出剩余空间。
 * @param pItemNum 输出有效 item 数。
 * @param buffSize 输入为名称缓存容量，输出为填充的名称数量。
 * @param ppBuff 名称缓存，二维字符数组，元素长度至少 24。
 * @return 成功返回 true，失败返回 false 并设置错误码。
 */
bool ReadInfoB(const char *lpBulletinName, int *pTotalSize, int *pDataSize, int *pLeftSize, int *pItemNum, int *buffSize, char ppBuff[][24]);

/**
 * @brief 按名称读取公告板 item（定长数据）。
 * @param lpBulletinName 公告板名称。
 * @param lpItemName item 名称。
 * @param lpItem 目标缓冲。
 * @param actSize 预期长度。
 * @param timestamp 可选输出写入时间。
 * @return 成功返回 true，未找到或长度不符返回 false 并设置错误码。
 */
bool ReadB(const char *lpBulletinName, const char *lpItemName, void *lpItem, int actSize, timespec *timestamp);

/**
 * @brief 按名称读取公告板字符串型 item，自动清零缓冲。
 * @param lpBulletinName 公告板名称。
 * @param lpItemName item 名称。
 * @param lpItem 目标缓冲。
 * @param actSize 缓冲容量。
 * @param timestamp 可选输出写入时间。
 * @return 成功返回 true，缓冲不足或未找到返回 false 并设置错误码。
 */
bool ReadB_String(const char *lpBulletinName, const char *lpItemName, void *lpItem, int actSize, timespec *timestamp);

/**
 * @brief 写入或部分更新公告板 item（定长数据）。
 * @param lpBulletinName 公告板名称。
 * @param lpItemName item 名称，必须已存在。
 * @param lpItem 原始完整数据指针。
 * @param actSize 数据总长度。
 * @param lpSubItem 若非空，表示仅写入该子区域。
 * @param actSubSize 子区域长度。
 * @return 成功返回 true，未找到或长度不符返回 false 并设置错误码。
 */
bool WriteB(const char *lpBulletinName, const char *lpItemName, void *lpItem, int actSize, void *lpSubItem, int actSubSize);

/**
 * @brief 写入公告板字符串型 item，长度必须不超过预留空间。
 * @param lpBulletinName 公告板名称。
 * @param lpItemName item 名称。
 * @param lpItem 字符串数据。
 * @param actSize 数据长度。
 * @param lpSubItem 不支持，须为 nullptr。
 * @param actSubSize 不支持。
 * @return 成功返回 true，字符串过长或未找到返回 false 并设置错误码。
 */
bool WriteB_String(const char *lpBulletinName, const char *lpItemName, void *lpItem, int actSize, void *lpSubItem, int actSubSize);

/**
 * @brief 以偏移方式部分写入公告板 item。
 * @param lpBulletinName 公告板名称。
 * @param lpItemName item 名称。
 * @param lpItem 原始数据指针。
 * @param actSize 数据总长度。
 * @param offSet 子区域起始偏移。
 * @param actSubSize 子区域长度。
 * @return 成功返回 true，失败返回 false 并设置错误码。
 */
bool WriteBOffSet(const char *lpBulletinName, const char *lpItemName, void *lpItem, int actSize, int offSet, int actSubSize);

/**
 * @brief 清空公告板，重置头部与索引。
 * @param lpBoardName 公告板名称。
 * @return 成功返回 true。
 */
bool ClearB(const char *lpBoardName);

/**
 * @brief 删除公告板 item，并紧缩数据与类型区偏移。
 * @param lpBoardName 公告板名称。
 * @param lpItemName item 名称。
 * @return 成功返回 true，未找到返回 false 并设置错误码。
 */
bool DeleteItem(const char *lpBoardName, const char *lpItemName);

/**
 * @brief 在数据库中创建表空间。
 * @param lpDBName 数据库名称。
 * @param lpTableName 表名。
 * @param recordSize 单条记录长度。
 * @param maxCount 最大记录数。
 * @param pType 可选类型描述。
 * @param typeSize 类型描述长度。
 * @return 成功返回 true，空间不足或表已存在返回 false 并设置错误码。
 */
bool CreateTable(const char *lpDBName, const char *lpTableName, int recordSize, int maxCount, void *pType, int typeSize);

/**
 * @brief 读取数据库头部。
 * @param lpDBName 数据库名称。
 * @param lpHead 目标缓冲，长度不少于 sizeof(DB_HEAD)。
 * @return 成功返回 true，否则返回 false 并设置错误码。
 */
bool ReadHeadDB(const char *lpDBName, void *lpHead);

/**
 * @brief 将指定表的当前记录数清零。
 * @param lpDBName 数据库名称。
 * @param lpTableName 表名。
 * @return 成功返回 true，表不存在返回 false 并设置错误码。
 */
bool ClearTable(const char *lpDBName, const char *lpTableName);

/**
 * @brief 按行号更新表中一条记录。
 * @param lpDBName 数据库名称。
 * @param lpTableName 表名。
 * @param rowid 行号（从 0 起）。
 * @param recordSize 记录长度，应与表定义一致。
 * @param lpRecord 新数据。
 * @return 成功返回 true，边界或长度错误返回 false 并设置错误码。
 */
bool UpdateTable(const char *lpDBName, const char *lpTableName, int rowid, int recordSize, void *lpRecord);

/**
 * @brief 追加多条记录到表尾。
 * @param lpDBName 数据库名称。
 * @param lpTableName 表名。
 * @param count 条数。
 * @param recordSize 单条长度。
 * @param lpRecords 数据块。
 * @return 成功返回 true，空间不足或长度错误返回 false 并设置错误码。
 */
bool InsertTable(const char *lpDBName, const char *lpTableName, int count, int recordSize, void *lpRecords);

/**
 * @brief 清空表后写入新数据集。
 * @param lpDBName 数据库名称。
 * @param lpTableName 表名。
 * @param count 条数。
 * @param recordSize 单条长度。
 * @param lpRecords 数据块。
 * @return 成功返回 true，失败返回 false 并设置错误码。
 */
bool RefreshTable(const char *lpDBName, const char *lpTableName, int count, int recordSize, void *lpRecords);

/**
 * @brief 选择表数据，复制到新分配缓冲。
 * @param lpDBName 数据库名称。
 * @param lpTableName 表名。
 * @param lppRecords 输出数据指针，需由调用方释放。
 * @param pCount 输出记录数。
 * @param pRecordSize 输出单条长度。
 * @param pWriteTime 可选输出最近写入时间。
 * @return 成功返回 true，表不存在返回 false 并设置错误码。
 */
bool SelectTable(const char *lpDBName, const char *lpTableName, void **lppRecords, int *pCount, int *pRecordSize, timespec *pWriteTime);

/**
 * @brief 删除整张表，紧缩数据区与类型区。
 * @param lpDBName 数据库名称。
 * @param lpTableName 表名。
 * @return 成功返回 true，表不存在返回 false 并设置错误码。
 */
bool DeleteTable(const char *lpDBName, const char *lpTableName);

/**
 * @brief 清空数据库，重置头部与索引。
 * @param lpDbName 数据库名称。
 * @return 成功返回 true。
 */
bool ClearDB(const char *lpDbName);
```
