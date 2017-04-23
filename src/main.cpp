
#include "App.hpp"

static class App app;

void handleSignal(int signal) {
  app.stop();
}

int main(const int argc, char* const argv[]) {
  signal(SIGINT, handleSignal);
  app.setOptions(argc, argv);

  try {
    app.run();
  } catch (std::exception &e) {
    fprintf(stderr, "ERROR: %s\n", e.what());
    return 1;
  }

  return 0;
}
