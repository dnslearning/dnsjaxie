
#pragma once

#include "Jax.hpp"
#include "JaxServer.hpp"

class App {
private:
  class JaxServer server;
public:
  App();
  ~App();

  void setOptions(const int argc, char* const argv[]);
  void run();
  void tick();
  void stop() { server.stop(); }
  bool isRunning() { return server.isRunning(); }
};
