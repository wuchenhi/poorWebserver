#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <deque>
#include <pthread.h>
#include <iostream>
#include "redis.h"

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
void connection_pool::init(string url, int Port, int MaxConn) {
	m_url = url;
	m_Port = Port;

	for (int i = 0; i < MaxConn; ++i) {
        redisContext* redis =  redisConnect(m_url.c_str(), m_Port);
        if(redis != nullptr &&redis ->err) {
            std::cout << "connect error: " << redis ->errstr << std::endl;
			return;
		}
		connDeque.push_back(redis);
		++m_FreeConn;
	}

	reserve = sem(m_FreeConn);

	m_MaxConn = m_FreeConn;
}

//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
redisContext *connection_pool::GetConnection() {
	if (connDeque.size() == 0)
		return nullptr;

	reserve.wait();
	
	lock.lock();

	redisContext *con = connDeque.front();
	connDeque.pop_front();

	--m_FreeConn;
	++m_CurConn;

	lock.unlock();
	return con;
}

//释放当前使用的连接
bool connection_pool::ReleaseConnection(redisContext *con) {
	if (con == nullptr) 
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
		deque<redisContext *>::iterator it;
		for (it = connDeque.begin(); it != connDeque.end(); ++it) {
			redisContext *con = *it;
			con = nullptr;
            //? 关闭连接
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

connectionRAII::connectionRAII(redisContext **REDIS, connection_pool *connPool) {
	*REDIS = connPool->GetConnection();
	
	conRAII = *REDIS;
	poolRAII = connPool;
}

connectionRAII::~connectionRAII() {
	poolRAII->ReleaseConnection(conRAII);
}