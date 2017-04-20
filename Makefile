
all:
	g++ -std=c++11 -Wall \
		-I/usr/include/cppconn \
		$(shell pkg-config --cflags mysqlclient) \
		-o dnsjaxie src/*.cpp \
		-lmysqlcppconn \
		$(shell pkg-config --libs mysqlclient)

build-deps:
	sudo apt install g++ libmysqlclient-dev libmysqlcppconn-dev
