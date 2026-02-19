# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

gPlat is a real-time data platform / middleware server for Linux. It provides inter-process communication via three data models:
- **Board**: Shared-memory key-value store for named data items ("tags"), supporting binary and string data
- **Queue**: FIFO message queues with configurable record sizes, supporting shift mode and normal mode
- **Database (DB)**: Table-based storage (partially implemented)

Additional features include publish-subscribe with delayed posting, timer-based periodic events, and TCP network access (default port 8777) using a custom binary protocol.

## Build System

The code is authored on Windows and **cross-compiled to Linux** via Visual Studio 2022's "Visual C++ for Linux Development" workload.

**Primary build**: Visual Studio solution `gPlat.sln` with `.vcxproj` projects targeting Linux (ApplicationType = Linux). Remote root directory: `~/projects`.

**CMake build** (gplat server only):
```bash
cd gplat
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

**Post-build**: `higplat` copies `libhigplat.so` to `/usr/local/lib/` and runs `ldconfig`.

**C++ standard**: C++17 for gplat and higplat, C++11 for testapp.

## Architecture

The server follows an **Nginx-inspired architecture**:
- **Master/worker process model** via `ngx_master_process_cycle()` with `socketpair()` IPC
- **Epoll-based event loop** for non-blocking I/O
- **Connection pool** with pre-allocated `ngx_connection_s` objects managed via free-list
- **Thread pool** (`CThreadPool`) consuming from a message queue with pthread mutex/condvar
- **Configuration** read from `nginx.conf` via `CConfig` singleton
- **Daemon mode** via `ngx_daemon()`

**Singletons** (all use nested `CGarhuishou` destructor pattern): `CConfig`, `CMemory`, `CCRC32`.

**Message dispatching**: Function pointer array `statusHandler[]` in `ngx_c_slogic.cxx`, indexed by `MSGID` enum values from `msg.h`.

**Pub/Sub**: `CSubscribe` uses `std::map<std::string, std::list<EventNode>>` with `std::shared_mutex` for thread-safe subscriber management. Event types: DEFAULT, POST_DELAY, NOT_EQUAL_ZERO, EQUAL_ZERO.

## Component Relationships

```
  testapp / toolgplat  (client apps)
         |  link libhigplat.so, use network API
         v
      higplat          (client shared library)
         |  TCP socket (port 8777)
         v
       gplat           (server, epoll + thread pool)
         |  calls higplat local API (ReadQ/WriteQ/ReadB/WriteB)
         v
    QBD files          (memory-mapped data files in ./qbdfile/)
```

- **gplat/** — Server executable. Entry point: `nginx.cxx`. Networking: `ngx_c_socket*`. Business logic: `ngx_c_slogic.*`. Subscribe engine: `CSubscribe.*`.
- **higplat/** — Client shared library (`libhigplat.so`). Contains both local mmap-based QBD operations and network client API. Public API declared with `extern "C"` in `include/higplat.h`.
- **include/** — Shared headers. `higplat.h` (public API + data structures), `msg.h` (wire protocol: `MSGID` enum + `MSGHEAD` struct), `timer_manager.h` (epoll+timerfd min-heap timer).
- **createq/, createb/** — CLI tools to create Queue and Board data files on disk.
- **toolgplat/** — Interactive REPL client using libreadline.
- **testapp/** — Multi-threaded integration test exercising subscribe, read, and write in 3 threads.

## Key Files

| File | Purpose |
|---|---|
| `gplat/nginx.cxx` | Server entry point (main), QBD file loading |
| `gplat/ngx_c_slogic.cxx` | Message handler dispatch table and handler implementations |
| `gplat/ngx_c_socket.cxx` | Epoll initialization, connection management |
| `gplat/ngx_c_socket_request.cxx` | Request parsing and response sending |
| `gplat/ngx_process_cycle.cxx` | Master/worker process lifecycle |
| `gplat/CSubscribe.cpp` | Publish-subscribe engine |
| `higplat/higplat.cpp` | All client API + local QBD operations |
| `include/msg.h` | Binary protocol definition (MSGID enum, MSGHEAD struct) |
| `include/higplat.h` | Public API declarations and QBD data structures |

## Wire Protocol

Messages use `MSGHEAD` (packed struct) as header followed by a variable-length body (max 128KB). The `MSGID` enum defines operation codes. CRC32 checksums validate data integrity. Adding a new message type requires:
1. Add enum value to `MSGID` in `include/msg.h`
2. Implement handler in `gplat/ngx_c_slogic.cxx`
3. Register handler in `statusHandler[]` array (index must match enum value)
4. Add client-side API in `higplat/higplat.cpp` and declare in `include/higplat.h`

## Dependencies

- **System**: pthread, Linux epoll, timerfd, eventfd, mmap, POSIX sockets/signals, `std::filesystem`
- **External**: libreadline (toolgplat only)
- **Internal**: All executables link `libhigplat.so`

## Data File Tools

```bash
# Create a Board file (size in bytes)
createb <boardname> -s <size>

# Create a Queue file
createq <queuename> -n <record_count> -l <record_size>
```

Data files are stored in `./qbdfile/` relative to the server working directory.

## Code Conventions

- Comments are a mix of Chinese (Simplified) and English
- Server-side files use `.cxx` extension; library and tool files use `.cpp`
- Commit messages follow the pattern: `YYMMDD-NN:description` (e.g., `260212-01:修改注释`)
- RAII locking via `CLock` wrapper around `pthread_mutex_t`
- No formal test framework; `testapp/` is the manual integration test
