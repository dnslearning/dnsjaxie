
#include <signal.h>
#include "dnsjaxie.hpp"

static class dnsjaxie jax;

void handleSignal(int signal) {
  jax.running = 0;
}

int main(const int argc, const char *argv[]) {
  jax.debug("Setting up signal handler");
  signal(SIGINT, handleSignal);

  jax.debug("Listening");
  jax.listen();

  jax.debug("Starting main loop");
  while (jax.running) { jax.tick(); }

  jax.debug("Cleaning up");
  jax.close();

  if (jax.hasError()) {
    fprintf(stderr, "ERROR: %s\n", jax.getError());
    return 1;
  }

  return 0;
}
