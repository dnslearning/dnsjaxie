
#pragma once

#include "Jax.hpp"
#include "JaxClient.hpp"
#include "JaxModel.hpp"
#include "JaxParser.hpp"

struct timelineRow {
  int id;
  std::string domain;
};

typedef struct timelineRow timelineRow;

class JaxServer {
private:
  bool stopped;
  std::list<JaxClient> clients;
  int sock;
  fd_set readFileDescs;
  std::list<timelineRow> timelineQueue;
public:
  sockaddr_in6 bindAddress;
  JaxModel model;
  JaxParser parser;
  sockaddr_in realDnsAddr;
  std::vector<in_addr_t> hide;

  JaxServer();
  ~JaxServer();
  void stop() { stopped = 1; }
  bool isRunning() { return !stopped; }
  void closeSocket();
  void listen();
  void tick();
  void tickSelect();
  void tickListener();
  bool recvQuestion();
  void recvQuestion(JaxPacket& packet, struct sockaddr_in6& senderAddress, struct in6_addr& recvAddress);
  void sendResponse(JaxClient& client, JaxPacket& packet);
  void forwardRequest(JaxClient& client, JaxPacket& packet);
  void sendFakeResponse(JaxClient& client);
  bool isAccessEnabled(JaxClient& client);
  int createOutboundSocket();
  void removeSocket(int outboundSocket);

  void sendtofrom(int s, const char *buffer, unsigned int bufferSize, struct sockaddr_in6& to, struct in6_addr& from);
};
