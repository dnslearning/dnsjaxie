
#include "dnsjaxie.hpp"

dnsjaxie::dnsjaxie() {
  running = 1;
  bindAddress.sin6_port = htons(14222);
  bindAddress.sin6_family = AF_INET6;
  bindAddress.sin6_addr = in6addr_any;
}

dnsjaxie::~dnsjaxie() {
  close();
}

void dnsjaxie::close() {
  if (sock) {
    ::close(sock);
    sock = 0;
  }
}

void dnsjaxie::run() {
  debug("Using MySQL version %s", mysql_get_client_info());
  config();
  listen();

  debug("Starting main loop");
  while (running) { tick(); }

  debug("Cleaning up");
  close();
}

void dnsjaxie::setOptions(const int argc, const char *argv[]) {
  if (argc <= 1) {
    return;
  }

  debug("Setting options");
}

bool dnsjaxie::hasError() {
  return errorString != NULL;
}

const char* dnsjaxie::getError() {
  return errorString;
}

void dnsjaxie::error(const char *format, ...) {
  va_list argptr;
  va_start(argptr, format);
  vsprintf(errorBuffer, format, argptr);
  va_end(argptr);
  errorString = errorBuffer;
}

void dnsjaxie::debug(const char *format, ...) {
#if JAX_DEBUG_ENABLE
  va_list argptr;
  va_start(argptr, format);
  vfprintf(stderr, format, argptr);
  va_end(argptr);
  fprintf(stderr, "\n");
#endif
}

void dnsjaxie::config() {

}

void dnsjaxie::listen() {
  int ok;

  if (hasError()) {
    debug("Cannot listen() because of an error previously");
    return;
  }

  if (sock) {
    debug("Socket already exists so refusing to listen() again");
    return;
  }

  // Open a UDP socket
  sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

  if (sock <= 0) {
    error("Unable to open socket: %s", strerror(errno));
    return;
  }

  // Bind the socket to bindAddress (probably in6addr_any on some port)
  ok = bind(sock, (struct sockaddr*)&bindAddress, sizeof(bindAddress));

  if (ok < 0) {
    error("Unable to bind socket: %s", strerror(errno));
    return;
  }

  // Values we set in socket options
  const int yes = 1;
  const struct timeval tv = { .tv_sec = 0, .tv_usec = 100000};

  ok = setsockopt(sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &yes, sizeof(yes));

  if (ok < 0) {
    error("Unable to set IPV6_RECVPKTINFO: %s", strerror(errno));
    return;
  }

  ok = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  if (ok < 0) {
    error("Unable to set SO_RCVTIMEO: %s", strerror(errno));
    return;
  }

  mysql = mysql_init(NULL);

}

void dnsjaxie::tick() {
  if (hasError()) {
    debug("Exiting because of an error %s", getError());
    running = 0;
  }

  if (!running) {
    return;
  }

  // Memory to store the "control data"
  char controlData[CMSG_SPACE(sizeof(struct in6_pktinfo))];
  jax_zero(controlData);

  // Memory to store UDP packet
  char recvBuffer[JAX_MAX_PACKET_SIZE];
  jax_zero(recvBuffer);

  // Absurd amount of ceremony for receiving a packet
  struct msghdr msg;
  jax_zero(msg);
	msg.msg_name = (struct sockaddr*)&recvAddress;
	msg.msg_namelen = sizeof(recvAddress);
  struct iovec iov[1];
  iov[0].iov_base = &recvBuffer;
  iov[0].iov_len = sizeof(recvBuffer);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = &controlData;
  msg.msg_controllen = sizeof(controlData);

  // Actually receive a packet
  int recvSize = recvmsg(sock, &msg, 0);

  if (recvSize == 0) {
    // Skip empty packets entirely
    return;
  } else if (recvSize == -1) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      error("Socket error after recvmsg: %s", strerror(errno));
    }

    return;
  }

  struct in6_pktinfo *packetInfo = NULL;
  struct cmsghdr *cmsg = NULL;

  // Parse control data to find IPV6_PKTINFO
  for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
    if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO) {
      packetInfo = (struct in6_pktinfo*) CMSG_DATA(cmsg);
      break;
    }
  }

  // Cannot determine incoming IP address
  if (!packetInfo) {
    debug("Cannot determine incoming IP address");
    return;
  }

  char ipString[INET6_ADDRSTRLEN];
  jax_zero(ipString);
  inet_ntop(AF_INET6, &(packetInfo->ipi6_addr), ipString, sizeof(ipString));

  debug("Received DNS packet (%d) from %s\n", recvSize, ipString);

  // TODO validate incoming DNS packet
  // TODO fetch device by IPv6 address
  // TODO insert device activity
  // TODO forward DNS packet
}
