
#pragma once

#include "Jax.hpp"

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
  bool learnMode;
  int deviceId;

  void prepare();
  bool getDomain(std::string host, JaxDomain& domain);
  bool fetch(struct in6_addr& local, struct in6_addr& remote);
  bool isFakeIPv6(std::string s);
  std::string toStringIPv4(std::string s);
  sql::ResultSet *fetchIPv6(std::string local);
  sql::ResultSet *fetchIPv4(std::string local, std::string remote);
  void insertActivity(int id, bool learnMode);
  void insertTimeline(int id, std::string domain);
};
