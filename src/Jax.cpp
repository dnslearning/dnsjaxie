
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
  return std::runtime_error(reason);
}

// http://stackoverflow.com/a/236803/226526
template<typename Out>
void Jax::split(const std::string &s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

std::vector<std::string> Jax::split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}
