
#include "JaxModel.hpp"

#define prepareSql(symbol, sql) symbol = symbol ? symbol : sqlConnection->prepareStatement(sql);

void JaxModel::prepare() {
  if (!sqlDriver) {
    sqlDriver = get_driver_instance();
    if (!sqlDriver) { throw std::runtime_error("Unable to load SQL driver"); }
  }

  if (!sqlConnection) {
    sqlConnection = sqlDriver->connect(host, user, pass);
    if (!sqlConnection) { throw std::runtime_error("No database connection"); }
    const bool yes = 1;
    sqlConnection->setClientOption("OPT_RECONNECT", &yes);
    sqlConnection->setSchema(name);
  }

  prepareSql(sqlFetchIPv6,
    "select `id`, `learnMode` "
    "from `device` where (`listen_ipv6` = ?) "
    "limit 1"
  );

  prepareSql(sqlFetchIPv4,
    "select `device`.`id`, `device`.`learnMode` "
    "from `device_ipv4` "
    "join `device` on (`device_ipv4`.`device_id` = `device`.`id`) "
    "where (`device_ipv4`.`local` = ?) and (`device_ipv4`.`remote` = ?)"
  );

  prepareSql(sqlInsertActivity,
    "insert into `device_activity` set "
    " `device_id` = ?, "
    " `time` = floor(unix_timestamp() / 60) * 60, "
    " `dns` = 1, "
    " `learnMode` = ? "
    "on duplicate key update `dns` = `dns` + 1"
  );

  prepareSql(sqlInsertTimeline,
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

  prepareSql(sqlFetchDomains,
    "select * from `domain`"
  );

  if (!domains.empty()) {
    return;
  }

  sqlFetchDomains->execute();
  sql::ResultSet *resultSet = sqlFetchDomains->getResultSet();

  while (resultSet->next()) {
    JaxDomain domain;
    domain.host = std::string(resultSet->getString("host"));
    domain.allow = resultSet->getBoolean("allow");
    domain.deny = resultSet->getBoolean("deny");
    domain.ignore = resultSet->getBoolean("ignore");
    domains[domain.host] = domain;

    Jax::debug("Domain: %s (allow=%d, deny=%d, ignore=%d)", domain.host.c_str(), domain.allow, domain.deny, domain.ignore);
  }

  delete resultSet;
}

bool JaxModel::getDomain(std::string host, JaxDomain& domain) {
  if (domains.find(host) == domains.end()) { return false; }
  domain = domains[host];
  return true;
}

bool JaxModel::isFakeIPv6(std::string s) {
  return s.find("::ffff:") == 0;
}

std::string JaxModel::toStringIPv4(std::string s) {
  if (!isFakeIPv6(s)) { return ""; }
  return s.substr(7);
}

bool JaxModel::fetch(struct in6_addr& localAddr, struct in6_addr& remoteAddr) {
  prepare();
  std::string local = Jax::toString(localAddr);
  std::string remote = Jax::toString(remoteAddr);
  sql::ResultSet *resultSet = NULL;
  Jax::debug("Looking up IP %s from %s", local.c_str(), remote.c_str());

  if (isFakeIPv6(local)) {
    resultSet = fetchIPv4(toStringIPv4(local), toStringIPv4(remote));
  } else {
    resultSet = fetchIPv6(local);
  }

  if (!resultSet) {
    return false;
  } else if (!resultSet->next()) {
    delete resultSet;
    return false;
  }

  deviceId = resultSet->getInt(1);
  learnMode = resultSet->getInt(2);
  delete resultSet;
  return true;
}

sql::ResultSet *JaxModel::fetchIPv6(std::string local) {
  sqlFetchIPv6->setString(1, local);
  if (!sqlFetchIPv6->execute()) { return NULL; }
  return sqlFetchIPv6->getResultSet();
}

sql::ResultSet *JaxModel::fetchIPv4(std::string local, std::string remote) {
  sqlFetchIPv4->setString(1, local);
  sqlFetchIPv4->setString(2, remote);
  if (!sqlFetchIPv4->execute()) { return NULL; }
  return sqlFetchIPv4->getResultSet();
}

void JaxModel::insertActivity(int id, bool learnMode) {
  prepare();
  sqlInsertActivity->setInt(1, id);
  sqlInsertActivity->setInt(2, learnMode ? 1 : 0);
  sqlInsertActivity->execute();
}

void JaxModel::insertTimeline(int id, std::string domain) {
  std::vector<std::string> parts = Jax::split(domain, '.');
  if (parts.size() < 2) { return; }
  parts = std::vector<std::string>(parts.end() - 2, parts.end());
  domain = Jax::toString(parts, ".");
  prepare();
  sqlInsertTimeline->setInt(1, id);
  sqlInsertTimeline->setString(2, domain);
  sqlInsertTimeline->execute();
}
