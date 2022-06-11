#include "webserver.h"

/*
WebServer::WebServer(int x) : utils(5) {
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
*/

WebServer::~WebServer() {
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);
    delete[] users;
    delete[] users_timer;
    delete m_pool;
}

void WebServer::init(int port, string user, string passWord, string databaseName, 
                     int sql_num, int thread_num) {
    m_port = port;
    m_user = user;
    m_passWord = passWord;
    m_databaseName = databaseName;
    m_sql_num = sql_num;
    m_thread_num = thread_num;
}

void WebServer::sql_pool() {
    //初始化数据库连接池
    m_connPool = connection_pool::GetInstance();
    m_connPool->init("localhost", m_user, m_passWord, m_databaseName, 3306, m_sql_num);

    //初始化数据库读取表
    users->initmysql_result(m_connPool);
}

void WebServer::thread_pool() {
    //线程池
    m_pool = new threadpool<http_conn>(m_connPool, m_thread_num);
}

void WebServer::eventListen() {
    //常规网络编程
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);

    //优雅关闭连接
    struct linger tmp = {1, 1};
    //level：选项定义的层次SOL_SOCKET 1 
    setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    int ret = 0;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    int flag = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    if(ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        spdlog::error("bind() error");
    }
    if(ret = listen(m_listenfd, 5) == -1) {
        spdlog::error("listen() error");
    }
  
    utils.init(TIMESLOT);

    //epoll创建内核事件表
    epoll_event events[MAX_EVENT_NUMBER];
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);
    ////将listenfd放在epoll树上
    utils.addfd(m_epollfd, m_listenfd, false);
    ////将上述epollfd赋值给http类对象的m_epollfd属性
    http_conn::m_epollfd = m_epollfd;
    //流管道 全双工 新创建的2个套接字描述符作为 m_pipefd[0] m_pipefd[1] 返回
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);
    //设置流管道写端为非阻塞 
    utils.setnonblocking(m_pipefd[1]);
    //设置流管道读端为ET非阻塞
    utils.addfd(m_epollfd, m_pipefd[0], false);
    //传递给主循环的信号值，关注SIGALRM（时间到了触发）和SIGTERM（kill会触发，Ctrl+C）
    utils.addsig(SIGPIPE, SIG_IGN);//?
    utils.addsig(SIGALRM, utils.sig_handler, false);
    utils.addsig(SIGTERM, utils.sig_handler, false);
    //每隔TIMESLOT时间触发SIGALRM信号
    alarm(TIMESLOT);

    //工具类,信号和描述符基础操作
    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd;
}

void WebServer::timer(int connfd, struct sockaddr_in client_address) {
    users[connfd].init(connfd, client_address, m_root, m_user, m_passWord, m_databaseName);

    //初始化client_data数据

    //初始化该连接对应的连接资源
    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;
    //创建定时器临时变量
    heap_timer *timer = new heap_timer(5);
    //设置定时器对应的连接资源
    timer->user_data = &users_timer[connfd];
    //设置回调函数
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    //设置绝对超时时间
    timer->expire = cur + 3 * TIMESLOT;
    //创建该连接对应的定时器，初始化为前述临时变量
    users_timer[connfd].timer = timer;
    //将该定时器添加到堆中
    utils.m_timer_lst.add_timer(timer);
}

//若有数据传输，则将定时器往后延迟3个单位 TODO
//并对新的定时器在链表上的位置进行调整
void WebServer::adjust_timer(heap_timer *timer) {
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    utils.m_timer_lst.adjust_timer(timer);
    spdlog::info("adjust timer once");
}

//服务器端关闭连接，移除对应的定时器
void WebServer::deal_timer(heap_timer *timer, int sockfd) {
    timer->cb_func(&users_timer[sockfd]);
    if (timer) {
        utils.m_timer_lst.del_timer(timer);
    }
    spdlog::info("close fd{0}", users_timer[sockfd].sockfd);
}

bool WebServer::dealclinetdata() {
    //初始化客户端连接地址
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    //ET listenfd
    while (1) {
        int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
        if (connfd < 0) {
            spdlog::error("accept error:errno is {0}", errno);
            break;
        }
        if (http_conn::m_user_count >= MAX_FD) {
            utils.show_error(connfd, "Internal server busy");
            spdlog::error("Internal server busy");
            break;
        }
        timer(connfd, client_address);
    }
    return false;
}

bool WebServer::dealwithsignal(bool &timeout, bool &stop_server) {
    int ret = 0;
    int sig;
    char signals[1024];
    //从管道读端读出信号值，成功返回字节数，失败返回-1
    //正常情况下，这里的ret返回值总是1，只有14和15两个ASCII码对应的字符
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    if (ret == -1) {
        return false;
    }
    else if (ret == 0) {
        return false;
    }
    else {
        //处理信号值对应的逻辑
        for (int i = 0; i < ret; ++i) {
            //switch的变量一般为字符或整型，当switch的变量为字符时，case中可以是字符
            //也可以是字符对应的ASCII码。
            //信号本身是整型数值，管道中传递的是ASCII码表中整型数值对应的字符
            switch (signals[i]) {
            case SIGALRM:
            {
                timeout = true;
                break;
            }
            case SIGTERM:
            {
                stop_server = true;
                break;
            }
            }
        }
    }
    return true;
}

//处理读事件时，若某连接上发生读事件，将对应定时器向后移动，否则，执行定时事件
void WebServer::dealwithread(int sockfd) {
    //创建定时器临时变量，将该连接对应的定时器取出来
    heap_timer *timer = users_timer[sockfd].timer;

    //reactor
    if (timer) {
        adjust_timer(timer);
    }

    //若监测到读事件，将该事件放入请求队列
    m_pool->append(users + sockfd, 0);

    while (true) {
        if (users[sockfd].improv == 1) {  //?
            if (users[sockfd].timer_flag == 1) {
                deal_timer(timer, sockfd);
                users[sockfd].timer_flag = 0;
            }
            users[sockfd].improv = 0;
            break;
        }
    }
}
//处理写事件时，若服务器通过某连接给浏览器发送数据，将对应定时器向后移动，否则，执行定时事件
void WebServer::dealwithwrite(int sockfd) {
    heap_timer *timer = users_timer[sockfd].timer;
    //reactor
    if (timer) {
        adjust_timer(timer);
    }

    m_pool->append(users + sockfd, 1);

    while (true) {
        if (users[sockfd].improv == 1) {
            if (users[sockfd].timer_flag == 1) {
                deal_timer(timer, sockfd);
                users[sockfd].timer_flag = 0;
            }
            users[sockfd].improv = 0;
            break;
        }
    }
}

void WebServer::eventLoop() {
    bool timeout = false;   //超时标志
    bool stop_server = false;

    while (!stop_server)
    {
        ////等待所监控文件描述符上有事件的产生
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            spdlog::error("epoll failure");
            break;
        }
        //对所有就绪事件进行处理
        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;

            //处理新到的客户连接
            if (sockfd == m_listenfd) {
                bool flag = dealclinetdata();
                if (flag == false)
                    continue;
            }
            //处理异常事件
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                //服务器端关闭连接，移除对应的定时器
                heap_timer *timer = users_timer[sockfd].timer;
                deal_timer(timer, sockfd);
            }
            //处理信号 管道读端对应文件描述符发生读事件
            else if ((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN)) {
                bool flag = dealwithsignal(timeout, stop_server);
                if (flag == false)
                    spdlog::error("dealclientdata failure");
            }
            //处理客户连接上接收到的数据
            else if (events[i].events & EPOLLIN) {
                dealwithread(sockfd);
            }
            else if (events[i].events & EPOLLOUT) {
                dealwithwrite(sockfd);
            }
        }
        //处理定时器为非必须事件，收到信号并不是立马处理，完成读写事件再处理
        if (timeout) {
            utils.timer_handler();
            spdlog::info("timer tick");

            timeout = false;
        }
    }
}