
#include "JaxPacket.hpp"

JaxPacket::JaxPacket(std::vector<char> v) : raw(v) {
  pos = 0;
}

JaxPacket::JaxPacket(unsigned int size) : raw(size) {
  pos = 0;
}

JaxPacket::JaxPacket(char *buffer, unsigned int len) : raw(buffer, buffer + len) {
  pos = 0;
}

unsigned char JaxPacket::readByte() {
  return raw.at(pos++);
}

void JaxPacket::writeByte(unsigned char c) {
  raw.at(pos++) = c;
}

void JaxPacket::readData(void* buffer, unsigned int len) {
  if ((pos + len) > raw.size()) { throw std::runtime_error("Attempt to read beyond packet bounds"); }
  memcpy(buffer, raw.data() + pos, len);
  pos += len;
}

void JaxPacket::writeData(const void *buffer, unsigned int len) {
  if ((pos + len) > raw.size()) { throw std::runtime_error("Attempt to write beyond packet bounds"); }
  memcpy(raw.data() + pos, buffer, len);
  pos += len;
}

std::string JaxPacket::readString() {
  unsigned char c;
  std::string str;

  do {
    c = readByte();
    if (c && !str.empty()) { str += '.'; }

    if (c & 0b11000000) {
      str += readStringCompressed(c);
      c = 0;
    } else if (c) {
      str += readStringLiteral(c);
    }
  } while (c);

  return str;
}

std::string JaxPacket::readStringLiteral(unsigned char len) {
  if (len == 0) { return ""; }
  if (len > 63) { throw std::runtime_error("DNS literal strings cannot be longer than 63 bytes"); }

  char chunk[64];
  readData(chunk, len);
  chunk[len] = '\0';
  return chunk;
}

std::string JaxPacket::readStringCompressed(unsigned char c) {
  unsigned short offset = c;
  if ((offset & 0b11000000) != 0b11000000) { throw std::runtime_error("DNS labels should have 2 most significant bits set"); }
  offset = ntohs((offset & 0b00111111) | (readByte() << 8));
  if (offset > pos) { throw std::runtime_error("DNS labels must point backwards"); }
  return peekString(offset);
}

std::string JaxPacket::peekString(unsigned int offset) {
  unsigned int t = pos;
  pos = offset;
  std::string s = readString();
  pos = t;
  return s;
}

void JaxPacket::writeString(std::string str) {
  if (str.empty()) {
    writeByte(0);
    return;
  }

  auto parts = Jax::split(str, '.');
  for (auto str : parts) { writeStringLiteral(str); }
  writeByte('\0');
}

void JaxPacket::writeStringLiteral(std::string s) {
  if (s.size() > 63) { throw std::runtime_error("DNS strings cannot be longer than 63 chars"); }
  writeByte(s.size());
  writeData(s.c_str(), s.size());
}

std::vector<char> JaxPacket::encodeString(std::string s) {
  JaxPacket p(512);
  p.writeString(s);
  p.raw.resize(p.pos);
  return p.raw;
}
