
#include "dnsjaxie.h"

void dnsjaxie_init(struct dnsjaxie_t *jax) {
  if (!jax) { dnsjaxie_debug("NULL passed to dnsjaxie_init()"); return; }
  memset(jax, 0, sizeof(struct dnsjaxie_t));

  jax->running = 1;
  jax->bindAddress.sin6_port = htons(14222);
  jax->bindAddress.sin6_family = AF_INET6;
  jax->bindAddress.sin6_addr = in6addr_any;
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

  // Bind the socket to bindAddress (probably in6addr_any on some port)
  int ok = bind(jax->sock, (struct sockaddr*)&jax->bindAddress, sizeof(jax->bindAddress));
  if (ok < 0) { dnsjaxie_error(jax, "Unable to bind socket: %s", strerror(errno)); return; }

  // Values we set in socket options
  const int yes = 1;
  const struct timeval tv = { .tv_sec = 0, .tv_usec = 100000};

  #define set(level, opt, value) { \
    ok = setsockopt(jax->sock, level, opt, &(value), sizeof(value)); \
    if (ok < 0) { \
      dnsjaxie_error(jax, "Unable to setsockopt(sock, %s, %s, ...) because: %s", #level, #opt, strerror(errno)); \
      return; \
    } \
  }

  // Get destination IP of UDP packet via recvmsg()
  set(IPPROTO_IPV6, IPV6_RECVPKTINFO, yes);

  // Allow recvmsg() to timeout
  set(SOL_SOCKET, SO_RCVTIMEO, tv);
}

void dnsjaxie_tick(struct dnsjaxie_t *jax) {
  if (jax->error) { jax->running = 0; }
  if (!jax->running) { return; }

  struct iovec iov[1];
	iov[0].iov_base = &jax->buffer;
  iov[0].iov_len = sizeof(jax->buffer);

  char controlData[CMSG_SPACE(sizeof(struct in6_pktinfo))];
  dnsjaxie_zero(controlData);

  // Absurd amount of ceremony for receiving a packet
  struct msghdr msg;
  dnsjaxie_zero(msg);
	msg.msg_name = (struct sockaddr*)&jax->recvAddress;
	msg.msg_namelen = sizeof(jax->recvAddress);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = &controlData;
  msg.msg_controllen = sizeof(controlData);

  // Actually receive a packet
  jax->recvSize = recvmsg(jax->sock, &msg, 0);

  if (jax->recvSize == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) { return; }
    jax->error = strerror(errno);
    return;
  }

  if (jax->recvSize == 0) {
    return;
  }

  struct in6_pktinfo *packetInfo = NULL;
  struct cmsghdr *cmsg = NULL;

  for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
    if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO) {
      packetInfo = (struct in6_pktinfo*) CMSG_DATA(cmsg);
      break;
    }
  }

  // Cannot determine incoming IP address
  if (!packetInfo) {
    dnsjaxie_debug("Cannot determine incoming IP address");
    return;
  }

  char ipString[INET6_ADDRSTRLEN];
  dnsjaxie_zero(ipString);
  inet_ntop(AF_INET6, &(packetInfo->ipi6_addr), ipString, sizeof(ipString));

  printf("Receive: %s\n", ipString);
}
