
#include "JaxParser.hpp"

bool JaxParser::decode(JaxPacket& p) {
  questions.clear();
  answers.clear();

  readData(p, &header, sizeof(header));
  header.id = ntohs(header.id);

  if (header.id <= 0) {
    Jax::debug("DNS packet ID is zero");
    return false;
  }

  header.qdcount = ntohs(header.qdcount);
  header.ancount = ntohs(header.ancount);
  header.nscount = ntohs(header.nscount);
  header.arcount = ntohs(header.arcount);
  Jax::debug("qdcount = %d, ancount = %d", header.qdcount, header.ancount);

  if (header.qdcount <= 0 && header.ancount <= 0 && header.nscount <= 0 && header.arcount <= 0) {
    Jax::debug("Refusing to decode empty DNS packet");
    return false;
  }

  for (int i=0; i < header.qdcount; i++) { Jax::debug("decodeQuestion %d", i); decodeQuestion(p); }
  for (int i=0; i < header.ancount; i++) { Jax::debug("decodeAnswer %d", i); decodeAnswer(p); }

  return true;
}

void JaxParser::decodeQuestion(JaxPacket& p) {
  JaxDnsQuestion question;
  question.domain = readString(p);
  readData(p, &question.header, sizeof(question.header));
  question.header.qtype = ntohs(question.header.qtype);
  question.header.qclass = ntohs(question.header.qclass);
  // TODO do we care if they are asking for a specific type of record like MX?
  questions.push_back(question);
  Jax::debug("Question: %s", question.domain.c_str());
}

void JaxParser::decodeAnswer(JaxPacket& p) {
  JaxDnsAnswer answer;
  answer.domain = readString(p);
  Jax::debug("Answer: %s", answer.domain.c_str());

  readData(p, &answer.header, sizeof(answer.header));
  answer.header.atype = ntohs(answer.header.atype);
  answer.header.aclass = ntohs(answer.header.aclass);
  answer.header.ttl = ntohl(answer.header.ttl);
  answer.header.dataLen = ntohs(answer.header.dataLen);

  if (answer.header.dataLen > 512) {
    Jax::debug("dataLen = %d", answer.header.dataLen);
    throw std::runtime_error("Answer tried to include a very large data chunk");
  }

  Jax::debug("Answer data is %d bytes", answer.header.dataLen);

  if (answer.header.atype == 1) {
    answer.ipv6 = false;
    readData(p, &answer.addr4, sizeof(answer.addr4));
  } else if (answer.header.atype == 28) {
    answer.ipv6 = true;
    readData(p, &answer.addr6, sizeof(answer.addr6));
  } else {
    char buffer[512];
    readData(p, buffer, answer.header.dataLen);
  }

  answers.push_back(answer);
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
  if (questions.empty() && answers.empty()) {
    Jax::debug("Refusing to encode empty DNS packet");
    return false;
  }

  header.id = htons(header.id);
  header.qdcount = htons(questions.size());
  header.ancount = htons(answers.size());
  header.nscount = 0;
  header.arcount = 0;
  writeData(p, &header, sizeof(header));

  for (auto question : questions) { encodeQuestion(p, question); }
  for (auto answer : answers) { encodeAnswer(p, answer); }

  return true;
}

void JaxParser::encodeQuestion(JaxPacket& p, JaxDnsQuestion& question) {
  writeString(p, question.domain);
  question.header.qtype = htons(question.header.qtype);
  question.header.qclass = htons(question.header.qclass);
  writeData(p, &question.header, sizeof(question.header));
}

void JaxParser::encodeAnswer(JaxPacket& p, JaxDnsAnswer& answer) {
  writeString(p, answer.domain);
  answer.header.atype = htons(answer.header.atype);
  answer.header.aclass = htons(answer.header.aclass);
  answer.header.ttl = htonl(answer.header.ttl);
  answer.header.dataLen = htons(answer.ipv6 ? sizeof(answer.addr6) : sizeof(answer.addr4));
  writeData(p, &answer.header, sizeof(answer.header));

  if (answer.ipv6) {
    writeData(p, &answer.addr6, sizeof(answer.addr6));
  } else {
    writeData(p, &answer.addr4, sizeof(answer.addr4));
  }
}

void JaxParser::writeString(JaxPacket& p, std::string str) {
  auto parts = Jax::split(str, '.');
  for (auto str : parts) { writeStringLiteral(p, str); }
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
