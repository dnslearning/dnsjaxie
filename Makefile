
all:
	g++ -std=c++11 -Wall -o dnsjaxie src/*.cpp $(shell pkg-config --cflags --libs mysqlclient)
