#include "configure/configure.h"

int main(int argc, char *argv[])
{
    //命令行解析
    Config config;
    config.parse_arg(argc, argv);

    WebServer server;
    //端口号
    //数据库连接池数量 redis_num
    //线程池内的线程数量 thread_num
    //初始化
    server.init(config.PORT, config.redis_num, config.thread_num);
    
    //数据库
    server.redis_pool();

    //线程池
    server.thread_pool();

    //监听
    server.eventListen();

    //运行
    server.eventLoop();

    return 0;
}