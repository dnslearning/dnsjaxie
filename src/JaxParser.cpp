
#include "JaxParser.hpp"

bool JaxParser::isEmpty() {
  return header.qdcount <= 0 && header.ancount <= 0 && header.nscount <= 0 && header.arcount <= 0;
}

bool JaxParser::decode(struct JaxPacket& p) {
  questions.clear();
  answers.clear();

  readData(p, &header, sizeof(header));
  header.id = ntohs(header.id);

  if (header.id <= 0) {
    Jax::debug("DNS packet ID is zero");
    return false;
  }

  header.flags = ntohs(header.flags);
  // TODO check flags

  header.qdcount = ntohs(header.qdcount);
  header.ancount = ntohs(header.ancount);
  header.nscount = ntohs(header.nscount);
  header.ancount = ntohs(header.ancount);
  Jax::debug("qdcount = %d, ancount = %d", header.qdcount, header.ancount);

  if (isEmpty()) {
    Jax::debug("Refusing to decode empty DNS packet");
    return false;
  }

  for (int i=0; i < header.qdcount; i++) { decodeQuestion(p); }
  for (int i=0; i < header.ancount; i++) { decodeAnswer(p); }

  return true;
}

void JaxParser::decodeQuestion(struct JaxPacket& p) {
  struct JaxDnsQuestion question;
  question.domain = readString(p);
  readData(p, &question.header, sizeof(question.header));
  // TODO do we care if they are asking for a specific type of record like MX?
  questions.push_back(question);
}

void JaxParser::decodeAnswer(struct JaxPacket& p) {
  struct JaxDnsAnswer answer;
  answer.domain = readString(p);
  readData(p, &answer.header, sizeof(answer.header));
  answers.push_back(answer);
}

std::string JaxParser::readString(struct JaxPacket& p) {
  std::string str = "";

  do {
    unsigned char len = peekByte(p);

    if (len == 0) {
      // Last part of the string
      break;
    }

    if (len & 0b11000000) {
      str += readStringCompressed(p);
    } else {
      if (!str.empty()) { str += '.'; }
      str += readStringLiteral(p);
    }
  } while (1);

  return str;
}

void JaxParser::readData(struct JaxPacket& p, void* buffer, unsigned int len) {
  if ((p.pos + len) >= p.inputSize) { throw std::runtime_error("Attempt to read outside DNS packet"); }
  memcpy(buffer, p.input + p.pos, len);
  p.pos += len;
}

std::string JaxParser::readStringLiteral(struct JaxPacket& p) {
  unsigned char len = readByte(p);
  if (len > 63) { throw std::runtime_error("DNS strings must be 63 chars or less"); }
  char chunk[64];
  readData(p, chunk, len);
  chunk[len] = '\0';
  return chunk;
}

std::string JaxParser::peekStringLiteral(struct JaxPacket p, unsigned int pos) {
  p.pos = pos;
  return readStringLiteral(p);
}

std::string JaxParser::readStringCompressed(struct JaxPacket& p) {
  unsigned short offset = readByte(p);
  if ((offset & 0b11000000) != 0b11000000) { throw std::runtime_error("DNS labels should have 2 most significant bits set"); }
  offset = ntohs((offset & 0b00111111) | (readByte(p) << 6));
  Jax::debug("offset = %d", offset);
  if (offset > p.pos) { throw std::runtime_error("DNS labels must point backwards"); }
  return peekStringLiteral(p, offset);
}

unsigned char JaxParser::readByte(struct JaxPacket& p) {
  if (p.pos >= p.inputSize) { throw std::runtime_error("DNS packet data attempts to go outside buffer"); }
  return p.input[p.pos++];
}

unsigned char JaxParser::peekByte(struct JaxPacket p) {
  return readByte(p);
}

bool JaxParser::encode(struct JaxPacket& p) {
  if (isEmpty()) {
    Jax::debug("Refusing to encode empty DNS packet");
    return false;
  }

  header.qdcount = htons(questions.size());
  header.ancount = htons(answers.size());
  header.nscount = 0;
  header.arcount = 0;
  writeData(p, &header, sizeof(header));

  for (auto question : questions) { encodeQuestion(p, question); }
  for (auto answer : answers) { encodeAnswer(p, answer); }

  return true;
}

void JaxParser::encodeQuestion(struct JaxPacket& p, struct JaxDnsQuestion& question) {

}

void JaxParser::encodeAnswer(struct JaxPacket& p, struct JaxDnsAnswer& answer) {

}

void JaxParser::writeString(struct JaxPacket& p, std::string str) {

}

void JaxParser::writeData(struct JaxPacket& p, void *buffer, unsigned int len) {
  if ((p.pos + len) >= p.inputSize) { throw std::runtime_error("Attempt to write outside DNS packet"); }
  memcpy(p.input + p.pos, buffer, len);
  p.pos += len;
}
