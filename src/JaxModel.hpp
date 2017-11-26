
#pragma once

#include "Jax.hpp"

// TODO sucks this goes here instead of somewhere it belongs
struct device_t {
  int id;
  bool study;
  int time;
};

class JaxModel {
private:
  sql::Driver *sqlDriver;
  sql::Connection *sqlConnection;
  sql::PreparedStatement *sqlFetchIPv6;
  sql::PreparedStatement *sqlFetchIPv4;
  sql::PreparedStatement *sqlInsertActivity;
  sql::PreparedStatement *sqlInsertTimeline;
  sql::PreparedStatement *sqlFetchDomains;
public:
  std::string name = "dnsjaxie";
  std::string user = "root";
  std::string pass = "";
  std::string host = "localhost";
  std::unordered_map<std::string, JaxDomain> domains;
  std::unordered_map<std::string, struct device_t> ipv4cache;
  std::unordered_map<std::string, struct device_t> ipv6cache;
  std::unordered_map<int, int> lastActivity;
  bool learnMode;
  int deviceId;

  void prepare();
  bool getDomain(const std::string host, JaxDomain& domain);
  bool fetch(JaxClient& client);
  bool fetchIPv6(const std::string local);
  bool fetchIPv4(const std::string local, const std::string remote);
  void insertActivity(int id, bool learnMode);
  void insertTimeline(int id, std::string domain);
};
