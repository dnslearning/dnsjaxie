
#ifndef DNSJAXIE_H
#define DNSJAXIE_H

#define JAX_DEBUG_ENABLE 1

// This ensure we get various headers like in6_addr and it does NOT imply
// change in licensing see the link:
// http://stackoverflow.com/questions/5582211/what-does-define-gnu-source-imply
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <mysql.h>

// Maximum size of a UDP packet we can receive (64KB)
#define JAX_MAX_PACKET_SIZE (1024 * 64)

#define JAX_ERROR_BUFFER_SIZE (1024)

class dnsjaxie {
private:
  // Any error that has occurred and a spare buffer to print more complex errors
  const char *errorString = NULL;
  char errorBuffer[JAX_ERROR_BUFFER_SIZE];

  // UDP socket and a 64KB receive buffer
  int sock = 0;

  // What hostname (in6addr_any) and port we listen on
  struct sockaddr_in6 bindAddress;

  // IP address of sender
  struct in6_addr senderAddress;

  // IP address of local network interface that received packet
  struct sockaddr_in6 recvAddress;

  MYSQL *mysql = NULL;
public:
  // Clear this to ask dnsjaxie to exit (like via SIGINT)
  volatile int running = 1;

  dnsjaxie();
  ~dnsjaxie();

  void run();
  void setOptions(const int argc, const char *argv[]);
  void config();
  void listen();
  void tick();
  void close();
  bool hasError();
  void error(const char *format, ...);
  const char* getError();
  void debug(const char *format, ...);
};

// Simple zero fixed size structure
#define jax_zero(s) memset(&(s), 0, sizeof(s));

#endif
