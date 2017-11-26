
#include "Jax.hpp"

void Jax::debug(const char *format, ...) {
#if JAX_DEBUG_ENABLED
  va_list argptr;
  va_start(argptr, format);
  vfprintf(stderr, format, argptr);
  va_end(argptr);
  fprintf(stderr, "\n");
#endif
}

std::runtime_error Jax::socketError(std::string reason) {
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "Socket error %s: %s", reason.c_str(), strerror(errno));
  return std::runtime_error(std::string(buffer));
}

std::vector<std::string> Jax::split(const std::string& s, char delim) {
  std::vector<std::string> elems;
  std::stringstream ss(s);
  std::string item;

  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }

  return elems;
}

std::string Jax::toString(struct in6_addr& addr) {
  char buffer[INET6_ADDRSTRLEN];
  jax_zero(buffer);
  inet_ntop(AF_INET6, &addr, buffer, sizeof(buffer));
  return std::string(buffer);
}

std::string Jax::toString(std::vector<std::string>& parts, const char *delim) {
  std::ostringstream joined;
  std::copy(parts.begin(), parts.end(), std::ostream_iterator<std::string>(joined, delim));
  std::string s = joined.str();
  s.pop_back();
  return s;
}

std::string Jax::toString(std::vector<char>& v) {
  return std::string(v.begin(), v.end());
}

bool Jax::isFakeIPv6(std::string s) {
  return s.find("::ffff:") == 0;
}

std::string Jax::convertFakeIPv6(std::string s) {
  if (!Jax::isFakeIPv6(s)) { return s; }
  return s.substr(7);
}

std::vector<char> Jax::toVector(std::string s) {
  return std::vector<char>(s.begin(), s.end());
}
