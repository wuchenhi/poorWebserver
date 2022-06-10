#ifndef m_CONNECTION_POOL_
#define m_CONNECTION_POOL_

#include <stdio.h>
#include <deque>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "spdlog/spdlog.h"
#include "../locker.h"

using namespace std;

class connection_pool {
public:
	MYSQL *GetConnection();				 //获取数据库连接
	bool ReleaseConnection(MYSQL *conn); //释放连接
	int GetFreeConn();					 //获取连接
	void DestroyPool();					 //销毁所有连接

	//局部静态变量单例模式
	static connection_pool *GetInstance();

	void init(string url, string User, string PassWord, string DataBaseName, int Port, int MaxConn); 

	string m_url;			 //主机地址
	string m_Port;		     //数据库端口号
	string m_User;		     //登陆数据库用户名
	string m_PassWord;	     //登陆数据库密码
	string m_DatabaseName;   //使用数据库名

private:
	connection_pool();
	~connection_pool();

	int m_MaxConn;  //最大连接数
	int m_CurConn;  //当前已使用的连接数
	int m_FreeConn; //当前空闲的连接数
	locker lock;
	deque<MYSQL *> connDeque; //连接池
	sem reserve;
};

class connectionRAII {
public:
	connectionRAII(MYSQL **con, connection_pool *connPool);
	~connectionRAII();
	
private:
	MYSQL *conRAII;
	connection_pool *poolRAII;
};

#endif
