
#pragma once

#include "Jax.hpp"

class JaxModel {
private:
  sql::Driver *sqlDriver;
  sql::Connection *sqlConnection;
  sql::PreparedStatement *sqlFetch;
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
  bool fetch(struct in6_addr& addr);
  void insertActivity(int id, bool learnMode);
  void insertTimeline(int id, std::string domain);
};
