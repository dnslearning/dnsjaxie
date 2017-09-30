
#include "JaxClient.hpp"
#include "JaxPacket.hpp"
#include "JaxServer.hpp"

JaxClient::JaxClient() {
  startTime = time(NULL);
}

JaxClient::~JaxClient() {
  // Ensure socket is closed (add this once working and debug to ensure working correctly)
}

void JaxClient::closeSocket() {
  if (outboundSocket) {
    if (close(outboundSocket) < 0) { Jax::debug("Unable to close() client socket"); }
    outboundSocket = 0;
  }
}

void JaxClient::timeout() {
  int age = time(NULL) - startTime;

  if (age > 20) {
    closeSocket();
  }
}

void JaxClient::recvAnswer(JaxServer& server) {
  Jax::debug("Received response from real DNS server");
  char recvBuffer[1024];
  jax_zero(recvBuffer);

  int recvSize = recv(outboundSocket, recvBuffer, sizeof(recvBuffer), MSG_DONTWAIT);
  if (recvSize < 0) { throw Jax::socketError("Cannot recv()"); }
  closeSocket();

  if (recvSize == 0) {
    Jax::debug("Real DNS server sent us an empty packet");
    return;
  }

  JaxPacket packet(recvBuffer, recvSize);

  if (!server.parser.decode(packet)) {
    Jax::debug("Real DNS server sent us a packet we cannot decode");
    return;
  }

  for (auto& answer : server.parser.answers) {
    if (answer.header.rtype == 1) {
      in_addr_t *answerIp = (in_addr_t*)answer.raw.data();

      for (auto ip : server.hide) {
        if (ip == *answerIp) {
          server.sendFakeResponse(*this);
          return;
        }
      }
    }

    answer.header.ttl = 1;
  }

  packet = JaxPacket(1024);

  if (!server.parser.encode(packet)) {
    Jax::debug("Real DNS server sent us a packet we cannot encode");
    return;
  }

  server.sendResponse(*this, packet);
}
