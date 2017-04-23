
#pragma once

#include "Jax.hpp"
#include "JaxServer.hpp"

class JaxClient {
public:
  struct sockaddr_in6 addr;
  int outboundSocket;
  int originalSocket;
  int startTime;

  JaxClient();
  ~JaxClient();
  void closeSocket();
  void recvAnswer(class JaxServer& server);
  void timeout();
};
