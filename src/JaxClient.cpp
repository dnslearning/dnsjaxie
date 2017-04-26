
#include "JaxClient.hpp"

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
  char recvBuffer[1024];
  jax_zero(recvBuffer);

  int recvSize = recv(outboundSocket, recvBuffer, sizeof(recvBuffer), MSG_DONTWAIT);
  if (recvSize < 0) { throw Jax::socketError("Cannot recv()"); }

  if (recvSize == 0) {
    closeSocket();
    Jax::debug("Real DNS server sent us an empty packet");
    return;
  }

  server.sendResponse(recvBuffer, sizeof(recvBuffer), addr);
}
