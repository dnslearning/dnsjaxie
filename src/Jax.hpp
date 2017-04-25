
#pragma once

#define JAX_DEBUG_ENABLED 1

#include <exception>
#include <stdexcept>
#include <utility>
#include <string>
#include <list>
#include <vector>
#include <iterator>
#include <fstream>
#include <sstream>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

// MySQL and libs
#include <mysql_connection.h>
#include <driver.h>

// Depending on version or platform these have to be manually included
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>


class Jax {
public:
  static std::runtime_error socketError(std::string reason);
  static void debug(const char *format, ...);

  template<typename Out>
  static void split(const std::string &s, char delim, Out result);
  static std::vector<std::string> split(const std::string &s, char delim);
};

struct JaxPacket {
  char *input;
  unsigned int inputSize;
  unsigned int pos;
};

#define jax_zero(s) memset(&(s), 0, sizeof(s));
