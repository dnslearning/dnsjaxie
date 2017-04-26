
#pragma once

#include "Jax.hpp"
#include "JaxServer.hpp"

class JaxClient {
public:
  struct sockaddr_in6 addr;
  struct in6_addr listenAddress;
  int outboundSocket;
  int originalSocket;
  int startTime;
  unsigned short id;

  JaxClient();
  ~JaxClient();
  void closeSocket();
  void recvAnswer(class JaxServer& server);
  void timeout();
};
