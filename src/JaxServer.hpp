
#pragma once

#include "Jax.hpp"
#include "JaxClient.hpp"
#include "JaxModel.hpp"
#include "JaxParser.hpp"

class JaxServer {
private:
  bool stopped;
  std::list<JaxClient> clients;
  int sock;
  fd_set readFileDescs;
public:
  sockaddr_in6 bindAddress;
  JaxModel model;
  JaxParser parser;
  sockaddr_in realDnsAddr;

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
  void recvQuestion(JaxPacket& packet, struct sockaddr_in6& senderAddress, struct in6_addr& recvAddress);
  void sendResponse(const char *buffer, unsigned int bufferSize, struct sockaddr_in6& addr);
  void forwardRequest(JaxClient& client, JaxPacket& packet);
  void sendFakeResponse(JaxClient& client);
  bool isAccessEnabled(JaxClient& client);
  int createOutboundSocket();
  void removeSocket(int outboundSocket);
};
