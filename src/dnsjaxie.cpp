
#include "dnsjaxie.hpp"

dnsjaxie::dnsjaxie() {
  running = 1;

  jax_zero(bindAddress);
  bindAddress.sin6_port = htons(14222);
  bindAddress.sin6_family = AF_INET6;
  bindAddress.sin6_addr = in6addr_any;

  jax_zero(realDnsAddress);
  realDnsAddress.sin_port = htons(53);
  realDnsAddress.sin_family = AF_INET;
  inet_pton(AF_INET, "8.8.8.8", &realDnsAddress.sin_addr);
}

dnsjaxie::~dnsjaxie() {
  if (sock) {
    close(sock);
    sock = 0;
  }

  for (auto it : jobs) {
    close(it.outboundSocket);
  }

  jobs.clear();
}

void dnsjaxie::run() {
  config();
  listen();

  debug("Starting main loop");
  while (isRunning()) { tick(); }

  debug("Cleaning up");
}

void dnsjaxie::setOptions(const int argc, char* const argv[]) {
  if (argc <= 1) {
    return;
  }

  int c;

  while ((c = getopt (argc, argv, "f:")) != -1) {
    switch (c) {
    case 'f':
      configPath = std::string(optarg);
      break;
    case '?':
      if (optopt == 'f') {
        error("Option -f needs an argument");
      } else if (isprint (optopt)) {
        error("Unknown option '-%c'", optopt);
      } else {
        error("Unknown option %x (hex)", optopt);
      }

      return;
    default:
      error("There was a problem parsing options");
      return;
    }
  }
}

bool dnsjaxie::hasError() {
  return errorString != NULL;
}

bool dnsjaxie::isRunning() {
  return running && !hasError();
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
  if (!configFile()) { return; }
  if (!configMysql()) { return; }
}

bool dnsjaxie::configFile() {
  FILE *fp = fopen(configPath.c_str(), "r");

  if (!fp) {
    error("Unable to open config file %s", configPath.c_str());
    return false;
  }

  char lineBuffer[1024];
  jax_zero(lineBuffer);

  while (!feof(fp)) {
    configFileLine(fgets(lineBuffer, sizeof(lineBuffer), fp));
  }

  fclose(fp);
  fp = NULL;
  return true;
}

void dnsjaxie::configFileLine(const char *line) {
  if (line == NULL || strlen(line) <= 0) {
    return;
  }

  char keyBuffer[1024];
  char valueBuffer[1024];
  jax_zero(keyBuffer);
  jax_zero(valueBuffer);

  int matched = sscanf(line, "%1023s %1023s", keyBuffer, valueBuffer);

  if (matched < 2) {
    error("Unable to load config line (%d) %s", matched, line);
    return;
  }

  if (strcmp(keyBuffer, "dbuser") == 0) {
    dbUser = std::string(valueBuffer);
  } else if (strcmp(keyBuffer, "dbpass") == 0) {
    dbPass = std::string(valueBuffer);
  } else if (strcmp(keyBuffer, "dbname") == 0) {
    dbName = std::string(valueBuffer);
  } else if (strcmp(keyBuffer, "dbhost")) {
    dbHost = std::string(valueBuffer);
  } else {
    debug("Unknown config option: %s", keyBuffer);
  }
}

bool dnsjaxie::configMysql() {
  sqlDriver = get_driver_instance();

  if (!sqlDriver) {
    error("Unable to load SQL driver");
    return false;
  }

  try {
    sqlConnection = sqlDriver->connect(dbHost, dbUser, dbPass);
    sqlConnection->setSchema(dbName);
  } catch (sql::SQLException &e) {
    error("Unable to connect to MySQL: %s", e.what());
    return false;
  }

  return true;
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

  FD_ZERO(&readFileDescs);

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

  // Add the main socket to the set of sockets
  FD_SET(sock, &readFileDescs);
}

void dnsjaxie::tick() {
  while (isRunning()) {
    if (!tickSelect()) {
      break;
    }
  }
}

bool dnsjaxie::tickSelect() {
  fd_set active = readFileDescs;
  int maxFileDesc = sock;
  for (auto job : jobs) { maxFileDesc = std::max(maxFileDesc, job.outboundSocket); }
  int activity = select(maxFileDesc + 1, &active, NULL, NULL, NULL);

  if (activity < 0) {
    error("select() failed: %s", strerror(errno));
    return false;
  }

  if (FD_ISSET(sock, &active)) {
    recvRequestAll();
  }

  std::list<struct Job>::iterator it;

  for (it = jobs.begin(); it != jobs.end(); ++it) {
    struct Job job = *it;
    bool isActive = FD_ISSET(job.outboundSocket, &active);
    bool isTimeout = false;

    if (isActive || isTimeout) {
      if (isActive) {
        recvResponse(job);
      }

      debug("Closing socket");
      close(job.outboundSocket);
      it = jobs.erase(it);
    }
  }

  return activity > 0;
}

// Parse all incoming DNS requests
void dnsjaxie::recvRequestAll() {
  while (isRunning()) {
    if (!recvRequest()) {
      return;
    }
  }
}

// Parse a single DNS request (returns false if nothing was done)
bool dnsjaxie::recvRequest() {
  // Memory to store the "control data"
  char controlData[CMSG_SPACE(sizeof(struct in6_pktinfo))];
  jax_zero(controlData);

  // Memory to store UDP packet
  char recvBuffer[JAX_MAX_PACKET_SIZE];
  jax_zero(recvBuffer);

  struct sockaddr_in6 senderAddress;
  jax_zero(senderAddress)

  // Absurd amount of ceremony for receiving a packet
  struct msghdr msg;
  jax_zero(msg);
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
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      error("Socket error after recvmsg: %s", strerror(errno));
    }

    return false;
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

  if (!packetInfo) {
    debug("Cannot determine destination IP address");
    return false;
  }

  recvRequestPacket(recvBuffer, recvSize, senderAddress, packetInfo->ipi6_addr);
  return true;
}

// Receive a DNS packet from a client
void dnsjaxie::recvRequestPacket(
  char *buffer, int bufferSize,
  struct sockaddr_in6& senderAddress, struct in6_addr& recvAddress
) {
  char ipString[INET6_ADDRSTRLEN];
  jax_zero(ipString);

  inet_ntop(AF_INET6, &recvAddress, ipString, sizeof(ipString));
  debug("Received DNS packet (%d) for address %s", bufferSize, ipString);

  // Keep track of the DNS request
  Job job;
  job.when = time(NULL);
  job.clientAddress = senderAddress;
  job.bindAddress = randomAddress();
  job.outboundSocket = createOutboundSocket(job.bindAddress);

  // Send the packet to the real DNS server
  if (!forwardPacket(job.outboundSocket, buffer, bufferSize)) {
    debug("Unable to forward DNS packet");
    return;
  }

  // Add to job list and register for select()
  jobs.push_back(job);
  FD_SET(job.outboundSocket, &readFileDescs);

  // TODO validate incoming DNS packet
  // TODO fetch device by IPv6 address
  // TODO insert device activity
}

// Creates an IPv4 socket bound to a random port on any address
int dnsjaxie::createOutboundSocket(struct sockaddr_in& addr) {
  int outboundSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (outboundSocket <= 0) {
    error("Unable to open an outbound socket");
    return 0;
  }

  int ok = bind(outboundSocket, (struct sockaddr*)&addr, sizeof(addr));

  if (ok < 0) {
    close(outboundSocket);
    error("Unable to bind socket to port %d", ntohs(addr.sin_port));
    return 0;
  }

  return outboundSocket;
}

// Send a DNS packet to the real server like Google
bool dnsjaxie::forwardPacket(int outboundSocket, const char *buffer, int bufferSize) {
  debug("Forwarding a packet (%d bytes)", bufferSize);

  return sendto(
    outboundSocket,
    buffer, bufferSize,
    0,
    (struct sockaddr*)&realDnsAddress, sizeof(realDnsAddress)
  ) >= 0;
}

// Generate a random port number
sockaddr_in dnsjaxie::randomAddress() {
  sockaddr_in addr;
  jax_zero(addr);
  addr.sin_port = htons(14223 + (rand() & 0xFFF));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  return addr;
}

void dnsjaxie::recvResponse(Job& job) {
  debug("Received a response from the real server");

  char buffer[JAX_MAX_PACKET_SIZE];
  jax_zero(buffer);

  int bufferSize = recv(job.outboundSocket, buffer, sizeof(buffer), MSG_DONTWAIT);

  if (bufferSize == -1) {
    debug("Unable to read socket: %s", strerror(errno));
    return;
  } else if (bufferSize == 0) {
    debug("Real DNS server sent us an empty packet");
    return;
  }

  sendto(
    sock,
    buffer, bufferSize,
    0,
    (struct sockaddr*)&job.clientAddress, sizeof(job.clientAddress)
  );
}
