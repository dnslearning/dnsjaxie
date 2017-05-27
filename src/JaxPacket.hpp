
#pragma once

#include "Jax.hpp"

class JaxPacket {
public:
  std::vector<char> raw;
  std::unordered_map<std::string, unsigned short> lookup;
  unsigned int pos;

  JaxPacket(unsigned int size);
  JaxPacket(std::vector<char> v);
  JaxPacket(char *buffer, unsigned int size);

  unsigned char readByte();
  void readData(void *buffer, unsigned int len);
  std::string readString();
  std::string readStringLiteral(unsigned char c);
  std::string readStringCompressed(unsigned char c);
  std::string peekString(unsigned int offset);

  void writeByte(unsigned char c);
  void writeData(const void *buffer, unsigned int len);
  void writeString(std::string s);
  void writeStringLiteral(std::string s);
  std::vector<char> compress(std::vector<char> v, int relative);

  static std::vector<char> encodeString(std::string s);
  static std::string decodeString(std::vector<char> v);
  static std::vector<char> encodeShort(unsigned short x);
};
