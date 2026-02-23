#ifndef S7CONFIG_H
#define S7CONFIG_H

#include <string>
#include <vector>
#include <cstdint>

// ---- 枚举 ----

enum class S7DataType { BOOL, INT, WORD, DINT, DWORD, REAL, STRING };
enum class S7AreaType { DB, I, Q, M };

// ---- 数据结构 ----

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

// ---- 函数声明 ----

bool LoadConfig(const std::string& filename, AppConfig& config);
void PrintConfig(const AppConfig& config);
int  AreaToSnap7(S7AreaType area);
const char* AreaName(S7AreaType area);

#endif // S7CONFIG_H
