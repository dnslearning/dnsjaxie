
all:
	g++ -std=c++11 -Wall \
		-I/usr/include/cppconn \
		$(shell mysql_config --cflags) \
		-o dnsjaxie src/*.cpp \
		-lmysqlcppconn \
		$(shell mysql_config --libs)

clean:
	rm dnsjaxie
