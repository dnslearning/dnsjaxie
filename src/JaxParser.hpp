
#pragma once

#include "Jax.hpp"

struct JaxDnsHeader {
  unsigned short id;
  unsigned short flags;
  unsigned short qdcount; // Question Section
  unsigned short ancount; // Answer Section
  unsigned short nscount; // Authority Section
  unsigned short arcount; // Additional Section
} __attribute__ ((packed));

struct JaxDnsQuestionHeader {
  unsigned short qtype;
  unsigned short qclass;
} __attribute__ ((packed));

struct JaxDnsQuestion {
  struct JaxDnsQuestionHeader header;
  std::string domain;
};

struct JaxDnsAnswerHeader {
  unsigned short atype;
  unsigned short aclass;
  unsigned int ttl;
  unsigned short dataLen;
} __attribute__ ((packed));

struct JaxDnsAnswer {
  struct JaxDnsAnswerHeader header;
  std::string domain;
  bool ipv6;
  in_addr addr4;
  in6_addr addr6;
};

class JaxParser {
public:
  struct JaxDnsHeader header;
  std::list<struct JaxDnsQuestion> questions;
  std::list<struct JaxDnsAnswer> answers;

  bool decode(struct JaxPacket& p);
  void decodeQuestion(struct JaxPacket& p);
  void decodeAnswer(struct JaxPacket& p);
  bool encode(struct JaxPacket& p);
  void encodeQuestion(struct JaxPacket& p, struct JaxDnsQuestion& question);
  void encodeAnswer(struct JaxPacket& p, struct JaxDnsAnswer& answer);

  bool isResponse() { return header.flags & FLAG_RESPONSE; }
  bool isAuthAnswer() { return header.flags & FLAG_AUTH_ANSWER; }
  bool isTruncated() { return header.flags & FLAG_TRUNCATED; }
  bool isRecursionDesired() { return header.flags & FLAG_RECURSION_DESIRED; }
  bool isRecursionAvail() { return header.flags & FLAG_RECURSION_AVAILABLE; }

  static unsigned char readByte(struct JaxPacket& p);
  static unsigned char peekByte(struct JaxPacket p);
  static void readData(struct JaxPacket& p, void *buffer, unsigned int len);
  static std::string readString(struct JaxPacket& p);
  static std::string readStringLiteral(struct JaxPacket& p);
  static std::string peekStringLiteral(struct JaxPacket p, unsigned int pos);
  static std::string readStringCompressed(struct JaxPacket& p);
  static void writeByte(struct JaxPacket& p, char c);
  static void writeData(struct JaxPacket& p, const void *buffer, unsigned int len);
  static void writeString(struct JaxPacket& p, std::string str);
  static void writeStringLiteral(struct JaxPacket &p, std::string str);

  static const unsigned short FLAG_RESPONSE = 1 << 15;
  static const unsigned short FLAG_AUTH_ANSWER = 1 << 10;
  static const unsigned short FLAG_TRUNCATED = 1 << 9;
  static const unsigned short FLAG_RECURSION_DESIRED = (1 << 8);
  static const unsigned short FLAG_RECURSION_AVAILABLE = (1 << 7);
};
