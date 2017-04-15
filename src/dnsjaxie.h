
#ifndef DNSJAXIE_H
#define DNSJAXIE_H

#define DEBUG 1
#define _GNU_SOURCE

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

struct dnsjaxie_t {
  // Clear this to ask dnsjaxie to exit (like via SIGINT)
  volatile int running;

  // Any error that has occurred and a spare buffer to print more complex errors
  const char *error;
  char errorBuffer[1024];

  // UDP socket and a 64KB receive buffer
  int sock;
  char buffer[64 * 1024];
  int recvSize;

  // What hostname (in6addr_any) and port we listen on
  struct sockaddr_in6 bindAddress;

  // IP address of sender
  struct in6_addr senderAddress;

  // IP address of local network interface that received packet
  struct sockaddr_in6 recvAddress;
};

// Basic app structure
void dnsjaxie_init(struct dnsjaxie_t *jax);
void dnsjaxie_listen(struct dnsjaxie_t *jax);
void dnsjaxie_tick(struct dnsjaxie_t *jax);
void dnsjaxie_close(struct dnsjaxie_t *jax);

// Basic debugging functions that disappear during production runtime
#ifdef DEBUG
  #include <stdlib.h>
  #define dnsjaxie_debug(msg) fprintf(stderr, "DEBUG: %s\n", msg);
  #define dnsjaxie_debugf(format, ...) fprintf(stderr, format, ##__VA_ARGS__);
#else
  // Creates empty macros when DEBUG is disabled
  #define dnsjaxie_debug(msg) do { } while (0);
  #define dnsjaxie_debug(format, ...) do { } while (0);
#endif

// Set an error with formatting
#define dnsjaxie_error(jax, format, ...) \
  snprintf(jax->errorBuffer, sizeof(jax->errorBuffer), format, ##__VA_ARGS__); \
  jax->error = jax->errorBuffer;

// Simple zero fixed size structure
#define dnsjaxie_zero(s) memset(&(s), 0, sizeof(s))

#endif
