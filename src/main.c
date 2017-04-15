
#include <signal.h>
#include "dnsjaxie.h"

int main(const int argc, const char *argv[]) {
  struct dnsjaxie_t jax;

  dnsjaxie_debug("Allocating dnsjaxie");
  dnsjaxie_init(&jax);

  dnsjaxie_debug("Setting up signal handler");
  void handleSignal(int signal) { jax.running = 0; }
  signal(SIGINT, handleSignal);

  dnsjaxie_debug("Listening");
  dnsjaxie_listen(&jax);

  dnsjaxie_debug("Starting main loop");
  while (jax.running) { dnsjaxie_tick(&jax); }

  dnsjaxie_debug("Cleaning up");
  dnsjaxie_close(&jax);

  if (jax.error) {
    printf("ERROR: %s\n", jax.error);
    return 1;
  }

  return 0;
}
