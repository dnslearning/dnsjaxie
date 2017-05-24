
#include "JaxParser.hpp"

bool JaxParser::decode(JaxPacket& p) {
  questions.clear();
  answers.clear();
  auths.clear();
  additional.clear();

  readData(p, &header, sizeof(header));
  header.id = ntohs(header.id);
  if (!header.id) { Jax::debug("DNS packet ID is zero"); return false; }
  header.qdcount = ntohs(header.qdcount);
  header.ancount = ntohs(header.ancount);
  header.nscount = ntohs(header.nscount);
  header.arcount = ntohs(header.arcount);
  Jax::debug("counts %d %d %d %d", header.qdcount, header.ancount, header.nscount, header.arcount);

  if ((header.qdcount + header.ancount + header.nscount + header.arcount) <= 0) {
    Jax::debug("Refusing to decode empty DNS packet");
    return false;
  }

  for (int i=0; i < header.qdcount; i++) { Jax::debug("decodeQuestion %d", i); decodeQuestion(p); }
  for (int i=0; i < header.ancount; i++) { Jax::debug("decodeAnswer %d", i); decodeAnswer(p); }
  for (int i=0; i < header.nscount; i++) { Jax::debug("decodeAuthority %d", i); decodeAuthority(p); }
  for (int i=0; i < header.arcount; i++) { Jax::debug("decodeResource %d", i); decodeResource(p); }

  return true;
}

void JaxParser::decodeQuestion(JaxPacket& p) {
  JaxDnsResource res = readResource(p, false);
  Jax::debug("Question: %s", res.domain.c_str());
  questions.push_back(res);
}

void JaxParser::decodeAnswer(JaxPacket& p) {
  JaxDnsResource res = readResource(p, true);
  Jax::debug("Answer: %s", res.domain.c_str());
  answers.push_back(res);
}

void JaxParser::decodeAuthority(JaxPacket& p) {
  JaxDnsResource res = readResource(p, true);
  Jax::debug("Authority: %s", res.domain.c_str());
  auths.push_back(res);
}

void JaxParser::decodeResource(JaxPacket& p) {
  JaxDnsResource res = readResource(p, true);
  Jax::debug("Additional: %s", res.domain.c_str());
  additional.push_back(res);
}

struct JaxDnsResource JaxParser::readResource(JaxPacket& p, bool r) {
  JaxDnsResource res;

  res.domain = readString(p);
  readData(p, &res.header, r ? 10 : 4);
  res.header.rtype = ntohs(res.header.rtype);
  res.header.rclass = ntohs(res.header.rclass);

  if (r) {
    res.header.ttl = ntohl(res.header.ttl);
    res.header.dataLen = ntohs(res.header.dataLen);
    if (res.header.dataLen > 512) { throw std::runtime_error("DNS resource too big"); }

    switch (res.header.rtype) {
    case 2:
    case 5:
      // label may reference data outside the buffer so we convert it
      res.raw = encodeString(readString(p));
      res.header.dataLen = res.raw.size();
      break;
    default:
      // Directly copy resource data
      res.raw.resize(res.header.dataLen);
      readData(p, res.raw.data(), res.header.dataLen);
      break;
    }
  } else {
    res.header.ttl = 0;
    res.header.dataLen = 0;
  }

  return res;
}

std::vector<char> JaxParser::encodeString(std::string s) {
  char buffer[512];
  JaxPacket p;
  p.input = buffer;
  p.inputSize = sizeof(buffer);
  p.pos = 0;
  writeString(p, s);
  return std::vector<char>(p.input, p.input + p.pos);
}

void JaxParser::writeResource(JaxPacket& p, JaxDnsResource res, bool r) {
  unsigned short len = res.header.dataLen;
  writeString(p, res.domain);
  res.header.rtype = htons(res.header.rtype);
  res.header.rclass = htons(res.header.rclass);

  if (r) {
    res.header.ttl = htonl(res.header.ttl);
    res.header.dataLen = htons(res.header.dataLen);
  }

  writeData(p, &res.header, r ? 10 : 4);
  if (r) { writeData(p, res.raw.data(), len); }
}

std::string JaxParser::readString(JaxPacket& p) {
  unsigned char c;
  std::string str;

  do {
    c = readByte(p);
    if (c && !str.empty()) { str += '.'; }

    if (c & 0b11000000) {
      str += readStringCompressed(p, c);
      c = 0;
    } else if (c) {
      str += readStringLiteral(p, c);
    }
  } while (c);

  return str;
}

std::string JaxParser::peekString(JaxPacket p, unsigned int pos) {
  p.pos = pos;
  return readString(p);
}

void JaxParser::readData(JaxPacket& p, void* buffer, unsigned int len) {
  if ((p.pos + len) > p.inputSize) { throw std::runtime_error("Attempt to read outside DNS packet"); }
  memcpy(buffer, p.input + p.pos, len);
  p.pos += len;
}

std::string JaxParser::readStringLiteral(JaxPacket& p, unsigned char len) {
  if (len == 0) { return ""; }
  if (len > 63) { throw std::runtime_error("DNS literal strings cannot be longer than 63 bytes"); }

  char chunk[64];
  readData(p, chunk, len);
  chunk[len] = '\0';
  return chunk;
}

std::string JaxParser::readStringCompressed(JaxPacket& p, unsigned char c) {
  unsigned short offset = c;
  if ((offset & 0b11000000) != 0b11000000) { throw std::runtime_error("DNS labels should have 2 most significant bits set"); }
  offset = ntohs((offset & 0b00111111) | (readByte(p) << 8));
  Jax::debug("offset = %d", offset);
  if (offset > p.pos) { throw std::runtime_error("DNS labels must point backwards"); }
  return peekString(p, offset);
}

unsigned char JaxParser::readByte(JaxPacket& p) {
  if (p.pos >= p.inputSize) { throw std::runtime_error("DNS packet data attempts to go outside buffer"); }
  return p.input[p.pos++];
}

unsigned char JaxParser::peekByte(JaxPacket p) {
  return readByte(p);
}

bool JaxParser::encode(JaxPacket& p) {
  //if (questions.empty() && answers.empty()) {
  //  Jax::debug("Refusing to encode empty DNS packet");
  //  return false;
  //}

  //auths.clear();

  header.id = htons(header.id);
  header.qdcount = htons(questions.size());
  header.ancount = htons(answers.size());
  header.nscount = htons(auths.size());
  header.arcount = htons(additional.size());
  writeData(p, &header, sizeof(header));

  for (auto question : questions) { encodeQuestion(p, question); }
  for (auto answer : answers) { encodeAnswer(p, answer); }
  for (auto authority : auths) { encodeAuthority(p, authority); }
  for (auto resource : additional) { encodeResource(p, resource); }

  return true;
}

void JaxParser::encodeQuestion(JaxPacket& p, JaxDnsResource& res) {
  writeResource(p, res, false);
}

void JaxParser::encodeAnswer(JaxPacket& p, JaxDnsResource& res) {
  writeResource(p, res, true);
}

void JaxParser::encodeAuthority(JaxPacket& p, JaxDnsResource& res) {
  writeResource(p, res, true);
}

void JaxParser::encodeResource(JaxPacket& p, JaxDnsResource& res) {
  writeResource(p, res, true);
}

void JaxParser::writeString(JaxPacket& p, std::string str) {
  if (str != "") {
    auto parts = Jax::split(str, '.');
    for (auto str : parts) { writeStringLiteral(p, str); }
  }

  writeByte(p, '\0');
}

void JaxParser::writeByte(JaxPacket& p, char c) {
  if (p.pos >= p.inputSize) { throw std::runtime_error("Attempt to write a byte outside bounds"); }
  p.input[p.pos++] = c;
}

void JaxParser::writeData(JaxPacket& p, const void *buffer, unsigned int len) {
  if ((p.pos + len) >= p.inputSize) { throw std::runtime_error("Attempt to write outside DNS packet"); }
  memcpy(p.input + p.pos, buffer, len);
  p.pos += len;
}

void JaxParser::writeStringLiteral(JaxPacket& p, std::string str) {
  if (str.size() > 63) { throw std::runtime_error("DNS strings cannot be longer than 63 chars"); }
  writeByte(p, str.size());
  writeData(p, str.c_str(), str.size());
}
