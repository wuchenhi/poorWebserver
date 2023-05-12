CXX ?= g++

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
	CXXFLAGS += -fsanitize=address
else
    CXXFLAGS += -O2

endif

server: main.cpp  ./timer/lst_timer.cpp ./http/http_conn.cpp  ./CGIredis/redis.cpp  ./webserver/webserver.cpp ./configure/configure.cpp ./log/log.cpp
	$(CXX) -o server  $^ $(CXXFLAGS) -lpthread -lhiredis

clean:
	rm -r server
