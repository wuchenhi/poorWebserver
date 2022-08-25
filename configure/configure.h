#ifndef M_CONFIG_H
#define M_CONFIG_H

#include "../webserver/webserver.h"

using namespace std;

class Config
{
public:
    Config();
    ~Config(){};

    void parse_arg(int argc, char*argv[]);

    //端口号
    int PORT;

    //数据库连接池数量
    int sql_num;

    //线程池内的线程数量
    int thread_num;
};

#endif