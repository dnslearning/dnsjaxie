
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
