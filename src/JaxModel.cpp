
#include "JaxModel.hpp"

void JaxModel::prepare() {
  if (!sqlDriver) {
    sqlDriver = get_driver_instance();
    if (!sqlDriver) { throw std::runtime_error("Unable to load SQL driver"); }
  }

  if (!sqlConnection) {
    sqlConnection = sqlDriver->connect(host, user, pass);
    if (!sqlConnection) { throw std::runtime_error("No database connection"); }
    sqlConnection->setSchema(name);
  }

  sqlFetch = sqlFetch ? sqlFetch : sqlConnection->prepareStatement(
    "select `id`, `learnMode` "
    "from `device` where (`listen_ipv6` = ?) "
    "limit 1"
  );

  sqlInsertActivity = sqlInsertActivity ? sqlInsertActivity : sqlConnection->prepareStatement(
    "insert into `device_activity` set "
    " `device_id` = ?, "
    " `time` = floor(unix_timestamp() / 60) * 60, "
    " `dns` = 1, "
    " `learnMode` = ? "
    "on duplicate key update `dns` = `dns` + 1"
  );

  sqlInsertTimeline = sqlInsertTimeline ? sqlInsertTimeline : sqlConnection->prepareStatement(
    "insert into `timeline` set "
    " `device_id` = ?, "
    " `type` = 'domain', "
    " `firstSeen` = unix_timestamp(), "
    " `lastSeen` = unix_timestamp(), "
    " `hits` = 1, "
    " `data` = ? "
    "on duplicate key update "
    " `hits` = `hits` + 1, "
    " `lastSeen` = unix_timestamp()"
  );
}

bool JaxModel::fetch(struct in6_addr& addr) {
  prepare();
  char addrStr[INET6_ADDRSTRLEN];
  jax_zero(addrStr);
  inet_ntop(AF_INET6, &addr, addrStr, sizeof(addrStr));
  Jax::debug("Looking up IP %s", addrStr);
  sqlFetch->setString(1, addrStr);
  if (!sqlFetch->execute()) { return false; }
  sql::ResultSet *resultSet = sqlFetch->getResultSet();
  if (!resultSet) { return false; }
  if (!resultSet->next()) { delete resultSet; return false; }
  deviceId = resultSet->getInt(1);
  learnMode = resultSet->getInt(2);
  delete resultSet;
  return true;
}

void JaxModel::insertActivity(int id, bool learnMode) {
  prepare();
  sqlInsertActivity->setInt(1, id);
  sqlInsertActivity->setInt(2, learnMode ? 1 : 0);
  sqlInsertActivity->execute();
}

void JaxModel::insertTimeline(int id, std::string domain) {
  prepare();
  sqlInsertTimeline->setInt(1, id);
  sqlInsertTimeline->setString(2, domain);
  sqlInsertTimeline->execute();
}
