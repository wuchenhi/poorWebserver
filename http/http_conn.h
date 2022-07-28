#ifndef M_HTTPCONNECTION_H
#define M_HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>
#include "spdlog/spdlog.h"
#include "../locker.h"
#include "../CGImysql/sql_connection_pool.h"
#include "../timer/lst_timer.h"

//主状态机在内部调用从状态机,从状态机将处理状态和数据传给主状态机
//客户端发出http连接请求
//从状态机读取数据,更新自身状态和接收数据,传给主状态机
//主状态机根据从状态机状态,更新自身状态,决定响应请求还是继续读取

//从状态机负责读取报文的一行，主状态机负责对该行数据进行解析
class http_conn {
public:
    //设置读取文件的名称m_real_file大小
    static const int FILENAME_LEN = 200;
    //设置读缓冲区m_read_buf大小
    static const int READ_BUFFER_SIZE = 2048;
    //设置写缓冲区m_write_buf大小
    static const int WRITE_BUFFER_SIZE = 1024;
    
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };
    /*
    //报文的请求方法，目前只有GET和POST
    enum class METHOD {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    //主状态机的状态
    enum class CHECK_STATE {
        REQUESTLINE = 0, //解析请求行
        HEADER,          //解析请求头
        CONTENT          //POST时 解析消息体
    };
    //报文解析的结果
    enum class HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    //从状态机的状态
    enum class LINE_STATUS {
        OK = 0, //完整行
        BAD,    //报文语法有误
        OPEN    //不完整行
    };
    */
public:
    http_conn() = default;
    ~http_conn() = default;

public:
    //初始化套接字地址，函数内部会调用私有方法init
    void init(int sockfd, const sockaddr_in &addr, char *, string user, string passwd, string sqlname);
    //关闭http连接
    void close_conn(bool real_close = true);
    void process();
    //读取浏览器端发来的全部数据
    bool read_once();
    bool write();
    sockaddr_in *get_address() {
        return &m_address;
    }
    //同步线程初始化数据库读取表
    void initmysql_result(connection_pool *connPool);
    int timer_flag;
    int improv;


private:
    void init();
    //从m_read_buf读取，并处理请求报文
    HTTP_CODE process_read();
    //向m_write_buf写入响应报文数据
    bool process_write(HTTP_CODE ret);
    //主状态机解析报文中的请求行数据
    HTTP_CODE parse_request_line(char *text);
    //主状态机解析报文中的请求头数据
    HTTP_CODE parse_headers(char *text);
    //主状态机解析报文中的请求内容
    HTTP_CODE parse_content(char *text);
    //生成响应报文
    HTTP_CODE do_request();
    //m_start_line是已经解析的字符
    //get_line用于将指针向后偏移，指向未处理的字符
    char *get_line() { return m_read_buf + m_start_line; };
    //从状态机读取一行，分析是请求报文的哪一部分
    LINE_STATUS parse_line();
    void unmap();

    //根据响应报文格式，生成对应8个部分，以下函数均由do_request调用
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL *mysql;
    int m_state;  //读为0, 写为1

private:
    int m_sockfd;
    sockaddr_in m_address;
    //存储读取的请求报文数据
    char m_read_buf[READ_BUFFER_SIZE];
    //缓冲区中m_read_buf中数据的最后一个字节的下一个位置
    int m_read_idx;
    //m_read_buf读取的位置m_checked_idx
    int m_checked_idx;
    //m_read_buf中已经解析的字符个数
    int m_start_line;
    //存储发出的响应报文数据
    char m_write_buf[WRITE_BUFFER_SIZE];
    //指示buffer中的长度
    int m_write_idx;
    //主状态机的状态
    CHECK_STATE m_check_state;
    //请求方法
    METHOD m_method;

    //以下为解析请求报文中对应的6个变量
    //存储读取文件的名称
    char m_real_file[FILENAME_LEN];
    char *m_url;
    char *m_version;
    char *m_host;
    int m_content_length;
    bool m_linger;
    //读取服务器上的文件地址
    char *m_file_address;
    struct stat m_file_stat;
    //io向量机制iovec
    struct iovec m_iv[2];
    int m_iv_count;
    //是否启用的POST
    int cgi;   
    //存储请求头数据
    char *m_string; 
    //剩余发送字节数
    int bytes_to_send;
    //已发送字节数
    int bytes_have_send;
    char *doc_root;

    map<string, string> m_users;
    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];
};

#endif
