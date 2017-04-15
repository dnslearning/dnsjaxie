
#ifndef DNSJAXIE_H
#define DNSJAXIE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/ipv6.h>
#include <linux/in6.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

struct dnsjaxie_t {
  const char *error;
  int sock;
  char buffer[64 * 1024];
  struct sockaddr_in6 bindAddress;
  struct sockaddr_in6 senderAddress;
  socklen_t senderAddressSize;
  int bindPort;
  int recvSize;
  volatile int running;
  //struct in6_pktinfo packetInfo;
};

void dnsjaxie_init(struct dnsjaxie_t *jax);
void dnsjaxie_listen(struct dnsjaxie_t *jax);
void dnsjaxie_tick(struct dnsjaxie_t *jax);
void dnsjaxie_close(struct dnsjaxie_t *jax);

#endif
