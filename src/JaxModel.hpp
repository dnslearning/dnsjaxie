
#pragma once

#include "Jax.hpp"

class JaxModel {
private:
  sql::Driver *sqlDriver;
  sql::Connection *sqlConnection;
  sql::PreparedStatement *sqlFetch;
  sql::PreparedStatement *sqlInsertActivity;
  sql::PreparedStatement *sqlInsertTimeline;
public:
  std::string name = "dnsjaxie";
  std::string user = "root";
  std::string pass = "";
  std::string host = "localhost";
  bool learnMode;
  int deviceId;

  void prepare();
  bool fetch(struct in6_addr& addr);
  void insertActivity(int id, bool learnMode);
  void insertTimeline(int id, std::string domain);
};
