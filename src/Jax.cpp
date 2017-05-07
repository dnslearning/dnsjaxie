
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

// http://stackoverflow.com/a/236803/226526
template<typename Out>
void Jax::split(const std::string& s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

std::vector<std::string> Jax::split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
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
