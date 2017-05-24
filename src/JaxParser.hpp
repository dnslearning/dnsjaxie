
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
  void decodeQuestion(JaxPacket& p);
  void decodeAnswer(JaxPacket& p);
  void decodeAuthority(JaxPacket& p);
  void decodeResource(JaxPacket& p);

  struct JaxDnsResource readResource(JaxPacket& p, bool r);
  void writeResource(JaxPacket& p, JaxDnsResource res, bool r);

  bool encode(JaxPacket& p);
  void encodeQuestion(JaxPacket& p, JaxDnsResource& question);
  void encodeAnswer(JaxPacket& p, JaxDnsResource& answer);
  void encodeAuthority(JaxPacket& p, JaxDnsResource& authority);
  void encodeResource(JaxPacket& p, JaxDnsResource& resource);

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
  static std::string readStringLiteral(JaxPacket& p, unsigned char len);
  static std::string readStringCompressed(JaxPacket& p, unsigned char c);
  static void writeByte(JaxPacket& p, char c);
  static void writeData(JaxPacket& p, const void *buffer, unsigned int len);
  static void writeString(JaxPacket& p, std::string str);
  static void writeStringLiteral(JaxPacket &p, std::string str);
  static std::vector<char> encodeString(std::string s);

  static const unsigned short FLAG_RESPONSE = 1 << 15;
  static const unsigned short FLAG_AUTH_ANSWER = 1 << 10;
  static const unsigned short FLAG_TRUNCATED = 1 << 9;
  static const unsigned short FLAG_RECURSION_DESIRED = (1 << 8);
  static const unsigned short FLAG_RECURSION_AVAILABLE = (1 << 7);
};
