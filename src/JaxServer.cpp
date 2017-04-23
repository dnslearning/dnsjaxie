
#include "JaxServer.hpp"

JaxServer::JaxServer() {
  bindAddress.sin6_family = AF_INET6;
  bindAddress.sin6_addr = in6addr_any;
  bindAddress.sin6_port = htons(14222);
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

  for (auto it = clients.begin(); it != clients.end(); ++it) {
    class JaxClient& client = *it;

    if (FD_ISSET(client.outboundSocket, &active)) {
      client.recvAnswer(*this);
    } else {
      client.timeout();
    }

    if (!client.outboundSocket) {
      it = clients.erase(it);
      if (client.originalSocket) { FD_CLR(client.originalSocket, &readFileDescs); }
    }
  }

  return activity > 0;
}

void JaxServer::tickListener() {
  while (isRunning()) {
    if (!recvQuestion()) { return; }
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

  struct JaxPacket packet;
  packet.pos = 0;
  packet.input = recvBuffer;
  packet.inputSize = recvSize;

  if (!parser.decode(packet)) {
    Jax::debug("Skipping a DNS packet");
    return false;
  }

  for (auto s : parser.domains) {
    Jax::debug("Domain: %s", s.c_str());
  }
  
  if (parser.isResponse()) {
    Jax::debug("Skipping DNS packet on inbound socket because isResponse");
    return false;
  }

  recvQuestion(packet, senderAddress, packetInfo->ipi6_addr);
  return true;
}

void JaxServer::recvQuestion(
  struct JaxPacket& p,
  struct sockaddr_in6& senderAddress, struct in6_addr& recvAddress
) {
  if (!model.fetch(recvAddress)) {
    sendFakeResponse(senderAddress);
    return;
  }

  class JaxClient client;
  client.addr = senderAddress;
  client.outboundSocket = createOutboundSocket();
  client.originalSocket = client.outboundSocket;
  clients.push_back(client);
  FD_SET(client.outboundSocket, &readFileDescs);
}

void JaxServer::sendFakeResponse(struct sockaddr_in6& addr) {

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

void JaxServer::sendResponse(const char *buffer, unsigned int bufferSize, struct sockaddr_in6& addr) {
  if (!sock) {
    Jax::debug("Not sending a packet because socket is gone");
    return;
  }

  int ok = sendto(
    sock,
    buffer, bufferSize,
    0, // flags
    (sockaddr*)&addr, sizeof(addr)
  );
  if (ok < 0) { throw Jax::socketError("Cannot send response packet"); }
}
