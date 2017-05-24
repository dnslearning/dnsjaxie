
#pragma once

#include "Jax.hpp"

class JaxClient {
public:
  struct sockaddr_in6 addr;
  struct in6_addr listenAddress;
  std::string local, remote;
  bool ipv4;
  int outboundSocket;
  int originalSocket;
  int startTime;
  unsigned short id;

  JaxClient();
  ~JaxClient();
  void closeSocket();
  void recvAnswer(JaxServer& server);
  void timeout();
};
