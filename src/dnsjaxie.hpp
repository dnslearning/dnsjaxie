
#ifndef DNSJAXIE_H
#define DNSJAXIE_H

#define JAX_DEBUG_ENABLE 1

// This ensure we get various headers like in6_addr and it does NOT imply
// change in licensing see the link:
// http://stackoverflow.com/questions/5582211/what-does-define-gnu-source-imply
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <list>
#include <string>
#include <algorithm>

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

// Maximum size of a UDP packet we can receive (4KB) while the DNS
// spec itself is limited to 512 bytes
#define JAX_MAX_PACKET_SIZE (1024 * 4)

struct Job {
  struct sockaddr_in6 clientAddress;
  struct sockaddr_in bindAddress;
  int outboundSocket;
  int when;
};

class dnsjaxie {
private:
  // Any error that has occurred and a spare buffer to print more complex errors
  const char *errorString = NULL;
  char errorBuffer[1024];

  // UDP socket and a 64KB receive buffer
  int sock = 0;

  // What hostname (in6addr_any) and port we listen on
  struct sockaddr_in6 bindAddress;

  // IP address of sender
  struct in6_addr senderAddress;

  // All sockets that we are waiting on
  fd_set readFileDescs;

  // All of the pending requests
  std::list<struct Job> jobs;

  // Address of the real DNS server (like Google)
  struct sockaddr_in realDnsAddress;

  // Connection to MySQL server
  std::string dbName = "dnsjaxie";
  std::string dbUser = "root";
  std::string dbPass = "";
  std::string dbHost = "localhost";
  sql::Driver *sqlDriver;
  sql::Connection *sqlConnection;
  //sql::Statement *sqlStatement;
  //sql::ResultSet *sqlResultSet;

  std::string configPath = "/etc/dnsjaxie.conf";
public:
  // Clear this to ask dnsjaxie to exit (like via SIGINT)
  volatile int running = 1;

  // Constructor and deconstructor
  dnsjaxie();
  ~dnsjaxie();

  // Error and debugging
  bool hasError();
  void error(const char *format, ...);
  const char* getError();
  void debug(const char *format, ...);

  // Accept options from command line
  void setOptions(const int argc, char* const argv[]);

  // Load configuration from /etc/dnsjaxie.conf
  void config();
  bool configFile();
  void configFileLine(const char *line);
  bool configMysql();

  // Open socket and start listening
  void listen();

  // Main loop (runs until error o)
  void run();
  bool isRunning();

  // A single tick in the main loop
  void tick();
  bool tickSelect();
  bool tickRequest(Job& job);

  // Handle all incoming DNS requests
  void recvRequestAll();
  bool recvRequest();
  void recvRequestPacket(char *buffer, int bufferSize, struct sockaddr_in6& senderAddress, struct in6_addr& recvAddress);
  int createOutboundSocket(struct sockaddr_in& addr);
  bool forwardPacket(int outboundSocket, const char *buffer, int bufferSize);

  // Handle response from real DNS server like Google
  void recvResponse(Job& job);

  // Helper method for generating a random port to send
  sockaddr_in randomAddress();
};

// Simple zero fixed size structure
#define jax_zero(s) memset(&(s), 0, sizeof(s));

#endif
