# Board 自定义类型 Tag：编译期反射 + 字段级显示

## 1. 需求概述

当前创建自定义类型 tag 需要手动指定字节大小：

```
create mytagname MyTagStruct$100
```

目标：

```
create mytagname MyTagStruct          # 自动获取 sizeof
create mytagname MyTagStruct 10       # 自动获取 sizeof, 数组长度 10
create mytagname Unknown$100          # 保留 $size 回退
select mytagname                      # 字段级格式化输出
```

---

## 2. 技术分析

### 核心约束

C++ `sizeof` 是编译期运算符，无法通过字符串在运行时调用。因此需要：

1. **包含头文件** — 编译器需要看到 struct 定义才能计算 sizeof
2. **编译期注册机制** — 将类型名字符串映射到 sizeof 结果 + 字段元数据

### 设计决策（用户已确认）

| 决策项 | 选择 |
|---|---|
| 注册方案 | 编译期注册表（宏辅助） |
| 头文件位置 | `include/` 下，多组件共享 |
| 字段描述方式 | `BEGIN_STRUCT / FIELD / END_STRUCT` 宏 |
| select 显示 | 字段级格式化输出 |
| 命令语法 | 优先注册表查询，回退到 `$size` |
| 元数据传输 | 仅客户端本地解释，服务端协议不变 |

---

## 3. 宏反射系统设计

### 3.1 用户头文件示例 (`include/user_types.h`)

用户使用 `BEGIN_STRUCT / FIELD / END_STRUCT` 宏同时定义 C++ struct 和元数据：

```cpp
// include/user_types.h
#ifndef USER_TYPES_H_
#define USER_TYPES_H_

#include "struct_reflect.h"

BEGIN_STRUCT(SensorData)
    FIELD(Int32,   temperature)
    FIELD(Int32,   humidity)
    FIELD(Double,  pressure)
    FIELD(Boolean, alarm)
END_STRUCT(SensorData)

BEGIN_STRUCT(MotorStatus)
    FIELD(Single,  speed)
    FIELD(Single,  current)
    FIELD(Int32,   error_code)
    FIELD(UInt32,  run_count)
END_STRUCT(MotorStatus)

#endif // USER_TYPES_H_
```

### 3.2 宏展开效果

`BEGIN_STRUCT(SensorData) ... END_STRUCT(SensorData)` 展开后等价于：

```cpp
// 1. 实际的 C++ struct（可以正常 #include 和使用）
#pragma pack(push, 8)
struct SensorData {
    int32_t   temperature;
    int32_t   humidity;
    double    pressure;
    bool      alarm;
};
#pragma pack(pop)

// 2. 静态的字段描述信息（编译期生成的元数据）
// 通过 GetStructInfo<SensorData>() 或 FindStructByName("SensorData") 可获取
```

### 3.3 反射基础设施 (`include/struct_reflect.h`)

这是整个方案的核心头文件，提供宏定义和反射查询 API。

#### 数据结构

```cpp
// 单个字段的描述信息
struct FieldInfo {
    const char* name;       // 字段名（如 "temperature"）
    TypeCode    type;       // 类型码（复用 type_handle.h 中的 TypeCode 枚举）
    int         size;       // 字段大小（字节）
    int         offset;     // 相对 struct 起始的偏移量
};

// 一个 struct 的完整描述
struct StructInfo {
    const char*       name;         // struct 名（如 "SensorData"）
    int               total_size;   // sizeof(struct)
    int               field_count;  // 字段数量
    const FieldInfo*  fields;       // 字段数组指针
};
```

#### 宏定义（核心）

采用两阶段展开策略：第一阶段生成 struct 定义，第二阶段生成 FieldInfo 数组。

```cpp
// TypeCode 到 C++ 类型的映射（用于 FIELD 宏中的类型声明）
// 这些宏将 TypeCode 枚举名映射为对应的 C++ 原生类型
#define CTYPE_Boolean   bool
#define CTYPE_Char      char
#define CTYPE_Int16     int16_t
#define CTYPE_UInt16    uint16_t
#define CTYPE_Int32     int32_t
#define CTYPE_UInt32    uint32_t
#define CTYPE_Int64     int64_t
#define CTYPE_UInt64    uint64_t
#define CTYPE_Single    float
#define CTYPE_Double    double

// 数组字段的类型声明辅助宏
// FIELD_ARRAY(Char, name, 32) 展开为声明 char name[32]
#define CTYPE_ARRAY(typecode, count) CTYPE_##typecode

#define BEGIN_STRUCT(NAME) \
    _Pragma("pack(push, 8)") \
    struct NAME {

#define FIELD(TYPECODE, FIELDNAME) \
    CTYPE_##TYPECODE FIELDNAME;

#define FIELD_ARRAY(TYPECODE, FIELDNAME, COUNT) \
    CTYPE_##TYPECODE FIELDNAME[COUNT];

#define END_STRUCT(NAME) \
    }; \
    _Pragma("pack(pop)") \
    static_assert(std::is_trivially_copyable_v<NAME>, \
        #NAME " must be trivially copyable for Board storage"); \
    /* 字段描述数组 — 需要第二阶段宏来生成，见下文 */
```

**难点：一套宏同时生成 struct 定义和 FieldInfo 数组**

标准 C++ 宏无法在一次展开中同时做两件事（定义 struct 成员 + 记录 FieldInfo）。有两种解决方法：

#### 方案 3.3-A：X-Macro 技术（推荐）

用户用一个 `FIELDS` 宏列表定义字段，然后展开两次：

```cpp
// include/user_types.h

// 第一步：用 X-Macro 列出字段
#define SENSORDATA_FIELDS \
    X(Int32,   temperature) \
    X(Int32,   humidity) \
    X(Double,  pressure) \
    X(Boolean, alarm)

// 第二步：生成 struct 定义
#pragma pack(push, 8)
struct SensorData {
    #define X(TYPE, NAME) CTYPE_##TYPE NAME;
    SENSORDATA_FIELDS
    #undef X
};
#pragma pack(pop)
static_assert(std::is_trivially_copyable_v<SensorData>,
    "SensorData must be trivially copyable");

// 第三步：生成 FieldInfo 数组
inline const StructInfo& GetStructInfo_SensorData() {
    static const FieldInfo fields[] = {
        #define X(TYPE, NAME) { #NAME, TYPE, (int)sizeof(CTYPE_##TYPE), (int)offsetof(SensorData, NAME) },
        SENSORDATA_FIELDS
        #undef X
    };
    static const StructInfo info = {
        "SensorData",
        (int)sizeof(SensorData),
        (int)(sizeof(fields) / sizeof(fields[0])),
        fields
    };
    return info;
}
```

**简化：提供封装宏减少模板代码**

```cpp
// include/struct_reflect.h 中提供的便捷宏

#define DEFINE_STRUCT(NAME, FIELDS_MACRO) \
    _Pragma("pack(push, 8)") \
    struct NAME { \
        FIELDS_MACRO(STRUCT_FIELD_DEF_) \
    }; \
    _Pragma("pack(pop)") \
    static_assert(std::is_trivially_copyable_v<NAME>, \
        #NAME " must be trivially copyable"); \
    inline const StructInfo& GetStructInfo_##NAME() { \
        static const FieldInfo fields[] = { \
            FIELDS_MACRO(STRUCT_FIELD_INFO_, NAME) \
        }; \
        static const StructInfo info = { \
            #NAME, (int)sizeof(NAME), \
            (int)(sizeof(fields)/sizeof(fields[0])), fields \
        }; \
        return info; \
    }

// 内部辅助宏
#define STRUCT_FIELD_DEF_(TYPE, NAME, ...) CTYPE_##TYPE NAME __VA_ARGS__;
#define STRUCT_FIELD_INFO_(TYPE, NAME, STRUCT, ...) \
    { #NAME, TYPE, (int)sizeof(CTYPE_##TYPE), (int)offsetof(STRUCT, NAME) },
```

#### 方案 3.3-B：两阶段手写（更简单但更冗余）

如果宏过于复杂，退而求其次：用户正常写 struct，再单独写注册：

```cpp
// include/user_types.h

#pragma pack(push, 8)
struct SensorData {
    int32_t temperature;
    int32_t humidity;
    double  pressure;
    bool    alarm;
};
#pragma pack(pop)

// 注册（紧跟 struct 定义之后）
REGISTER_STRUCT(SensorData,
    FIELD_DESC(Int32,   SensorData, temperature),
    FIELD_DESC(Int32,   SensorData, humidity),
    FIELD_DESC(Double,  SensorData, pressure),
    FIELD_DESC(Boolean, SensorData, alarm)
)
```

其中：

```cpp
// include/struct_reflect.h

#define FIELD_DESC(TYPE, STRUCT, NAME) \
    { #NAME, TYPE, (int)sizeof(CTYPE_##TYPE), (int)offsetof(STRUCT, NAME) }

#define REGISTER_STRUCT(NAME, ...) \
    static_assert(std::is_trivially_copyable_v<NAME>, \
        #NAME " must be trivially copyable"); \
    inline const StructInfo& GetStructInfo_##NAME() { \
        static const FieldInfo fields[] = { __VA_ARGS__ }; \
        static const StructInfo info = { \
            #NAME, (int)sizeof(NAME), \
            (int)(sizeof(fields)/sizeof(fields[0])), fields \
        }; \
        return info; \
    }
```

**方案对比**

| | X-Macro（A）| 两阶段（B）|
|---|---|---|
| 字段只写一次 | 是 | 否（struct + 注册各写一次）|
| 宏可读性 | 稍差 | 好 |
| 字段一致性风险 | 无 | 有（两处可能不一致）|
| 实现复杂度 | 中 | 低 |

**建议先实现方案 B**（两阶段手写），以简单为优先。若后续字段频繁变动导致一致性问题，再迁移到方案 A。

---

## 4. 全局类型注册表

### 4.1 注册表头文件 (`include/struct_registry.h`)

将所有已注册的 struct 汇总到一个可按名称查询的注册表中：

```cpp
// include/struct_registry.h
#ifndef STRUCT_REGISTRY_H_
#define STRUCT_REGISTRY_H_

#include "struct_reflect.h"
#include "user_types.h"       // 包含所有用户定义的 struct
#include <unordered_map>
#include <string>

// 注册宏：将 GetStructInfo_XXX() 加入全局表
#define REG(NAME) { #NAME, &GetStructInfo_##NAME() }

inline const std::unordered_map<std::string, const StructInfo*>& GetStructRegistry()
{
    static const std::unordered_map<std::string, const StructInfo*> table = {
        REG(SensorData),
        REG(MotorStatus),
        // 新增类型在此添加一行
    };
    return table;
}

#undef REG

// 便捷查找函数
inline const StructInfo* FindStructByName(const std::string& name)
{
    auto& table = GetStructRegistry();
    auto it = table.find(name);
    return (it != table.end()) ? it->second : nullptr;
}

#endif // STRUCT_REGISTRY_H_
```

### 4.2 查询层次

`CreateTag` 中的类型解析顺序：

```
1. para 包含 '$'  → 自定义类型（手动指定 size），直接使用 → 现有逻辑不变
2. FindTypeByName(para) 匹配 → 内置简单类型 → 现有逻辑不变
3. FindStructByName(para) 匹配 → 已注册自定义 struct → 新增逻辑
4. 均不匹配 → 报错 "Type definition error!"
```

---

## 5. 涉及文件及修改清单

| 文件 | 操作 | 说明 |
|---|---|---|
| `include/struct_reflect.h` | **新建** | FieldInfo/StructInfo 定义 + REGISTER_STRUCT/FIELD_DESC 宏 + CTYPE_xxx 映射宏 |
| `include/user_types.h` | **新建** | 用户自定义 struct 定义 + REGISTER_STRUCT 注册 |
| `include/struct_registry.h` | **新建** | 全局注册表 GetStructRegistry() + FindStructByName() |
| `toolgplat/type_handle.h` | **修改** | #include "struct_registry.h" |
| `toolgplat/main.cpp` | **修改** | (1) CreateTag 新增第三分支 (2) SelectTag 新增自定义类型字段级输出 (3) 新增 `types` 命令 |

---

## 6. 详细实施步骤

### 步骤 1：创建 `include/struct_reflect.h`

反射基础设施文件，包含：

- `FieldInfo` 结构体定义
- `StructInfo` 结构体定义
- `CTYPE_xxx` 映射宏（TypeCode 枚举名 → C++ 原生类型名）
- `FIELD_DESC(TypeCode, StructName, FieldName)` 宏
- `REGISTER_STRUCT(StructName, ...)` 宏

需要 `#include <cstddef>` (offsetof) 和 `<type_traits>` (is_trivially_copyable_v)。

需要包含 `TypeCode` 枚举定义。由于 `TypeCode` 当前定义在 `toolgplat/type_handle.h` 中，而 `struct_reflect.h` 要放在 `include/` 下被多个组件共享，因此需要把 `TypeCode` 枚举提取到 `include/` 下的一个公共头文件中。

**子步骤 1a**：将 `TypeCode` 枚举从 `toolgplat/type_handle.h` 提取到 `include/type_code.h`。

```cpp
// include/type_code.h
#ifndef TYPE_CODE_H_
#define TYPE_CODE_H_

// -1 代表复合类型
enum TypeCode
{
    Empty = 0,
    Boolean,    // 1
    Char,       // 2
    Int16,      // 3
    UInt16,     // 4
    Int32,      // 5
    UInt32,     // 6
    Int64,      // 7
    UInt64,     // 8
    Single,     // 9
    Double,     // 10
};

#endif // TYPE_CODE_H_
```

`toolgplat/type_handle.h` 改为 `#include "../include/type_code.h"` 替代内联定义的 TypeCode。

### 步骤 2：创建 `include/user_types.h`

用户自定义结构体文件。初始内容包含示例 struct + REGISTER_STRUCT：

```cpp
#ifndef USER_TYPES_H_
#define USER_TYPES_H_

#include "struct_reflect.h"

#pragma pack(push, 8)

struct SensorData {
    int32_t   temperature;
    int32_t   humidity;
    double    pressure;
    bool      alarm;
};

#pragma pack(pop)

REGISTER_STRUCT(SensorData,
    FIELD_DESC(Int32,   SensorData, temperature),
    FIELD_DESC(Int32,   SensorData, humidity),
    FIELD_DESC(Double,  SensorData, pressure),
    FIELD_DESC(Boolean, SensorData, alarm)
)

#endif // USER_TYPES_H_
```

### 步骤 3：创建 `include/struct_registry.h`

全局注册表，汇总所有 user_types.h 中注册的 struct，提供 FindStructByName()。

### 步骤 4：修改 `toolgplat/type_handle.h`

- 将 `TypeCode` 枚举替换为 `#include "../include/type_code.h"`
- 添加 `#include "../include/struct_registry.h"`

### 步骤 5：修改 `toolgplat/main.cpp` — CreateTag 函数

在 `CreateTag` 函数的 `else` 分支中（第 203 行附近），当 `FindTypeByName` 返回 nullptr 时，新增自定义类型查找：

```cpp
// 现有代码
const TypeInfo* ti = FindTypeByName(typeName);
if (!ti)
{
    // ===== 新增：查找已注册的自定义 struct =====
    const StructInfo* si = FindStructByName(typeName);
    if (si)
    {
        size_t itemsize = si->total_size;
        size_t tagsize = (arraysize > 1) ? itemsize * arraysize : itemsize;

        if (tagsize > 16000)
        {
            std::cout << "TAG的大小超过了16000，无法创建" << std::endl;
            return false;
        }

        // 构建类型元数据 buffer: [typecode=-1][arraysize][classname\0]
        char buff[128];
        int* ptypecode = (int*)buff;
        int* parraysize_p = (int*)(buff + 4);
        *ptypecode = -1;
        *parraysize_p = (arraysize > 1) ? arraysize : 0;
        char* classname = buff + 8;
        strcpy(classname, typeName.c_str());
        int typesize = 8 + typeName.length() + 1;

        unsigned int err;
        bool res = createtag(g_hConn, tagName.c_str(), tagsize, buff, typesize, &err);

        if (res)
            std::cout << "Tag '" << tagName << "' created (struct " << typeName
                      << ", " << itemsize << " bytes)" << std::endl;
        else
            std::cout << "Create Tag '" << tagName << "' fail with error code " << err << std::endl;

        return res;
    }
    // ===== 新增结束 =====

    std::cout << "Type definition error!" << std::endl;
    return false;
}
```

### 步骤 6：修改 `toolgplat/main.cpp` — SelectTag 函数（字段级输出）

当前 `SelectTag` 只处理了 `typecode > 0` 的简单类型。需要新增 `typecode == -1` 的自定义类型分支：

```cpp
void SelectTag(std::string tagName)
{
    int typesize;
    unsigned int err;
    char buffer[2048];

    if (readtype(g_hConn, "BOARD", tagName.c_str(), buffer, 2048, &typesize, &err))
    {
        int typecode = *(int*)buffer;
        int arraysize = *(int*)(buffer + 4);

        if (typecode > 0)
        {
            // ... 现有的简单类型处理逻辑（不变）...
        }
        else if (typecode == -1)
        {
            // ===== 新增：自定义 struct 类型 =====
            const char* classname = buffer + 8;
            const StructInfo* si = FindStructByName(classname);

            if (!si)
            {
                std::cout << "Custom type '" << classname
                          << "' not found in local registry." << std::endl;
                return;
            }

            int itemcount = (arraysize > 0) ? arraysize : 1;
            int totalsize = si->total_size * itemcount;
            std::vector<char> data(totalsize);
            timespec timestamp;

            if (readb(g_hConn, tagName.c_str(), data.data(), totalsize, &err, &timestamp))
            {
                for (int idx = 0; idx < itemcount; idx++)
                {
                    if (itemcount > 1)
                        std::cout << "[" << idx << "]" << std::endl;

                    char* base = data.data() + idx * si->total_size;

                    for (int f = 0; f < si->field_count; f++)
                    {
                        const FieldInfo& fi = si->fields[f];
                        std::cout << "  " << fi.name << ": ";
                        const TypeInfo* ti = FindTypeByCode((int)fi.type);
                        if (ti && ti->print)
                            ti->print(std::cout, base + fi.offset);
                        else
                            PrintHex(std::cout, base + fi.offset, fi.size);
                        std::cout << std::endl;
                    }
                }
                std::cout << "-------------------------------------" << std::endl;
                std::cout << "last write time: "
                          << std::asctime(std::localtime(&timestamp.tv_sec));
            }
            // ===== 新增结束 =====
        }
    }
}
```

需要新增一个 `PrintHex` 辅助函数用于无法识别类型时的 fallback 显示：

```cpp
inline void PrintHex(std::ostream& os, const void* data, int size)
{
    const unsigned char* p = (const unsigned char*)data;
    for (int i = 0; i < size && i < 64; i++)
        os << std::hex << std::setw(2) << std::setfill('0') << (int)p[i] << " ";
    os << std::dec;
}
```

### 步骤 7：新增 `types` 命令

在 `Analyse` 函数中添加 `types` 命令，列出所有已注册的内置类型和自定义 struct 类型：

```cpp
if (cmd == "types")
{
    std::cout << "Built-in types:" << std::endl;
    for (auto& [name, info] : GetTypeByName())
        std::cout << "  " << name << "  (" << info.size << " bytes)" << std::endl;

    std::cout << std::endl << "Registered struct types:" << std::endl;
    for (auto& [name, info] : GetStructRegistry())
    {
        std::cout << "  " << name << "  (" << info->total_size << " bytes, "
                  << info->field_count << " fields)" << std::endl;
        for (int i = 0; i < info->field_count; i++)
        {
            const FieldInfo& f = info->fields[i];
            std::cout << "    +" << f.offset << "  " << f.name << "  ("
                      << f.size << " bytes)" << std::endl;
        }
    }
    return;
}
```

---

## 7. `#pragma pack` 一致性保证

这是本方案**最关键的风险点**。

`sizeof(SensorData)` 取决于编译时的对齐设置。如果 `toolgplat` 和业务程序使用不同的 pack，会导致数据错误。

**解决策略**：

1. `include/user_types.h` 内部强制 `#pragma pack(push, 8)` / `#pragma pack(pop)`
2. 所有组件（toolgplat、testapp、业务程序）统一 `#include "user_types.h"`，一处定义
3. 与 `higplat.h` 保持一致（也是 `pack(push, 8)`）
4. struct 中只使用固定大小类型（`int32_t`, `float`, `double` 等），避免平台差异

---

## 8. 向后兼容性

| 用法 | 改动前 | 改动后 |
|---|---|---|
| `create tag1 Int32` | 正常 | 正常（不变） |
| `create tag2 MyStruct$100` | 正常 | 正常（不变，$size 优先） |
| `create tag3 SensorData` | 报 "Type definition error!" | 自动获取 sizeof 并创建 |
| `create tag4 SensorData 10` | 报错 | 创建长度 10 的 SensorData 数组 |
| `create tag5 Unknown` | 报错 | 报错（不变） |
| `select tag3` | 无自定义类型输出 | 字段级格式化输出 |
| `select tag1` | 简单类型输出 | 不变 |

服务端协议完全不变。自定义类型创建时仍然发送 `typecode=-1 + classname`，与现有逻辑一致。

---

## 9. 用户工作流（新增自定义类型）

1. 在 `include/user_types.h` 中定义 struct + REGISTER_STRUCT
2. 在 `include/struct_registry.h` 的 `GetStructRegistry()` 中添加一行 `REG(XXX)`
3. 重新编译 toolgplat（以及需要使用该类型的其他客户端）
4. 在 toolgplat 中直接 `create tagname XXX`（无需 `$size`）
5. `select tagname` 可看到字段级输出

---

## 10. 新增文件结构

```
include/
├── higplat.h              (不变)
├── msg.h                  (不变)
├── type_code.h            (新建 — 提取 TypeCode 枚举)
├── struct_reflect.h       (新建 — FieldInfo/StructInfo + 宏)
├── user_types.h           (新建 — 用户自定义 struct)
└── struct_registry.h      (新建 — 全局注册表)

toolgplat/
├── type_handle.h          (修改 — 改为 include type_code.h, 添加 include struct_registry.h)
└── main.cpp               (修改 — CreateTag/SelectTag/Analyse)
```

---

## 11. 重要注意事项

### POD 类型要求

自定义 struct **必须是 trivially copyable**（没有虚函数、没有 `std::string` 等非平凡成员），因为数据会被 `memcpy` 和 `mmap` 存取。`REGISTER_STRUCT` 宏内嵌 `static_assert` 强制检查。

### 跨编译环境

项目在 Windows 上交叉编译到 Linux。`#pragma pack(push, 8)` 在 MSVC 和 GCC 上行为一致，但需要注意：
- 避免使用 `bool` 做跨进程通信字段（size 不保证），可用 `int32_t` 替代
- 固定大小类型（`int32_t`, `uint64_t`, `float`, `double`）无平台差异

### FIELD_ARRAY 支持

对于 `char name[32]` 这类数组字段，需要额外的 `FIELD_ARRAY` 宏和对应的 `FIELD_DESC_ARRAY`：

```cpp
#define FIELD_DESC_ARRAY(TYPE, STRUCT, NAME, COUNT) \
    { #NAME, TYPE, (int)sizeof(CTYPE_##TYPE) * COUNT, (int)offsetof(STRUCT, NAME) }
```

select 输出时，如果是 `Char` 数组则当字符串输出，其他数组类型逐元素输出。

### 类型名长度限制

当前 `createtag` 发送类型元数据时，类型名保存在 type area 中。classname 写入 `buff + 8`、buff 固定 128 bytes，所以类型名长度最长约 119 字符。实际使用中足够。

---

## 12. 未来扩展方向（不在本次范围内）

- **字段级 write**：`write myTag.temperature 25` — 按字段名写入单个字段（需要 offsetof + WriteBPartial API）
- **X-Macro 迁移**：如果两处定义一致性成为负担，迁移到 X-Macro 方案 (3.3-A)
- **嵌套 struct**：支持 struct 中嵌套另一个已注册的 struct
- **JSON 导入/导出**：将 tag 的字段值以 JSON 格式导入或导出
