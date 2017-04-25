
#pragma once

#include "Jax.hpp"
#include "JaxClient.hpp"
#include "JaxModel.hpp"
#include "JaxParser.hpp"

class JaxServer {
private:
  bool stopped;
  std::list<class JaxClient> clients;
  int sock;
  fd_set readFileDescs;
public:
  sockaddr_in6 bindAddress;
  class JaxModel model;
  class JaxParser parser;

  JaxServer();
  ~JaxServer();
  void stop() { stopped = 1; }
  bool isRunning() { return !stopped; }
  void closeSocket();
  void listen();
  void tick();
  bool tickSelect();
  void tickListener();
  bool recvQuestion();
  void recvQuestion(struct JaxPacket& packet, struct sockaddr_in6& senderAddress, struct in6_addr& recvAddress);
  void sendResponse(const char *buffer, unsigned int bufferSize, struct sockaddr_in6& addr);
  void sendFakeResponse(struct sockaddr_in6& addr);
  bool isAccessEnabled(struct sockaddr_in6& addr);
  int createOutboundSocket();
  void removeSocket(int outboundSocket);
};
