
#include "dnsjaxie.h"

void dnsjaxie_init(struct dnsjaxie_t *jax) {
  jax->error = NULL;
  jax->sock = 0;
  jax->recvSize = 0;
  jax->bindPort = 14222;
  jax->running = 1;

  memset(&jax->buffer, 0 , sizeof(jax->buffer));
  memset(&jax->bindAddress, 0, sizeof(jax->bindAddress));
  memset(&jax->senderAddress, 0, sizeof(jax->senderAddress));
  //memset(&jax->packetInfo, 0, sizeof(jax->packetInfo));
}

void dnsjaxie_close(struct dnsjaxie_t *jax) {
  if (jax->sock) {
    close(jax->sock);
    jax->sock = 0;
  }
}

void dnsjaxie_listen(struct dnsjaxie_t *jax) {
  if (jax->error) { return; }

  // Open a UDP socket
  jax->sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

  if (jax->sock <= 0) {
    jax->error = strerror(errno);
    return;
  }

  // Bind the socket to any incoming address
  jax->bindAddress.sin6_family = AF_INET6;
  jax->bindAddress.sin6_port = htons(jax->bindPort);
  jax->bindAddress.sin6_addr = in6addr_any;

  int ok = bind(jax->sock, (struct sockaddr*)&jax->bindAddress, sizeof(jax->bindAddress));

  if (ok < 0) {
    jax->error = strerror(errno);
    return;
  }

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 100000;
  ok = setsockopt(jax->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  if (ok < 0) {
    jax->error = "Unable to set socket timeout";
    return;
  }

  // Set the socket option saying we want extra IP header information
  //setsockopt(jax->sock, IPPROTO_IPV6, IPV6_PKTINFO, &jax->packetInfo, sizeof(jax->packetInfo));
}

void dnsjaxie_tick(struct dnsjaxie_t *jax) {
  if (jax->error) { jax->running = 0; }
  if (!jax->running) { return; }

  jax->recvSize = recvfrom(
    jax->sock,
    jax->buffer, sizeof(jax->buffer),
    0, // Flags
    (struct sockaddr*) &jax->senderAddress, &jax->senderAddressSize
  );

  if (jax->recvSize == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) { return; }
    jax->error = strerror(errno);
    return;
  }

  if (jax->recvSize == 0) {
    return;
  }

  printf("Receive: %s\n", "blah");
}
