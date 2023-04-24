## poorWebServer

### 并发模型采用 线程池 + 非阻塞socket + ET epoll + Reactor事件处理

### HTTP支持GET、POST，POST请求用于请求登录和注册功能

### 用RAII封装锁、信号量，创建时自动调用构造函数，超出作用域自动调用析构函数，安全管理资源

### 多线程模型，使用线程池，空间换时间，工作线程在请求队列上，当有任务到来时，通过竞争获得任务的接管权

### 用Redis数据库代替mysql数据库，WebBench测压速度提升一倍

### 日志采用spdlog，只用包含头文件即可

[gabime/spdlog: Fast C++ logging library. (github.com)](https://github.com/gabime/spdlog)

将源码的include中的spdlog文件夹整个复制到/usr/include/目录下，包含头文件即可,如 #include"spdlog/spdlog.h"

### 内存泄露检测 LeakSanitizer

 -fsanitize=address -g

无内存泄露

### 运行：

服务器测试环境

* Ubuntu版本 20.04
* Redis 版本 5.0.7
  测试前确认已启用Redis数据库

```
make
webbench文件夹内：
make clean //原来的webbench存在就先删除
sudo make && sudo make install ../server
webbench -c 10000  -t  5 http://localhost:9000/
```

### 参考

Linux高性能服务器编程，游双著.

[qinguoyi/TinyWebServer: Linux下C++轻量级Web服务器 (github.com](https://github.com/qinguoyi/TinyWebServer)
