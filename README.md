# gPlat 项目说明

## 项目概况
这是一个 Linux 平台的 C++ 网络项目，原先使用 Visual Studio 进行管理。为了方便在 Linux 环境下使用任意编辑器进行开发，本项目现已支持 Makefile 构建系统。

## 项目结构
主要包含以下模块：

### 1. gplat
- **类型**: 可执行程序 (Server)
- **描述**: 核心服务进程，包含网络通信、进程管理、日志等功能 (类似 Nginx 架构)。
- **入口**: `gplat/nginx.cxx` -> `main()`
- **依赖**: 
  - `pthread` (POSIX 线程库)
- **源文件**: `gplat/*.cxx`

### 2. higplat
- **类型**: 动态链接库 (`libhigplat.so`)
- **描述**: 提供公共功能的共享库。
- **输出**: `lib/libhigplat.so`
- **源文件**: `higplat/*.cpp`

### 3. createq
- **类型**: 可执行程序 (工具)
- **描述**: 一个依赖于 `higplat` 的工具程序。
- **依赖**: 
  - `libhigplat.so`
  - `include/` 目录下的头文件
- **源文件**: `createq/*.cpp`

### 4. TestApplication
- **类型**: 可执行程序 (测试)
- **描述**: 用于测试 `higplat` 功能的测试程序。
- **依赖**: 
  - `libhigplat.so`
- **源文件**: `TestApplication/*.cpp`

### 其他目录
- `include/`: 公共头文件目录。
- `dotNetConsoleApp1/`: .NET 控制台项目 (Makefile 构建系统暂不包含此部分)。
- `readme/`: 旧版文档资源。

## 编译环境要求
- **操作系统**: Linux (推荐) 或 macOS。
- **不兼容**: Windows (需使用 WSL 或虚拟机)。
- **编译器**: 
  - Linux: GNU G++ (支持 C++17)
  - macOS: Clang++

## 编译与运行

### 编译
在项目根目录下运行 `make` 命令即可完成增量编译。

```bash
make
```

构建产物将生成在：
- 可执行文件: `./bin/`
- 库文件: `./lib/`
- 中间文件: `./build/`

### 清理
```bash
make clean
```

### 运行
运行生成的程序前，请确保库路径正确（Makefile 已为生成的可执行文件设置了 rpath）：

```bash
./bin/gplat
./bin/createq
```

## 开发指南
- **添加新文件**: 直接在对应模块的源码目录 (`gplat/`, `higplat/` 等) 添加 `.cpp` 或 `.cxx` 文件即可，无需修改 Makefile (支持通配符)。
- **头文件**: `gplat` 主要使用其目录下的头文件。外部工具可能引用 `include/` 下的头文件。

