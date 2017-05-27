
#pragma once

#include "Jax.hpp"
#include "JaxPacket.hpp"

struct JaxDnsHeader {
  unsigned short id;
  unsigned short flags;
  unsigned short qdcount; // Question Section
  unsigned short ancount; // Answer Section
  unsigned short nscount; // Authority Section
  unsigned short arcount; // Additional Section
} __attribute__ ((packed));

struct JaxDnsResourceHeader {
  unsigned short rtype;
  unsigned short rclass;
  unsigned int ttl;
  unsigned short dataLen;
} __attribute__ ((packed));

struct JaxDnsResource {
  std::string domain;
  struct JaxDnsResourceHeader header;
  std::vector<char> raw;
};

class JaxParser {
public:
  JaxDnsHeader header;
  std::list<JaxDnsResource> questions;
  std::list<JaxDnsResource> answers;
  std::list<JaxDnsResource> auths;
  std::list<JaxDnsResource> additional;

  bool decode(JaxPacket& p);
  bool encode(JaxPacket& p);

  JaxDnsResource readQuestion(JaxPacket& p);
  JaxDnsResource readResource(JaxPacket& p);
  void writeQuestion(JaxPacket& p, JaxDnsResource& res);
  void writeResource(JaxPacket& p, JaxDnsResource& res);

  bool isResponse() { return header.flags & FLAG_RESPONSE; }
  bool isAuthAnswer() { return header.flags & FLAG_AUTH_ANSWER; }
  bool isTruncated() { return header.flags & FLAG_TRUNCATED; }
  bool isRecursionDesired() { return header.flags & FLAG_RECURSION_DESIRED; }
  bool isRecursionAvail() { return header.flags & FLAG_RECURSION_AVAILABLE; }

  static const unsigned short FLAG_RESPONSE = 1 << 15;
  static const unsigned short FLAG_AUTH_ANSWER = 1 << 10;
  static const unsigned short FLAG_TRUNCATED = 1 << 9;
  static const unsigned short FLAG_RECURSION_DESIRED = (1 << 8);
  static const unsigned short FLAG_RECURSION_AVAILABLE = (1 << 7);
};
