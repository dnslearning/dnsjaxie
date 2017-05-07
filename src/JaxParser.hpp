
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
  JaxDnsHeader header;
  std::list<JaxDnsQuestion> questions;
  std::list<JaxDnsAnswer> answers;

  bool decode(JaxPacket& p);
  void decodeQuestion(JaxPacket& p);
  void decodeAnswer(JaxPacket& p);
  bool encode(JaxPacket& p);
  void encodeQuestion(JaxPacket& p, JaxDnsQuestion& question);
  void encodeAnswer(JaxPacket& p, JaxDnsAnswer& answer);

  bool isResponse() { return header.flags & FLAG_RESPONSE; }
  bool isAuthAnswer() { return header.flags & FLAG_AUTH_ANSWER; }
  bool isTruncated() { return header.flags & FLAG_TRUNCATED; }
  bool isRecursionDesired() { return header.flags & FLAG_RECURSION_DESIRED; }
  bool isRecursionAvail() { return header.flags & FLAG_RECURSION_AVAILABLE; }

  static unsigned char readByte(JaxPacket& p);
  static unsigned char peekByte(JaxPacket p);
  static void readData(JaxPacket& p, void *buffer, unsigned int len);
  static std::string readString(JaxPacket& p);
  static std::string peekString(JaxPacket p, unsigned int pos);
  static std::string readStringLiteral(JaxPacket& p);
  static std::string readStringCompressed(JaxPacket& p);
  static void writeByte(JaxPacket& p, char c);
  static void writeData(JaxPacket& p, const void *buffer, unsigned int len);
  static void writeString(JaxPacket& p, std::string str);
  static void writeStringLiteral(JaxPacket &p, std::string str);

  static const unsigned short FLAG_RESPONSE = 1 << 15;
  static const unsigned short FLAG_AUTH_ANSWER = 1 << 10;
  static const unsigned short FLAG_TRUNCATED = 1 << 9;
  static const unsigned short FLAG_RECURSION_DESIRED = (1 << 8);
  static const unsigned short FLAG_RECURSION_AVAILABLE = (1 << 7);
};
