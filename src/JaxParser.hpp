
#pragma once

#include "Jax.hpp"

struct JaxDnsHeader {
  unsigned short id;
  unsigned short flags;
  unsigned short qdcount;
  unsigned short ancount;
  unsigned short nscount;
  unsigned short arcount;
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
};

class JaxParser {
public:
  struct JaxDnsHeader header;
  //std::list<std::string> domains;
  std::list<struct JaxDnsQuestion> questions;
  std::list<struct JaxDnsAnswer> answers;

  bool decode(struct JaxPacket& p);
  void decodeQuestion(struct JaxPacket& p);
  void decodeAnswer(struct JaxPacket& p);
  bool encode(struct JaxPacket& p);
  void encodeQuestion(struct JaxPacket& p, struct JaxDnsQuestion& question);
  void encodeAnswer(struct JaxPacket& p, struct JaxDnsAnswer& answer);

  bool isEmpty();
  bool isResponse() { return header.flags & 0b1; }
  unsigned int getOpcode() { return (header.flags >> 1) & 0b11; }
  bool isAuthAnswer() { return (header.flags >> 5) & 0b1; }
  bool isTruncated() { return (header.flags >> 6) & 0b1; }
  bool isRecursionDesired() { return (header.flags >> 7) & 0b1; }
  bool isRecursionAvail() { return (header.flags >> 8) & 0b1; }
  unsigned int getReserved() { return (header.flags >> 9) & 0b111; }
  bool isReservedZero() { return getReserved() == 0; }
  unsigned int getResponseCode() { return  (header.flags >> 12) & 0b1111; }

  static unsigned char readByte(struct JaxPacket& p);
  static unsigned char peekByte(struct JaxPacket p);
  static void readData(struct JaxPacket& p, void *buffer, unsigned int len);
  static void writeData(struct JaxPacket& p, void *buffer, unsigned int len);
  static std::string readString(struct JaxPacket& p);
  static std::string readStringLiteral(struct JaxPacket& p);
  static std::string peekStringLiteral(struct JaxPacket p, unsigned int pos);
  static std::string readStringCompressed(struct JaxPacket& p);
  static void writeString(struct JaxPacket& p, std::string str);
};
