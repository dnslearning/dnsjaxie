
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

  // Values we set in socket options
  const int yes = 1;
  struct timeval tv; //= { .tv_sec = 0, .tv_usec = 100000};
  tv.tv_sec = 0;
  tv.tv_usec = 100000;

  #define set(level, opt, value) {\
    ok = setsockopt(jax->sock, level, opt, &(value), sizeof(value)); \
    if (ok < 0) { \
      jax->error = strerror(errno); \
      if (!jax->error) { jax->error = "Unknown error"; } \
      snprintf(jax->errorBuffer, sizeof(jax->errorBuffer), "Unable to set socket option (%s, %s): %s", #level, #opt, jax->error); \
      jax->error = jax->errorBuffer; \
      return; \
    } \
  }
  
  set(IPPROTO_IPV6, IPV6_PKTINFO, yes);
  set(SOL_SOCKET, SO_RCVTIMEO, tv);
}

void dnsjaxie_tick(struct dnsjaxie_t *jax) {
  if (jax->error) { jax->running = 0; }
  if (!jax->running) { return; }

  struct msghdr msg;
  struct cmsghdr *cmsg;
  struct in6_pktinfo *packetInfo;
  char ipString[INET6_ADDRSTRLEN];

  struct iovec iov[1];
	iov[0].iov_base = &jax->buffer;
  iov[0].iov_len = sizeof(jax->buffer);

  union {
    char cmsg[CMSG_SPACE(sizeof(struct in_pktinfo))];
    char cmsg6[CMSG_SPACE(sizeof(struct in6_pktinfo))];
  } u;

  memset(ipString, 0, sizeof(ipString));

  // Absurd amount of ceremony for receiving a packet
  memset(&msg, 0, sizeof(msg));
	msg.msg_name = (struct sockaddr*)&jax->recvAddress;
	msg.msg_namelen = sizeof(jax->recvAddress);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = &u;
  msg.msg_controllen = sizeof(u);

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

  for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
    printf("something\n");
    if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO) {
      packetInfo = (struct in6_pktinfo*) CMSG_DATA(cmsg);
      printf("got it\n");
      inet_ntop(AF_INET6, &(packetInfo->ipi6_addr), ipString, sizeof(ipString));
      break;
    }
  }




  printf("Receive: %s\n", ipString);
}
