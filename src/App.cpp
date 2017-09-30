
#include "App.hpp"

void App::run() {
  configure();
  server.listen();

  while (isRunning()) {
    tick();
  }
}

void App::tick() {
  server.tick();
}

void App::setOptions(int argc, char* const argv[]) {
  int c;

  while ((c = getopt (argc, argv, "f:")) != -1)
    switch (c) {
    case 'f':
      configPath = optarg;
      break;
    case '?':
      if (optopt == 'f') {
        fprintf(stderr, "Missing argument (file) with option -f\n");
      } else {
        fprintf(stderr, "Unknown option -%c\n", optopt);
      }

      break;
    default:
      throw std::runtime_error("Unknown option during parsing");
  }
}

void App::configure() {
  std::ifstream conf(configPath);
  std::string key = "", value = "";
  // TODO better config file loading

  while (conf >> key >> value) {
    configure(key, value);
  };
}

void App::configure(std::string key, std::string value) {
  if (key == "dbuser") {
    server.model.user = value;
  } else if (key == "dbpass") {
    server.model.pass = value;
  } else if (key == "dbname") {
    server.model.name = value;
  } else if (key == "dbhost") {
    server.model.host = value;
  } else if (key == "dnshost") {
    server.realDnsAddr.sin_addr.s_addr = inet_addr(value.c_str());
  } else if (key == "dnsport") {
    server.realDnsAddr.sin_port = htons((unsigned short)std::stoi(value));
  } else if (key == "dnshide") {
    in_addr_t addr = inet_addr(value.c_str());
    server.hide.push_back(addr);
  } else {
    Jax::debug("Unknown config key '%s'", key.c_str());
  }
}
