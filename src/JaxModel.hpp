
#pragma once

#include "Jax.hpp"
#include "JaxDevice.hpp"

class JaxModel {
private:
  sql::Driver *sqlDriver;
  sql::Connection *sqlConnection;
  sql::PreparedStatement *sqlFetchIPv6;
  sql::PreparedStatement *sqlFetchIPv4;
  sql::PreparedStatement *sqlInsertActivity;
  sql::PreparedStatement *sqlInsertTimeline;
  sql::PreparedStatement *sqlFetchDomains;

  bool readDevice(JaxDevice& device, sql::PreparedStatement *statement);
  bool fetchIPv6(const std::string address, JaxDevice& device);
  bool fetchIPv4(const std::string local, const std::string remote, JaxDevice& device);
public:
  std::string name = "dnsjaxie";
  std::string user = "root";
  std::string pass = "";
  std::string host = "localhost";
  std::unordered_map<std::string, JaxDomain> domains;
  std::unordered_map<std::string, int> ipv4cache;
  std::unordered_map<std::string, int> ipv6cache;
  std::unordered_map<int, JaxDevice> deviceCache;

  void prepare();
  bool getDomain(const std::string host, JaxDomain& domain);
  bool fetch(JaxClient& client, JaxDevice& device);
  void insertActivity(int id, bool learnMode);
  void insertTimeline(int id, std::string domain);
};
