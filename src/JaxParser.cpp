
#include "JaxParser.hpp"

bool JaxParser::decode(struct JaxPacket& p) {
  domains.clear();
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

  if (
    header.qdcount <= 0 &&
    header.ancount <= 0 &&
    header.nscount <= 0 &&
    header.arcount <= 0
  ) {
    Jax::debug("DNS packet has nothing in it");
    return false;
  }

  for (int i=0; i < header.qdcount; i++) {
    if (!decodeQuestion(p)) { return false; }
  }

  return true;
}

bool JaxParser::decodeQuestion(struct JaxPacket& p) {
  std::string domain = parseString(p);
  struct JaxDnsQuestion question;
  readData(p, &question, sizeof(question));

  // TODO do we care if they are asking for a specific type of record like MX?

  if (question.qclass != 1 && question.qclass != 255) {
    Jax::debug("Unknown DNS question class %d", question.qclass);
    return false;
  }

  domains.push_back(domain);
  return true;
}

std::string JaxParser::parseString(struct JaxPacket& p) {
  std::string str = "";

  do {
    unsigned char len = peekByte(p);

    if (len == 0) {
      // Last part of the string
      break;
    }

    if (len & 0b11000000) {
      str += readLabel(p);
    } else {
      if (!str.empty()) { str += '.'; }
      str += readString(p);
    }
  } while (1);

  return str;
}

std::string JaxParser::readString(struct JaxPacket& p) {
  unsigned char len = readByte(p);
  if (len > 63) { throw std::runtime_error("DNS strings must be 63 chars or less"); }
  char chunk[64];
  readData(p, chunk, len);
  chunk[len] = '\0';
  return chunk;
}

void JaxParser::readData(struct JaxPacket& p, void* buffer, unsigned int len) {
  if ((p.pos + len) >= p.inputSize) { throw std::runtime_error("DNS string goes outside buffer"); }
  memcpy(buffer, p.input + p.pos, len);
  p.pos += len;
}

std::string JaxParser::peekString(struct JaxPacket p, unsigned int pos) {
  p.pos = pos;
  return readString(p);
}

std::string JaxParser::readLabel(struct JaxPacket& p) {
  unsigned short offset = readByte(p);
  if ((offset & 0b11000000) != 0b11000000) { throw std::runtime_error("DNS labels should have 2 most significant bits set"); }
  offset = ntohs((offset & 0b00111111) | (readByte(p) << 6));
  Jax::debug("offset = %d", offset);
  if (offset > p.pos) { throw std::runtime_error("DNS labels must point backwards"); }
  return peekString(p, offset);
}

unsigned char JaxParser::readByte(struct JaxPacket& p) {
  if (p.pos >= p.inputSize) { throw std::runtime_error("DNS packet data attempts to go outside buffer"); }
  return p.input[p.pos++];
}

unsigned char JaxParser::peekByte(struct JaxPacket p) {
  return readByte(p);
}

bool JaxParser::encode(struct JaxPacket& p) {
  //memset(buffer, 0, bufferSize);
  //memcpy(buffer, &header, sizeof(header));
  return false;
}
