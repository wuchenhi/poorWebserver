#ifndef M_DATABASE_POOL_
#define M_DATABASE_POOL_

#include <deque>
#include <mysql/mysql.h>

class DatabasePool {
private:
    DatabasePool() {

    }


public:
    MYSQL* MakeConnect();
    bool ReleaseConnect();
    int Get
};

#endif
