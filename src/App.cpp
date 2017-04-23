
#include "App.hpp"

App::App() {

}

App::~App() {

}

void App::run() {
  server.listen();

  while (isRunning()) {
    tick();
  }
}

void App::tick() {
  server.tick();
}

void App::setOptions(int argc, char* const argv[]) {

}
