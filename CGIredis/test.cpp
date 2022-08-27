// 测试redis数据库连接池
#include <map>
#include "redis.h"

void initredis_result(connection_pool *connPool) {
    //先从连接池中取一个连接
    redisContext *redis = nullptr;
    //redis = connPool->GetConnection();
    connectionRAII rediscon(&redis, connPool);
    
    //在user表中检索username，passwd数据
    //redisCommand(redis, "EXISTS %s", map_[0].c_str());
   
    string key = "yw", value = "123";
    //redisCommand(redis, "SET %s %s", key.c_str(), value.c_str());
    
    //从表中检索完整的结果集
    redisReply* reply = static_cast<redisReply*>(redisCommand(redis, "GET %s", key.c_str()));
    if(reply->str == nullptr) {
        std::cout << "reply->str is empty" << std::endl;
        return;
    }
	std::string str = reply->str;
	freeReplyObject(reply);
    if(str.empty()) {
        std::cout << "str is empty" << std::endl;
        return;
    }
    std::cout << str << std::endl;
    
}
int main() {
    std::map<string, string> map_;
    map_.insert(pair<string, string>("wuyi", "123"));  
    connection_pool* connPool;
    connPool = connection_pool::GetInstance();
    connPool->init("127.0.0.1", 6379, 8);

    initredis_result(connPool);
}