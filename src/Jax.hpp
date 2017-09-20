
#pragma once

#define JAX_DEBUG_ENABLED 1

#include <exception>
#include <stdexcept>
#include <utility>
#include <string>
#include <list>
#include <unordered_map>
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
  static void split(const std::string& s, char delim, Out result);
  static std::vector<std::string> split(const std::string& s, char delim);
  static std::string toString(struct in6_addr& addr);
  static std::string toString(std::vector<std::string>& parts, const char *delim);
  static std::string toString(std::vector<char>& v);
  static bool isFakeIPv6(std::string s);
  static std::string convertFakeIPv6(std::string s);
  static std::vector<char> toVector(std::string s);
  static std::string toTopLevel(std::string s);
};

struct JaxDomain {
  std::string host;
  bool allow;
  bool deny;
  bool ignore;
};

typedef class App App;
typedef class JaxServer JaxServer;
typedef class JaxClient JaxClient;
typedef class JaxParser JaxParser;
typedef class JaxModel JaxModel;
typedef class JaxPacket JaxPacket;

typedef struct JaxDnsHeader JaxDnsHeader;
typedef struct JaxDnsResourceHeader JaxDnsResourceHeader;
typedef struct JaxDnsResource JaxDnsResource;
typedef struct JaxDomain JaxDomain;

#define jax_zero(s) memset(&(s), 0, sizeof(s));
