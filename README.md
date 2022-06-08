## webserver

**并发模型采用 线程池 + 非阻塞socket + ET epoll + Reactor事件处理**

### HTTP支持GET、POST请求

### 用RAII封装锁、信号量，创建时自动调用构造函数，超出作用域自动调用析构函数，安全管理资源

### 多线程模型，使用线程池，空间换时间，工作线程在请求队列上，当有任务到来时，通过竞争获得任务的接管权

### 配合mysql数据库实现web端用户**注册、登录**功能，可以请求服务器**图片和视频文件**

### WebBench测压可以实现**上万的并发连接**

### 日志采用spdlog，只用包含头文件即可

[gabime/spdlog: Fast C++ logging library. (github.com)](https://github.com/gabime/spdlog)

将源码的include中的spdlog文件夹整个复制到/usr/include/目录下，包含头文件即可,如 #include"spdlog/spdlog.h"

### 非阻塞 ET

### 内存泄露检测 LeakSanitizer

clang++ -fsanitize=address -g webserve.cpp

无内存泄露

### 运行：

服务器测试环境

* Ubuntu版本20.04
* MySQL版本8.0.28
  测试前确认已安装MySQL数据库

```C++
  //有必要可以先创建一个用户，如

  CREATE USER 'wuyi'@'LOCALHOST' IDENTIFIED BY 'Wch@0824';
  update mysql.user set Host='%' 
  where User='wuyi' and Host='localhost';

  //给予权限
  grant all privileges on *.* to wuyi@'%'；
  // 建立yourdb库

  create database yourdb;


  // 创建user表

  USE yourdb;

  CREATE TABLE user(

      username char(50) NULL,

      passwd char(50) NULL

  )ENGINE=InnoDB;


  // 添加数据

  INSERT INTO user(username, passwd) VALUES('name', 'passwd');

```

修改main.cpp中的数据库初始化信息

```C++
//数据库登录名,密码,库名
string user = "wuyi";
string passwd = "Wch@0824";
string databasename = "yourdb";

make
webbench文件夹内：
make clean //原来的webbench存在就先删除
sudo make && sudo make install ../server
webbench -c 10000  -t  5 http://127.0.0.1:9001

```

### 参考

Linux高性能服务器编程，游双著.

[qinguoyi/TinyWebServer: Linux下C++轻量级Web服务器 (github.com)](https://github.com/qinguoyi/TinyWebServer)
