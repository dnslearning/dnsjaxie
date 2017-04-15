
#include <signal.h>
#include "dnsjaxie.h"

int main(const int argc, const char *argv[]) {
  struct dnsjaxie_t jax;

  printf("Allocating dnsjaxie\n");
  dnsjaxie_init(&jax);

  printf("Setting up signal handler\n");
  void handleSignal(int signal) { jax.running = 0; }
  signal(SIGINT, handleSignal);

  printf("Listening\n");
  dnsjaxie_listen(&jax);

  printf("Starting main loop\n");
  while (jax.running) { dnsjaxie_tick(&jax); }

  printf("Cleaning up\n");
  dnsjaxie_close(&jax);

  if (jax.error) {
    printf("ERROR: %s\n", jax.error);
    return 1;
  }

  return 0;
}
