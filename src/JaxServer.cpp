
#include "JaxServer.hpp"
#include "JaxPacket.hpp"

JaxServer::JaxServer() {
  bindAddress.sin6_family = AF_INET6;
  bindAddress.sin6_addr = in6addr_any;
  bindAddress.sin6_port = htons(14222);
  realDnsAddr.sin_family = AF_INET;
  realDnsAddr.sin_port = htons(53);
  realDnsAddr.sin_addr.s_addr = inet_addr("8.8.8.8");
}

JaxServer::~JaxServer() {
  closeSocket();
}

void JaxServer::closeSocket() {
  if (sock) {
    if (close(sock) < 0) { Jax::debug("close() for socket failed"); }
    sock = 0;
  }
}

void JaxServer::listen() {
  if (sock) { throw std::runtime_error("Socket already exists"); }
  FD_ZERO(&readFileDescs);
  Jax::debug("Opening inbound socket");
  sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
  if (sock <= 0) { throw Jax::socketError("Unable to open inbound socket"); }
  int ok = bind(sock, (struct sockaddr*)&bindAddress, sizeof(bindAddress));
  if (ok < 0) { throw Jax::socketError("Unable to bind inbound socket"); }
  const int yes = 1;
  ok = setsockopt(sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &yes, sizeof(yes));
  if (ok < 0) { throw Jax::socketError("Unable to set IPV6_RECVPKTINFO"); }
  FD_SET(sock, &readFileDescs);
  model.prepare();
}

void JaxServer::tick() {
  while (isRunning()) {
    if (!tickSelect()) { return; }
  }
}

bool JaxServer::tickSelect() {
  fd_set active = readFileDescs;
  int maxFileDesc = sock;
  for (auto client : clients) { maxFileDesc = std::max(maxFileDesc, client.outboundSocket); }
  int activity = select(maxFileDesc + 1, &active, NULL, NULL, NULL);
  if (activity < 0) { throw Jax::socketError("Unable to select() sockets"); }

  if (FD_ISSET(sock, &active)) {
    tickListener();
  }

  for (auto it = clients.begin(); it != clients.end(); ) {
    JaxClient& client = *it;

    if (FD_ISSET(client.outboundSocket, &active)) {
      try {
        client.recvAnswer(*this);
      } catch (std::exception& e) {
        Jax::debug("Error while recvAnswer(): %s", e.what());
        client.closeSocket();
      }
    } else {
      client.timeout();
    }

    if (!client.outboundSocket) {
      if (client.originalSocket) { FD_CLR(client.originalSocket, &readFileDescs); }
      it = clients.erase(it);
    } else {
      ++it;
    }
  }

  return activity > 0;
}

void JaxServer::tickListener() {
  while (isRunning()) {
    try {
      if (!recvQuestion()) { return; }
    } catch (std::exception& e) {
      Jax::debug("Error while recvQuestion(): %s", e.what());
      break;
    }
  }
}

bool JaxServer::recvQuestion() {
  char controlData[CMSG_SPACE(sizeof(struct in6_pktinfo))];
  char recvBuffer[1024];
  struct sockaddr_in6 senderAddress;
  struct msghdr msg;
  struct in6_pktinfo *packetInfo = NULL;
  struct cmsghdr *cmsg = NULL;

  jax_zero(controlData);
  jax_zero(recvBuffer);
  jax_zero(senderAddress);
  jax_zero(msg);

  // Prepare to receive a packet
  msg.msg_name = (struct sockaddr*)&senderAddress;
  msg.msg_namelen = sizeof(senderAddress);
  struct iovec iov[1];
  iov[0].iov_base = &recvBuffer;
  iov[0].iov_len = sizeof(recvBuffer);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  msg.msg_control = &controlData;
  msg.msg_controllen = sizeof(controlData);

  // Actually receive a packet
  int recvSize = recvmsg(sock, &msg, MSG_DONTWAIT);

  if (recvSize == 0) {
    // Skip empty packets entirely
    return false;
  } else if (recvSize == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // Skip non-blocking errors
      return false;
    }

    throw Jax::socketError("Unable to recvmsg()");
  }

  // Parse control data to find IPV6_PKTINFO
  for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
    if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO) {
      packetInfo = (struct in6_pktinfo*) CMSG_DATA(cmsg);
      break;
    }
  }

  if (!packetInfo) {
    Jax::debug("Cannot determine destination IP address");
    return false;
  }

  JaxPacket packet(recvBuffer, recvSize);

  if (!parser.decode(packet)) {
    Jax::debug("Skipping a DNS packet");
    return false;
  }

  if (parser.isResponse()) {
    Jax::debug("Skipping DNS packet on inbound socket because isResponse");
    return false;
  }

  recvQuestion(packet, senderAddress, packetInfo->ipi6_addr);
  return true;
}

void JaxServer::recvQuestion(
  JaxPacket& p,
  struct sockaddr_in6& senderAddress, struct in6_addr& recvAddress
) {
  JaxClient client;
  client.id = parser.header.id;
  client.addr = senderAddress;
  client.listenAddress = recvAddress;
  client.local = Jax::toString(recvAddress);
  client.remote = Jax::toString(senderAddress.sin6_addr);
  client.ipv4 = Jax::isFakeIPv6(client.local);

  if (client.ipv4) {
    client.local = Jax::convertFakeIPv6(client.local);
    client.remote = Jax::convertFakeIPv6(client.remote);
  }

  if (!isAccessEnabled(client)) {
    sendFakeResponse(client);
    return;
  }

  client.outboundSocket = createOutboundSocket();
  client.originalSocket = client.outboundSocket;
  clients.push_back(client);
  FD_SET(client.outboundSocket, &readFileDescs);
  forwardRequest(client, p);
}

bool JaxServer::isAccessEnabled(JaxClient& client) {
  for (auto q : parser.questions) {
    JaxDomain domain;
    std::string top = Jax::toTopLevel(q.domain);
    Jax::debug("Domain: %s", q.domain.c_str());

    if (model.getDomain(top, domain)) {
      if (domain.allow) {
        return true;
      } else if (domain.deny) {
        return false;
      }
    }
  }

  if (!model.fetch(client)) {
    return false;
  }

  model.insertActivity(model.deviceId, model.learnMode);

  if (model.learnMode <=0 ) {
    for (auto q : parser.questions) {
      model.insertTimeline(model.deviceId, q.domain);
    }
  }

  return model.learnMode <= 0;
}

void JaxServer::sendFakeResponse(JaxClient& client) {
  Jax::debug("Sending fake response");

  parser.header = {};
  parser.header.id = client.id;
  parser.header.flags = JaxParser::FLAG_RESPONSE | JaxParser::FLAG_RECURSION_AVAILABLE/* | JaxParser::FLAG_NXDOMAIN */;

  parser.additional.clear();
  parser.auths.clear();
  parser.answers.clear();

  for (auto question : parser.questions) {
    JaxDnsResource answer = {};
    answer.domain = question.domain;
    answer.header.ttl = 15;
    answer.header.rtype = client.ipv4 ? 1 : 28;
    answer.header.rclass = 1;
    answer.header.dataLen = client.ipv4 ? sizeof(in_addr) : sizeof(in6_addr);

    if (client.ipv4) {
      struct in_addr addr4;
      inet_pton(AF_INET, client.local.c_str(), &addr4);
      answer.raw.resize(sizeof(addr4));
      memcpy(answer.raw.data(), &addr4, sizeof(addr4));
    } else {
      struct in6_addr addr6;
      inet_pton(AF_INET6, client.local.c_str(), &addr6);
      answer.raw.resize(sizeof(addr6));
      memcpy(answer.raw.data(), &addr6, sizeof(addr6));
    }

    parser.answers.push_back(answer);
  }

  JaxPacket packet(1024);

  if (!parser.encode(packet)) {
    Jax::debug("Failed to encode a fake response packet");
    return;
  }

  sendtofrom(sock, packet.raw.data(), packet.raw.size(), client.addr, client.listenAddress);
}

void JaxServer::forwardRequest(JaxClient& client, JaxPacket& packet) {
  int ok = sendto(
    client.outboundSocket,
    packet.raw.data(), packet.raw.size(),
    MSG_DONTWAIT,
    (sockaddr*)&realDnsAddr, sizeof(realDnsAddr)
  );

  if (ok < 0) { throw Jax::socketError("Unable to forward DNS request"); }
}

int JaxServer::createOutboundSocket() {
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(14223 + (rand() & 0xfff));
  addr.sin_addr.s_addr = INADDR_ANY;
  int outboundSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (outboundSocket <= 0) { throw Jax::socketError("Unable to open outbound socket"); }
  int ok = bind(outboundSocket, (struct sockaddr*)&addr, sizeof(addr));
  if (ok < 0) { throw Jax::socketError("Unable to bind outbound socket"); }
  return outboundSocket;
}

void JaxServer::sendResponse(JaxClient& client, JaxPacket& packet) {
  if (!sock) {
    Jax::debug("Not sending a packet because socket is gone");
    return;
  }

  Jax::debug("Sending response to client [%s]:%d", Jax::toString(client.addr.sin6_addr).c_str(), ntohs(client.addr.sin6_port));
  sendtofrom(sock, packet.raw.data(), packet.raw.size(), client.addr, client.listenAddress);
}

void JaxServer::sendtofrom(int s, const char *buffer, unsigned int bufferSize, struct sockaddr_in6& to, struct in6_addr& from) {
  if (s <= 0) { throw std::runtime_error("Missing socket"); }
  struct msghdr msg = {};
  struct in6_pktinfo *packetInfo = NULL;
  struct cmsghdr *cmsg = NULL;
  char controlData[CMSG_SPACE(sizeof(struct in6_pktinfo))];
  jax_zero(controlData);

  struct iovec iov[1];
  iov[0].iov_base = (void*)buffer;
  iov[0].iov_len = bufferSize;
  msg.msg_name = &to;
  msg.msg_namelen = sizeof(to);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  msg.msg_control = controlData;
  msg.msg_controllen = sizeof(controlData);
  msg.msg_flags = 0;
  cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = IPPROTO_IPV6;
  cmsg->cmsg_type = IPV6_PKTINFO;
  cmsg->cmsg_len = CMSG_LEN(sizeof(in6_pktinfo));
  packetInfo = (in6_pktinfo*)CMSG_DATA(cmsg);
  packetInfo->ipi6_ifindex = 0;
  packetInfo->ipi6_addr = from;
  msg.msg_controllen = cmsg->cmsg_len;

  if (sendmsg(s, &msg, MSG_DONTWAIT) < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      throw Jax::socketError("Unable to sendmsg()");
    }
  }
}
