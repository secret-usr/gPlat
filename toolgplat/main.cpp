#include <readline/readline.h>
#include <readline/history.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdlib> // for strtol
#include <unistd.h>
#include <string.h>
#include <iomanip>
#include <ctime>
#include "..//include//higplat.h"


//-1代表复合类型
enum TypeCode {
    Empty = 0,
    Boolean,
    Char,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Single,
    Double,
    //Decimal,
    //DateTime
};

enum QBDType
{
    queue = 0,
    board,
    database
};
QBDType g_qbdtype;

int g_hConn;

struct GlobalObj {
    static std::string prefix;
    static std::string nodename;
    static std::string qbdname;
    // Note: rectype and recFields are .NET specific and don't have direct equivalents
    // They would need to be replaced with appropriate C++ alternatives
};

std::string GlobalObj::prefix = "gplat>";
std::string GlobalObj::nodename = "";
std::string GlobalObj::qbdname = "";

void Analyse(const std::vector<std::string>& words);

int main(int argc, char* argv[]) {
    // 设置 readline 的自动补全（可选）
    rl_bind_key('\t', rl_complete); // 启用 Tab 补全

    char* line;
    std::vector<std::string> words;

    while ((line = readline(GlobalObj::prefix.c_str()))) { // 使用 readline 替代 std::getline
        if (!line) break; // 处理 Ctrl+D

        // 添加到历史记录（非空行）
        if (*line) add_history(line);

        // 转换为 std::string 方便处理
        std::string input(line);
        free(line); // readline 返回的指针需要手动释放

        if (input.empty()) continue;

        // 分割单词逻辑（保持不变）
        size_t start = 0, end = 0;
        while ((end = input.find(' ', start)) != std::string::npos) {
            if (end != start) {
                words.push_back(input.substr(start, end - start));
            }
            start = end + 1;
        }
        if (start < input.length()) {
            words.push_back(input.substr(start));
        }

        if (!words.empty()) {
            std::string firstWord = words[0];
            std::transform(firstWord.begin(), firstWord.end(), firstWord.begin(),
                [](unsigned char c) { return std::tolower(c); });

            if (firstWord == "exit" || firstWord == "q")
                break;

            Analyse(words);
        }

        words.clear();
    }

    if (g_hConn > 0)
        close(g_hConn);

    return 0;
}

//int main(int argc, char* argv[])
//{
//    std::cout << GlobalObj::prefix;
//    std::string line;
//    std::vector<std::string> words;
//
//    while (std::getline(std::cin, line))
//    {
//        if (line.empty()) continue;
//
//        // Split into words
//        size_t start = 0, end = 0;
//        while ((end = line.find(' ', start)) != std::string::npos)
//        {
//            if (end != start)
//            {
//                words.push_back(line.substr(start, end - start));
//            }
//            start = end + 1;
//        }
//        if (start < line.length())
//        {
//            words.push_back(line.substr(start));
//        }
//
//        if (!words.empty())
//        {
//            // Case-insensitive comparison
//            std::string firstWord = words[0];
//            std::transform(firstWord.begin(), firstWord.end(), firstWord.begin(),
//                [](unsigned char c) { return std::tolower(c); });
//
//            if (firstWord == "exit" || firstWord == "q")
//                break;
//
//            Analyse(words);
//        }
//
//        words.clear();
//        std::cout << GlobalObj::prefix;
//    }
//
//    if (g_hConn > 0)
//        close(g_hConn);
//
//    return 0;
//}

void HandleConnect(const std::string& remotehost)
{
    int hConn = connectgplat(remotehost.c_str(), 8777);

    if (hConn > 0)
    {
        if (g_hConn > 0)
        {
            close(g_hConn);
        }
        g_hConn = hConn;

        GlobalObj::nodename = remotehost;
        GlobalObj::prefix = remotehost + ">";
    }
    else
    {
        std::cout << "无法连接到" << remotehost << "." << std::endl;
    }
}

void HandleOpenBoard()
{
    g_qbdtype = board;
    GlobalObj::qbdname = "BOARD";

    GlobalObj::prefix = GlobalObj::nodename + ".BOARD>";
}

//para  复合类型名$负荷类型大小 或者 简单类型名
//para2 数组大小
bool CreateItem(const std::string& itemName, const std::string& para, const std::string& para2)
{
    int arraysize = 0;

    if (!para2.empty()) {
        try {
            arraysize = std::stoi(para2);
        } catch (...) {
            arraysize = 0;
        }
    }

    // Split para by '$'
    std::vector<std::string> split;
    size_t start = 0, end = para.find('$');
    while (end != std::string::npos) {
        split.push_back(para.substr(start, end - start));
        start = end + 1;
        end = para.find('$', start);
    }
    split.push_back(para.substr(start));

    if (split.size() > 1) {
        if (arraysize == 0 || arraysize == 1) {
            // 复合类型 - 需要实现
            char buff[100];
            int* ptypecode = (int*)buff;
            int* parraysize = (int*)(buff + 4);
            *ptypecode = -1;
            *parraysize = 0;
            int itemsize = std::stoi(split[1]);
            char* classname = buff + 8;
            strcpy(classname, split[0].c_str());
            int typesize = 8 + split[0].length() + 1; //类型代码，数组大小，类型名，0结束符

            // 检查大小限制
            if (itemsize > 128000) {
                std::cout << "TAG的大小超过了128000，无法创建" << std::endl;
                return false;
            }

            unsigned int err;
            bool res = createtag(g_hConn, itemName.c_str(), itemsize, buff, typesize, &err);

            if (res) {
                std::cout << "Tag '" << itemName << "' created successfully, Record size="
                    << itemsize << ", Type size=" << typesize << std::endl;
            }
            else {
                std::cout << "Create Tag '" << itemName << "' fail with error code " << err << std::endl;
            }

            return res;
        }
        else if (arraysize > 1) {
            // 复合类型数组 - 需要实现
            char buff[100];
            int* ptypecode = (int*)buff;
            int* parraysize = (int*)(buff + 4);
            *ptypecode = -1;
            *parraysize = 0;
            int itemsize = std::stoi(split[1]) * arraysize;
            char* classname = buff + 8;
            strcpy(classname, split[0].c_str());
            int typesize = 8 + split[0].length() + 1; //类型代码，数组大小，类型名，0结束符

            // 检查大小限制
            if (itemsize > 128000) {
                std::cout << "TAG的大小超过了128000，无法创建" << std::endl;
                return false;
            }

            unsigned int err;
            bool res = createtag(g_hConn, itemName.c_str(), itemsize, buff, typesize, &err);

            if (res) {
                std::cout << "Tag '" << itemName << "' created successfully, Record size="
                    << itemsize << ", Type size=" << typesize << std::endl;
            }
            else {
                std::cout << "Create Tag '" << itemName << "' fail with error code " << err << std::endl;
            }

            return res;
        }
    }
    else if (arraysize == 0 || arraysize == 1) {
        // 简单类型
        std::string typeName = para;
        TypeCode typecode = Empty;
        size_t itemsize = 0;

        // 确定类型和大小
        if (typeName == "Boolean") {
            typecode = Boolean;
            itemsize = sizeof(bool);
        }
        else if (typeName == "Char") {
            typecode = Char;
            itemsize = sizeof(char);
        }
        else if (typeName == "Double") {
            typecode = Double;
            itemsize = sizeof(double);
        }
        else if (typeName == "Int16") {
            typecode = Int16;
            itemsize = sizeof(int16_t);
        }
        else if (typeName == "Int32") {
            typecode = Int32;
            itemsize = sizeof(int32_t);
        }
        else if (typeName == "Int64") {
            typecode = Int64;
            itemsize = sizeof(int64_t);
        }
        else if (typeName == "Single") {
            typecode = Single;
            itemsize = sizeof(float);
        }
        else if (typeName == "UInt16") {
            typecode = UInt16;
            itemsize = sizeof(uint16_t);
        }
        else if (typeName == "UInt32") {
            typecode = UInt32;
            itemsize = sizeof(uint32_t);
        }
        else if (typeName == "UInt64") {
            typecode = UInt64;
            itemsize = sizeof(uint64_t);
        }
        else {
            std::cout << "Type definition error!" << std::endl;
            return false;
        }

        // 检查大小限制
        if (itemsize > 128000) {
            std::cout << "TAG的大小超过了128000，无法创建" << std::endl;
            return false;
        }

        char buff[8];
        int* ptypecode = (int*)buff;
        int* parraysize = (int*)(buff + 4);
        *ptypecode = (int)typecode;
        *parraysize = 0;

        unsigned int err;
        bool res = createtag(g_hConn, itemName.c_str(), (int)itemsize, buff, sizeof(buff), &err);

        if (res) {
            std::cout << "Tag '" << itemName << "' created successfully, Record size=" 
                     << itemsize << ", Type size=" << sizeof(typecode) << std::endl;
        } else {
            std::cout << "Create Tag '" << itemName << "' fail with error code " << err << std::endl;
        }
        return res;
    }
    else if (arraysize > 1) {
        // 简单类型数组
        std::string typeName = para;
        TypeCode typecode = Empty;
        size_t itemsize = 0;

        if (typeName == "String" || typeName == "string" || typeName == "STRING") {
            typecode = Char;
            itemsize = sizeof(char) * arraysize;
        }
        else if (typeName == "Boolean") {
            typecode = Boolean;
            itemsize = sizeof(bool) * arraysize;
        }
        else if (typeName == "Char") {
            typecode = Char;
            itemsize = sizeof(char) * arraysize;
        }
        else if (typeName == "Double") {
            typecode = Double;
            itemsize = sizeof(double) * arraysize;
        }
        else if (typeName == "Int16") {
            typecode = Int16;
            itemsize = sizeof(int16_t) * arraysize;
        }
        else if (typeName == "Int32") {
            typecode = Int32;
            itemsize = sizeof(int32_t) * arraysize;
        }
        else if (typeName == "Int64") {
            typecode = Int64;
            itemsize = sizeof(int64_t) * arraysize;
        }
        else if (typeName == "Single") {
            typecode = Single;
            itemsize = sizeof(float) * arraysize;
        }
        else if (typeName == "UInt16") {
            typecode = UInt16;
            itemsize = sizeof(uint16_t) * arraysize;
        }
        else if (typeName == "UInt32") {
            typecode = UInt32;
            itemsize = sizeof(uint32_t) * arraysize;
        }
        else if (typeName == "UInt64") {
            typecode = UInt64;
            itemsize = sizeof(uint64_t) * arraysize;
        }
        else {
            std::cout << "Type definition error!" << std::endl;
            return false;
        }

        // 检查大小限制
        if (itemsize > 128000) {
            std::cout << "TAG的大小超过了128000，无法创建" << std::endl;
            return false;
        }

        char buff[8];
        int* ptypecode = (int*)buff;
        int* parraysize = (int*)(buff + 4);
        *ptypecode = (int)typecode;
        *parraysize = arraysize;

        unsigned int err;
        bool res = createtag(g_hConn, itemName.c_str(), (int)itemsize, buff, sizeof(buff), &err);

        if (res) {
            std::cout << "Array Tag '" << itemName << "' created successfully, Record size=" 
                     << itemsize << ", Type size=" << sizeof(buff) << std::endl;
        } else {
            std::cout << "Create Array Tag '" << itemName << "' fail with error code " << err << std::endl;
        }
        return res;
    }

    return true;
}

void HandleCreate(const std::vector<std::string>& words)
{
    if (GlobalObj::qbdname.empty())
    {
        std::cout << "No db or board opened." << std::endl;
        return;
    }

    if (g_qbdtype == database && words.size() == 4)
    {
        //CreateTable(words[1], words[2], words[3]);
    }
    else if (g_qbdtype == board && words.size() == 3)
    {
        std::string firstWord = words[1];
        std::transform(firstWord.begin(), firstWord.end(), firstWord.begin(),
            [](unsigned char c) { return std::tolower(c); });

        if (firstWord == "from")
        {
            //CreateItemFromFile(words[2]);
        }
        else
        {
            CreateItem(words[1], words[2], "");
        }
    }
    else if (g_qbdtype == board && words.size() == 4)
    {
        CreateItem(words[1], words[2], words[3]);
    }
}

void HandleMan()
{
    struct CommandInfo {
        std::string name;
        std::string usage;
        std::string desc;
    };

    static const std::vector<CommandInfo> commands = {
        {"connect", "connect [ip]", "Connect to server (default 127.0.0.1)"},
        {"openb", "openb", "Open BOARD"},
        {"create", "create <name> <type> [arr_size]", "Create a new tag"},
        {"select", "select <tagname>", "Read data from a tag"},
        {"list", "list", "List all tag names in the opened board"},
        {"man", "man", "Show this help message"},
        {"exit", "exit (or q)", "Exit the tool"}
    };

    std::cout << std::left << std::setw(12) << "Command" 
              << std::setw(35) << "Usage" 
              << "Function" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    for (const auto& cmd : commands)
    {
        std::cout << std::left << std::setw(12) << cmd.name 
                  << std::setw(35) << cmd.usage 
                  << cmd.desc << std::endl;
    }
}

void HandleList()
{
    if (GlobalObj::qbdname.empty())
    {
        std::cout << "No db or board opened." << std::endl;
        return;
    }
    
    if (g_qbdtype != board) {
        std::cout << "List is only supported for BOARD type." << std::endl;
        return;
    }

    // Allocate memory for BOARD_HEAD
    // Note: sizeof(BOARD_HEAD) includes the full index array
    BOARD_HEAD* pHead = (BOARD_HEAD*)malloc(sizeof(BOARD_HEAD));
    if (pHead == NULL) {
        std::cout << "Failed to allocate memory for board header." << std::endl;
        return;
    }

    unsigned int error = 0;
    if (readheadb(g_hConn, GlobalObj::qbdname.c_str(), pHead, &error))
    {
        std::cout << "\nExisting Tags in " << GlobalObj::qbdname << ":" << std::endl;
        std::cout << std::left 
                  << std::setw(8)  << "Index"
                  << std::setw(30) << "itemname" 
                  << std::setw(12) << "itemsize" 
                  << std::setw(12) << "typesize"
                  << "timestamp(UTC+8)" << std::endl;
        std::cout << std::string(90, '-') << std::endl;

        int count = 0;
        for (int i = 0; i < INDEXSIZE; i++)
        {
            // Check if itemname is not empty string and not erased
            if (pHead->index[i].itemname[0] != '\0' && !pHead->index[i].erased)
            {
                char timebuf[64] = {0};
                time_t seconds = pHead->index[i].timestamp.tv_sec;
                seconds += 8 * 3600; // UTC+8
                struct tm* tm_info = gmtime(&seconds);
                if (tm_info) {
                    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);
                }

                std::cout << std::left 
                          << std::setw(8)  << i
                          << std::setw(30) << pHead->index[i].itemname 
                          << std::setw(12) << pHead->index[i].itemsize
                          << std::setw(12) << pHead->index[i].typesize
                          << timebuf << std::endl;
                
                count++;
            }
        }
        std::cout << "\nTotal tags found: " << count << std::endl;
    }
    else
    {
        std::cout << "Failed to list tags. Error code: " << error << std::endl;
    }

    free(pHead);
}

void HandleSelect(const std::string& tagName)
{
    if (GlobalObj::qbdname.empty())
    {
        std::cout << "No db or board opened." << std::endl;
        return;
    }

    if (g_qbdtype != board) {
        std::cout << "Select is only supported for BOARD type." << std::endl;
        return;
    }

    // 1. Get Board Header to find item size
    BOARD_HEAD* pHead = (BOARD_HEAD*)malloc(sizeof(BOARD_HEAD));
    if (pHead == NULL) {
        std::cout << "Failed to allocate memory for board header." << std::endl;
        return;
    }

    unsigned int error = 0;
    if (!readheadb(g_hConn, GlobalObj::qbdname.c_str(), pHead, &error))
    {
        std::cout << "Failed to read board header. Error code: " << error << std::endl;
        free(pHead);
        return;
    }

    int itemSize = -1;
    for (int i = 0; i < INDEXSIZE; i++)
    {
        if (pHead->index[i].itemname[0] != '\0' && !pHead->index[i].erased)
        {
            if (tagName == pHead->index[i].itemname)
            {
                itemSize = pHead->index[i].itemsize;
                break;
            }
        }
    }
    free(pHead); // Done with header

    if (itemSize == -1)
    {
        std::cout << "Tag '" << tagName << "' not found." << std::endl;
        return;
    }

    // 2. Read Data using readb
    char* buffer = new char[itemSize + 1]; // +1 for safety if treated as string later
    memset(buffer, 0, itemSize + 1);
    timespec ts = {0, 0};

    if (readb(g_hConn, tagName.c_str(), buffer, itemSize, &error, &ts))
    {
        char timebuf[64] = {0};
        time_t seconds = ts.tv_sec;
        seconds += 8 * 3600; // UTC+8
        struct tm* tm_info = gmtime(&seconds);
        if (tm_info) {
            strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);
        }

        std::cout << "Read Tag '" << tagName << "' successfully." << std::endl;
        std::cout << "Timestamp: " << timebuf << " (UTC+8)" << std::endl;
        std::cout << "Data Size: " << itemSize << " bytes" << std::endl;
        std::cout << "Data (Hex): " << std::endl;
        
        // Hex Dump
        for (int i = 0; i < itemSize; ++i)
        {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)buffer[i] << " ";
            if ((i + 1) % 16 == 0) std::cout << std::endl;
        }
        std::cout << std::dec << std::endl; // Restore dec

        // Try printing as string if it looks printable
        bool isPrintable = true;
        for(int i=0; i<itemSize; ++i) {
             if (buffer[i] == 0 && i == itemSize - 1) continue; // Null terminator at end is fine
             if (buffer[i] != 0 && !isprint((unsigned char)buffer[i])) { 
                 isPrintable = false; 
                 break; 
             }
        }
        if (isPrintable && itemSize > 0) {
            std::cout << "Data (String): " << buffer << std::endl;
        }
    }
    else
    {
        std::cout << "Failed to read tag '" << tagName << "'. Error code: " << error << std::endl;
    }

    delete[] buffer;
}

void Analyse(const std::vector<std::string>& words)
{
    if (words.empty()) return;

    std::string cmd = words[0];
    // 转换为小写
    std::transform(cmd.begin(), cmd.end(), cmd.begin(),
        [](unsigned char c) { return std::tolower(c); });

 //   if (cmd == "clear")
 //   {
 //       if (words.size() > 1)
 //       {
 //           if (g_qbdtype == database)
 //           {
 //               HandleClearDB(GlobalObj::qbdname);
 //           }
 //           else if (g_qbdtype == board)
 //           {
 //               HandleClearBoard(GlobalObj::qbdname);
 //           }
 //       }
 //       return;
 //   }

    if (cmd == "openb")
    {
        HandleOpenBoard();
        return;
    }

 //   if (words.size() < 2)
    if (cmd == "man")
    {
        HandleMan();
        return;
    }

    if (cmd == "list")
    {
        HandleList();
        return;
    }

 //   {
 //       std::cout << "Syntax error." << std::endl;
 //       return;
 //   }

    if (cmd == "connect")
    {
        if (words.size() < 2)
        {
            HandleConnect("127.0.0.1");
        }
        else
        {
            HandleConnect(words[1]);
        }
        return;
    }

 //   if (cmd == "open")
 //   {
 //       HandleOpen(words[1]);
 //       return;
 //   }

    

 //   if (cmd == "opendb")
 //   {
 //       HandleOpenDB(words[1]);
 //       return;
 //   }
   if (cmd == "select")
    {
        if (words.size() < 2) {
             std::cout << "Usage: select <tagname>" << std::endl;
        } else {
             HandleSelect(words[1]);
        }
        return;
    }

 
 //   if (cmd == "select")
 //   {
 //       HandleSelect(words[1]);
 //       return;
 //   }

    if (cmd == "create")
    {
        HandleCreate(words);
        return;
    }

 //   if (cmd == "delete")
 //   {
 //       HandleDelete(words[1]);
 //       return;
 //   }

    std::cout << "Unknown command." << std::endl;
}
