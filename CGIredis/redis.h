#ifndef M_REDIS_H
#define M_REDIS_H

#include <stdio.h>
#include <deque>
#include <hiredis/hiredis.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../locker.h"

using namespace std;

class connection_pool {
public:
	redisContext *GetConnection();				//获取数据库连接
	bool ReleaseConnection(redisContext *conn); //释放连接
	int GetFreeConn();					        //获取连接
	void DestroyPool();					        //销毁所有连接

	//局部静态变量单例模式
	static connection_pool *GetInstance();

	void init(string url, int Port, int MaxConn); 
    
private:
	connection_pool();
	~connection_pool();

	int m_MaxConn;  //最大连接数
	int m_CurConn;  //当前已使用的连接数
	int m_FreeConn; //当前空闲的连接数
	locker lock;
	deque<redisContext *> connDeque; //连接池
	sem reserve;

public:
	string m_url;			 //主机地址
	int m_Port;		         //数据库端口号
};

class connectionRAII {
public:
	connectionRAII(redisContext **REDIS, connection_pool *connPool);
	~connectionRAII();
	
private:
	redisContext *conRAII;
	connection_pool *poolRAII;
};

#endif
