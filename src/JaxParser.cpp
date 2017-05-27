
#include "JaxParser.hpp"
#include "JaxPacket.hpp"

bool JaxParser::decode(JaxPacket& p) {
  questions.clear();
  answers.clear();
  auths.clear();
  additional.clear();

  p.readData(&header, sizeof(header));
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

  for (int i=0; i < header.qdcount; i++) { questions.push_back(readQuestion(p)); }
  for (int i=0; i < header.ancount; i++) { answers.push_back(readResource(p)); }
  for (int i=0; i < header.nscount; i++) { auths.push_back(readResource(p)); }
  for (int i=0; i < header.arcount; i++) { additional.push_back(readResource(p)); }

  return true;
}

bool JaxParser::encode(JaxPacket& p) {
  if (questions.empty() && answers.empty() && auths.empty() && additional.empty()) {
    Jax::debug("Refusing to encode empty DNS packet");
    return false;
  }

  // TODO temporary hack to make netflix work when packet is too big
  if (answers.size() > 5) {
    answers.resize(5);
    auths.clear();
  }

  header.id = htons(header.id);
  header.qdcount = htons(questions.size());
  header.ancount = htons(answers.size());
  header.nscount = htons(auths.size());
  header.arcount = htons(additional.size());
  p.writeData(&header, sizeof(header));

  for (auto question : questions) { writeQuestion(p, question); }
  for (auto answer : answers) { writeResource(p, answer); }
  for (auto authority : auths) { writeResource(p, authority); }
  for (auto resource : additional) { writeResource(p, resource); }

  p.raw.resize(p.pos);
  return true;
}

JaxDnsResource JaxParser::readQuestion(JaxPacket& p) {
  JaxDnsResource res;
  res.domain = p.readString();
  p.readData(&res.header, 4);
  res.header.rtype = ntohs(res.header.rtype);
  res.header.rclass = ntohs(res.header.rclass);
  res.header.ttl = 0;
  res.header.dataLen = 0;
  return res;
}

JaxDnsResource JaxParser::readResource(JaxPacket& p) {
  JaxDnsResource res;
  res.domain = p.readString();
  p.readData(&res.header, sizeof(res.header));
  res.header.rtype = ntohs(res.header.rtype);
  res.header.rclass = ntohs(res.header.rclass);
  res.header.ttl = ntohl(res.header.ttl);
  res.header.dataLen = ntohs(res.header.dataLen);
  if (res.header.dataLen > 512) { throw std::runtime_error("DNS resource is too large"); }

  switch (res.header.rtype) {
  case 2:
  case 5:
    res.raw = JaxPacket::encodeString(p.readString());
    res.header.dataLen = res.raw.size();
    break;
  default:
    res.raw.resize(res.header.dataLen);
    p.readData(res.raw.data(), res.header.dataLen);
    break;
  }

  return res;
}

void JaxParser::writeQuestion(JaxPacket& p, JaxDnsResource& res) {
  p.writeString(res.domain);
  res.header.rtype = htons(res.header.rtype);
  res.header.rclass = htons(res.header.rclass);
  p.writeData(&res.header, 4);
}

void JaxParser::writeResource(JaxPacket& p, JaxDnsResource& res) {
  p.writeString(res.domain);
  res.header.rtype = htons(res.header.rtype);
  res.header.rclass = htons(res.header.rclass);
  res.header.ttl = htonl(res.header.ttl);
  res.header.dataLen = htons(res.raw.size());
  p.writeData(&res.header, sizeof(res.header));
  p.writeData(res.raw.data(), res.raw.size());
}
