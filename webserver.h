#ifndef m_WEBSERVER_H
#define m_WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include "threadpool.h"
#include "./http/http_conn.h"

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时单位

class WebServer
{
public:
    WebServer(int x) {
        //http_conn类对象users
        users = new http_conn[MAX_FD];

        //root文件夹路径存在m_root里
        char server_path[200];
        getcwd(server_path, 200);//当前工作目录的绝对路径复制到server_path所指的内存空间中
        char root[6] = "/root";
        m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);
        strcpy(m_root, server_path);//char *strcpy(char *dest, const char *src)
        strcat(m_root, root);   // root 所指向的字符串追加到 m_root 所指向的字符串的结尾

        //定时器对象users_timer
        users_timer = new client_data[MAX_FD];
    }
    ~WebServer();

    void init(int port , string user, string passWord, string databaseName,
              int sql_num, int thread_num);

    void thread_pool();
    void sql_pool();
    void eventListen();
    void eventLoop();
    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(heap_timer *timer);
    void deal_timer(heap_timer *timer, int sockfd);
    bool dealclinetdata();
    bool dealwithsignal(bool& timeout, bool& stop_server);
    void dealwithread(int sockfd);
    void dealwithwrite(int sockfd);

public:
    //基础
    int m_port;
    char *m_root;

    int m_pipefd[2];
    int m_epollfd;
    http_conn *users;

    //数据库相关
    connection_pool *m_connPool;
    string m_user;         //登陆数据库用户名
    string m_passWord;     //登陆数据库密码
    string m_databaseName; //使用数据库名
    int m_sql_num;

    //线程池相关
    threadpool<http_conn> *m_pool;
    int m_thread_num;

    //epoll_event相关
    epoll_event events[MAX_EVENT_NUMBER];

    int m_listenfd;

    //定时器相关
    client_data *users_timer;
    Utils utils;
};
#endif
