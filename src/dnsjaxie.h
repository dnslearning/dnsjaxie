
#ifndef DNSJAXIE_H
#define DNSJAXIE_H

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

  // What hostname (in6addr_any) and port we listen on
  struct sockaddr_in6 bindAddress;

  // IP address of sender
  struct in6_addr senderAddress;

  // IP address of local network interface that received packet
  struct sockaddr_in6 recvAddress;

  int bindPort;
  int recvSize;

};

void dnsjaxie_init(struct dnsjaxie_t *jax);
void dnsjaxie_listen(struct dnsjaxie_t *jax);
void dnsjaxie_tick(struct dnsjaxie_t *jax);
void dnsjaxie_close(struct dnsjaxie_t *jax);

#endif
