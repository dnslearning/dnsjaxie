
#pragma once

#include "Jax.hpp"
#include "JaxClient.hpp"
#include "JaxModel.hpp"
#include "JaxParser.hpp"

typedef struct timelineRow timelineRow;

struct timelineRow {
  int id;
  std::string domain;

  bool operator==(const timelineRow &other) const {
    return (id == other.id) && (domain == other.domain);
  }
};

namespace std {
  template <>
  struct hash<timelineRow> {
    std::size_t operator()(const timelineRow& k) const {
      return std::hash<int>()(k.id) ^ std::hash<std::string>()(k.domain);
    }
  };
}

class JaxServer {
private:
  bool stopped;
  std::list<JaxClient> clients;
  int sock;
  fd_set readFileDescs;
  std::unordered_set<timelineRow> timelineQueue;
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
  void sendFakeResponse(JaxClient& client, const std::string ip);
  void sendRedirectResponse(JaxClient& client, const std::string redirect);
  bool isAccessEnabled(JaxClient& client);
  int createOutboundSocket();
  void removeSocket(int outboundSocket);

  void sendtofrom(int s, const char *buffer, unsigned int bufferSize, struct sockaddr_in6& to, struct in6_addr& from);
};
