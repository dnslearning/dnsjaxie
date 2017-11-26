
#pragma once

#include "Jax.hpp"
#include "JaxServer.hpp"

class App {
private:
  JaxServer server;
  std::string configPath = "/etc/dnsjaxie.conf";
public:
  void setOptions(const int argc, char* const argv[]);
  void configure();
  void configure(const std::string key, const std::string value);
  void run();
  void tick();
  void stop() { server.stop(); }
  bool isRunning() { return server.isRunning(); }
};
