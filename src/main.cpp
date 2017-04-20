
#include "dnsjaxie.hpp"

static class dnsjaxie app;

void handleSignal(int signal) {
  app.running = 0;
}

int main(const int argc, char* const argv[]) {
  signal(SIGINT, handleSignal);
  app.setOptions(argc, argv);
  app.run();

  if (app.hasError()) {
    fprintf(stderr, "ERROR: %s\n", app.getError());
    return 1;
  }

  return 0;
}
