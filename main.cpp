#include "configure/configure.h"

int main(int argc, char *argv[])
{
    //需要修改的数据库信息,登录名,密码,库名
    string user = "wuyi";
    string passwd = "Wch@0824";
    string databasename = "yourdb";

    //命令行解析
    Config config;
    config.parse_arg(argc, argv);

    WebServer server;
    //端口号
    //数据库连接池数量 sql_num
    //线程池内的线程数量 thread_num
    //初始化
    server.init(config.PORT, user, passwd, databasename,
                config.sql_num, config.thread_num
                );
    
    //数据库
    server.sql_pool();

    //线程池
    server.thread_pool();

    //监听
    server.eventListen();

    //运行
    server.eventLoop();

    return 0;
}