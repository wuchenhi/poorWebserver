## webserver

### 支持GET、POST请求

### 用RAII封装锁、信号量，创建时自动调用构造函数，超出作用域自动调用析构函数，安全管理资源

### 多线程模型，使用线程池，空间换时间，工作线程在请求队列上，当有任务到来时，通过竞争获得任务的接管权

### WebBench测压

### 日志采用spdlog，只用包含头文件即可

[gabime/spdlog: Fast C++ logging library. (github.com)](https://github.com/gabime/spdlog)

将源码的include中的spdlog文件夹整个复制到/usr/include/目录下，包含头文件即可,如 #include"spdlog/spdlog.h"

### 非阻塞 ET

### 内存泄露检测 LeakSanitizer

clang++ -fsanitize=address -g webServe.cpp

无内存泄露

### 运行：

服务器测试环境

* Ubuntu版本20.04
* MySQL版本8.0.28

```
make
webbench文件夹内：
sudo make && sudo make install ../server
webbench -c 1  -t  2 http://127.0.0.1:9001
```
CREATE USER 'wuyi'@'LOCALHOST' IDENTIFIED BY 'Wch@0824';