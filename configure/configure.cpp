#include "configure.h"

Config::Config(){
    //端口号,默认9000
    PORT = 9000;

    //数据库连接池数量,默认8
    redis_num = 8;

    //线程池内的线程数量,根据硬件确定
    thread_num = std::thread::hardware_concurrency();
}

void Config::parse_arg(int argc, char*argv[]){
    int opt;
    // 单个字符后接一个冒号：表示该选项后必须跟一个参数
    const char *str = "p:s:t";
    // getopt()用来分析命令行参数 参数argc和argv分别代表参数个数和内容
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            PORT = atoi(optarg);
            break;
        }
        case 's':
        {
            redis_num = atoi(optarg);
            break;
        }
        case 't':
        {
            thread_num = atoi(optarg);
            break;
        }
        default:
            break;
        }
    }
}