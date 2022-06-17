#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <deque>
#include <pthread.h>
#include <iostream>
#include "sql_connection_pool.h"

using namespace std;

connection_pool::connection_pool() {
	m_CurConn = 0;
	m_FreeConn = 0;
}

connection_pool *connection_pool::GetInstance() {
	static connection_pool connPool;
	return &connPool;
}

//初始化
void connection_pool::init(string url, string User, string PassWord, string DBName, int Port, int MaxConn) {
	m_url = url;
	m_Port = Port;
	m_User = User;
	m_PassWord = PassWord;
	m_DatabaseName = DBName;

	for (int i = 0; i < MaxConn; i++) {
		MYSQL *con = NULL;
		con = mysql_init(con);

		if (con == NULL) {
            spdlog::error("MySQL error");
			exit(1);
		}
		con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(), Port, NULL, 0);

		if (con == NULL) {
			spdlog::error("MySQL error");
			exit(1);
		}
		connDeque.push_back(con);
		++m_FreeConn;
	}

	reserve = sem(m_FreeConn);

	m_MaxConn = m_FreeConn;
}

//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *connection_pool::GetConnection() {
	MYSQL *con = NULL;

	if (connDeque.size() == 0)
		return NULL;

	reserve.wait();
	
	lock.lock();

	con = connDeque.front();
	connDeque.pop_front();

	--m_FreeConn;
	++m_CurConn;

	lock.unlock();
	return con;
}

//释放当前使用的连接
bool connection_pool::ReleaseConnection(MYSQL *con) {
	if (NULL == con)
		return false;

	lock.lock();

	connDeque.push_back(con);
	++m_FreeConn;
	--m_CurConn;

	lock.unlock();

	reserve.post();
	return true;
}

//销毁数据库连接池
void connection_pool::DestroyPool() {

	lock.lock();
	if (connDeque.size() > 0)
	{
		deque<MYSQL *>::iterator it;
		for (it = connDeque.begin(); it != connDeque.end(); ++it)
		{
			MYSQL *con = *it;
			mysql_close(con);
		}
		m_CurConn = 0;
		m_FreeConn = 0;
		connDeque.clear();
	}
	lock.unlock();
}

//当前空闲的连接数
int connection_pool::GetFreeConn() {
	return this->m_FreeConn;
}

connection_pool::~connection_pool() {
	DestroyPool();
}

connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool) {
	*SQL = connPool->GetConnection();
	
	conRAII = *SQL;
	poolRAII = connPool;
}

connectionRAII::~connectionRAII() {
	poolRAII->ReleaseConnection(conRAII);
}