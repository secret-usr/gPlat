cmake_minimum_required(VERSION 3.12)
project(gplat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录（Debug 配置）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/../x64/Debug")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/../x64/Debug")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/../x64/Debug")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/../x64/Release")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/../x64/Release")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/../x64/Release")

# 设置目标平台和配置相关选项
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-DLINUX)
endif()

# 设置调试和发布版本的编译选项
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# 包含目录
include_directories(${CMAKE_SOURCE_DIR})

# 源文件列表
set(SRC_FILES
    CSubscribe.cpp
    nginx.cxx
    ngx_c_conf.cxx
    ngx_c_crc32.cxx
    ngx_c_memory.cxx
    ngx_c_slogic.cxx
    ngx_c_socket.cxx
    ngx_c_socket_accept.cxx
    ngx_c_socket_conn.cxx
    ngx_c_socket_inet.cxx
    ngx_c_socket_request.cxx
    ngx_c_threadpool.cxx
    ngx_daemon.cxx
    ngx_event.cxx
    ngx_log.cxx
    ngx_printf.cxx
    ngx_process_cycle.cxx
    ngx_setproctitle.cxx
    ngx_signal.cxx
    ngx_string.cxx
)

# 头文件列表
set(HEADER_FILES
    CSubscribe.h
    ngx_comm.h
    ngx_c_conf.h
    ngx_c_crc32.h
    ngx_c_lockmutex.h
    ngx_c_memory.h
    ngx_c_slogic.h
    ngx_c_socket.h
    ngx_c_threadpool.h
    ngx_func.h
    ngx_global.h
    ngx_logiccomm.h
    ngx_macro.h
    ngx_signal.h
)

# 添加库搜索路径（根据实际路径调整）
link_directories(
    ${CMAKE_SOURCE_DIR}/../x64/Debug   # 假设库在 VS 的默认输出目录
    /usr/local/lib                     # 常见 Linux 库路径
)

# 创建可执行文件
add_executable(gplat ${SRC_FILES} ${HEADER_FILES})

# 链接库 (根据您的实际需求调整)
target_link_libraries(gplat PRIVATE 
    pthread
    higplat            # 链接 higplat 库
)

# 安装规则 (可选)
install(TARGETS gplat
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 多配置支持
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES Debug Release)
    set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
        "Reset the configurations to what we need" FORCE)
endif()