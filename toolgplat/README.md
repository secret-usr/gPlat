## troubleshooting

### 1

报错 `fatal error: readline/readline.h: No such file or directory`，需要安装 readline 开发库：

```bash
sudo apt-get update && sudo apt-get install -y libreadline-dev
```

如果是 ubuntu noble 系统，在 arm64 平台不能使用清华源、阿里源，需要使用官方源。


## 引入依赖

### iomanip

iomanip 提供了输入输出流的格式化功能，例如设置宽度、精度、填充字符等。会用到 `std::setw`、`std::setprecision` 等函数来控制输出格式。

### ctime

ctime 提供了处理日期和时间的函数，例如获取当前时间、格式化时间等。会用到 `std::strftime` 函数将时间戳格式化为人类可读格式。