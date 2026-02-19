# 贡献指南

欢迎为 gPlat 项目做出贡献！本文档提供了参与项目开发的指南。

---

## 开发环境设置

### 前置要求
- Linux 系统（或 WSL2/CYGWIN）
- GCC/G++ 编译器（支持 C++17）
- CMake 3.12+
- Git
- Visual Studio 2022（可选）

### 环境配置

#### Linux 环境
```bash
# 安装依赖
sudo apt-get update
sudo apt-get install build-essential cmake git

# 克隆项目
git clone <repository-url>
cd gPlat

# 创建构建目录
mkdir build && cd build
cmake ..
make

# 创建运行时目录
mkdir -p ../qbdfile
```

#### Visual Studio 远程开发
1. 安装 Visual Studio 2022
2. 安装 "Linux development with C++" 工作负载
3. 配置远程 Linux 连接
4. 打开 gPlat.sln
5. 选择远程目标并构建

---

## 分支管理

### 分支策略

```
master          # 稳定版本，用于发布
  └─ develop    # 开发主分支
       ├─ feature/xxx   # 功能开发分支
       ├─ bugfix/xxx    # Bug 修复分支
       └─ hotfix/xxx    # 紧急修复分支
```

### 分支命名规范

- **功能分支**: `feature/description`
  - 例如: `feature/add-timeout-handling`
- **Bug 修复**: `bugfix/issue-number-description`
  - 例如: `bugfix/123-fix-memory-leak`
- **紧急修复**: `hotfix/description`
  - 例如: `hotfix/crash-on-disconnect`

---

## 工作流程

### 1. 创建分支
```bash
# 从 develop 创建功能分支
git checkout develop
git pull origin develop
git checkout -b feature/your-feature-name
```

### 2. 开发

#### 代码编写
- 遵循项目代码风格（见 CLAUDE.md）
- 添加必要的注释
- 保持函数简洁（建议 < 50 行）
- 避免重复代码

#### 编译测试
```bash
# 编译
cd build
make

# 运行测试
cd ../x64/Debug
./testapp
```

### 3. 提交代码

#### 提交信息格式
```
<type>(<scope>): <subject>

<body>

<footer>
```

**type（类型）**:
- `feat`: 新功能
- `fix`: Bug 修复
- `docs`: 文档更新
- `refactor`: 代码重构
- `test`: 测试相关
- `chore`: 构建/工具相关
- `perf`: 性能优化
- `style`: 代码格式（不影响功能）

**scope（范围）**:
- `socket`: 网络层
- `logic`: 业务逻辑层
- `qbd`: 数据管理
- `config`: 配置系统
- `api`: 客户端 API
- `tool`: 工具程序

**示例**:
```bash
git commit -m "feat(socket): add connection timeout handling

Add configurable timeout for socket connections to prevent
indefinite blocking. Default timeout is 30 seconds.

Closes #123"
```

### 4. 推送和创建 Pull Request
```bash
# 推送分支
git push origin feature/your-feature-name

# 在 GitHub/GitLab 创建 Pull Request
# 目标分支: develop
```

---

## 代码审查

### 审查要点

#### 1. 功能正确性
- [ ] 功能符合需求
- [ ] 边界条件处理正确
- [ ] 错误处理完整

#### 2. 代码质量
- [ ] 遵循项目代码风格
- [ ] 命名清晰有意义
- [ ] 注释充分且准确
- [ ] 无明显的代码异味

#### 3. 性能考虑
- [ ] 无明显性能瓶颈
- [ ] 合理使用锁
- [ ] 避免不必要的内存拷贝

#### 4. 安全性
- [ ] 无缓冲区溢出风险
- [ ] 参数验证完整
- [ ] 无竞态条件

#### 5. 可维护性
- [ ] 代码结构清晰
- [ ] 易于扩展
- [ ] 测试覆盖充分

### 审查流程
1. 提交者创建 PR
2. 自动检查（编译、测试）
3. 代码审查（至少 1 人）
4. 修改反馈
5. 批准合并

---

## 测试规范

### 单元测试
```cpp
// 在 testapp/ 中添加测试函数
void test_new_feature() {
    printf("Testing new feature...\n");

    unsigned int error = 0;
    int sockfd = connectgplat("127.0.0.1", 8777);
    assert(sockfd >= 0);

    // 测试逻辑
    bool result = new_api_function(sockfd, params, &error);
    assert(result == true);
    assert(error == 0);

    disconnectgplat(sockfd);
    printf("Test passed!\n");
}
```

### 集成测试
```bash
# 启动服务端
./gplat

# 运行测试客户端
./testapp

# 检查日志
tail -f error.log

# 停止服务端
killall gplat
```

### 性能测试
```cpp
// 测量操作延迟
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();
// 执行操作
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
printf("Operation took %ld microseconds\n", duration.count());
```

---

## 文档更新

### 何时更新文档

- 添加新功能 → 更新 `PRODUCT.md`
- 修改架构 → 更新 `ARCHITECTURE.md`
- 添加 API → 更新 `.claude/context/api_reference.md`
- 修改配置 → 更新 `CLAUDE.md`
- 重要变更 → 更新 `CHANGELOG.md`（如果有）

### 文档风格
- 使用清晰的标题层级
- 提供代码示例
- 保持格式一致
- 中英文混排时注意空格

---

## 常见问题

### Q: epoll_wait 返回错误
```bash
# 检查文件描述符限制
ulimit -n
ulimit -n 10000  # 增加限制
```

### Q: 如何调试 Worker 进程
```bash
# 方式 1: 设置 Daemon=0 前台运行
# nginx.conf
[Proc]
Daemon = 0

# 方式 2: attach 到 worker 进程
ps aux | grep "worker process"
gdb -p <worker_pid>
```

---

## 发布流程

### 版本号规范
使用语义化版本 (Semantic Versioning):
- `MAJOR.MINOR.PATCH`
- 例如: `1.2.3`
  - MAJOR: 不兼容的 API 变更
  - MINOR: 向后兼容的功能新增
  - PATCH: 向后兼容的 Bug 修复

### 发布步骤
1. 更新版本号
2. 更新 CHANGELOG.md
3. 合并到 master
4. 打 tag
   ```bash
   git tag -a v1.2.3 -m "Release version 1.2.3"
   git push origin v1.2.3
   ```
5. 创建 Release 说明
6. 构建发布包

---

## 联系方式

### 问题报告
- 创建 GitHub Issue
- 邮件联系维护者

### 功能建议
- 创建 Feature Request Issue
- 参与讨论

---

**最后更新**: 2026-02-13
