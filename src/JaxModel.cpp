
#include "JaxModel.hpp"
#include "JaxClient.hpp"

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

  //if (!sqlConnection->isValid()) {
  //  sqlConnection->reconnect();
  //}

  prepareSql(sqlFetchIPv6,
    "select `id`, `study` "
    "from `device` where (`ipv6` = ?) "
    "limit 1"
  );

  prepareSql(sqlFetchIPv4,
    "select `device`.`id`, `device`.`study` "
    "from `ipv4` "
    "join `device` on (`ipv4`.`device_id` = `device`.`id`) "
    "where (`ipv4`.`local` = ?) and (`ipv4`.`remote` = ?)"
  );

  prepareSql(sqlInsertActivity,
    "insert into `activity` set "
    " `device_id` = ?, "
    " `time` = unix_timestamp(), "
    " `seconds` = 15, "
    " `study` = ? "
  );

  prepareSql(sqlInsertTimeline,
    "insert into `timeline` set "
    " `device_id` = ?, "
    " `time` = unix_timestamp(), "
    " `type` = 'domain', "
    " `data` = ? "
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
    domain.redirect = std::string(resultSet->getString("redirect"));
    domains[domain.host] = domain;
  }

  delete resultSet;
  Jax::debug("Loaded %d domains", domains.size());
}

bool JaxModel::getDomain(const std::string host, JaxDomain& domain) {
  std::deque<std::string> parts;
  std::stringstream ss(host);
  std::string item;

  while (std::getline(ss, item, '.')) {
    parts.push_back(item);
  }

  while (parts.size() >= 2) {
    std::string subdomain = Jax::join(parts, ".");

    if (domains.find(subdomain) != domains.end()) {
      domain = domains[subdomain];
      return true;
    }

    parts.pop_front();
  }

  return false;
}

bool JaxModel::fetch(JaxClient& client) {
  prepare();
  Jax::debug("Looking up IP %s from %s", client.local.c_str(), client.remote.c_str());

  if (client.ipv4) {
    return fetchIPv4(client.local, client.remote);
  } else {
    return fetchIPv6(client.local);
  }
}

// TODO local is wrong word here
bool JaxModel::fetchIPv6(const std::string local) {
  int now = (int)time(NULL);
  auto entry = ipv6cache.find(local);

  if (entry != ipv6cache.end()) {
    struct device_t device = entry->second;
    int age = now - device.time;

    if (age < 15) {
      deviceId = device.id;
      learnMode = device.study;
      return true;
    }
  }

  sqlFetchIPv6->setString(1, local);

  if (!sqlFetchIPv6->execute()) {
    return false;
  }

  sql::ResultSet *results = sqlFetchIPv6->getResultSet();

  if (!results) {
    return false;
  } else if (!results->next()) {
    delete results;
    return false;
  }

  deviceId = results->getInt(1);
  learnMode = results->getInt(2);
  delete results;

  // Save into cache
  device_t cache = {};
  cache.id = deviceId;
  cache.study = learnMode;
  cache.time = (int)time(NULL);
  ipv6cache[local] = cache;

  return true;
}

bool JaxModel::fetchIPv4(const std::string local, const std::string remote) {
  // TODO poor mans way of making 2 strings into one key for now
  std::string key = local + "\n" + remote;

  int now = (int)time(NULL);
  auto entry = ipv4cache.find(key);

  if (entry != ipv4cache.end()) {
    struct device_t device = entry->second;
    int age = now - device.time;

    if (age < 15) {
      deviceId = device.id;
      learnMode = device.study;
      return true;
    }
  }

  sqlFetchIPv4->setString(1, local);
  sqlFetchIPv4->setString(2, remote);

  if (!sqlFetchIPv4->execute()) {
    return false;
  }

  sql::ResultSet *results = sqlFetchIPv4->getResultSet();

  if (!results) {
    return false;
  } else if (!results->next()) {
    delete results;
    return false;
  }

  deviceId = results->getInt(1);
  learnMode = results->getInt(2);
  delete results;

  // Save into cache
  device_t cache = {};
  cache.id = deviceId;
  cache.study = learnMode;
  cache.time = (int)time(NULL);
  ipv4cache[key] = cache;

  return true;
}

void JaxModel::insertActivity(int id, bool learnMode) {
  prepare();

  int now = (int)time(NULL);
  auto then = lastActivity.find(id);

  if (then != lastActivity.end()) {
    int age = now - then->second;

    if (age < 15) {
      return;
    }
  }

  lastActivity[id] = now;
  sqlInsertActivity->setInt(1, id);
  sqlInsertActivity->setInt(2, learnMode ? 1 : 0);
  sqlInsertActivity->execute();
}

void JaxModel::insertTimeline(int id, std::string domain) {
  std::vector<std::string> parts = Jax::split(domain, '.');
  if (parts.size() < 2) { return; }
  parts = std::vector<std::string>(parts.end() - 2, parts.end());
  domain = Jax::join(parts, ".");
  prepare();
  sqlInsertTimeline->setInt(1, id);
  sqlInsertTimeline->setString(2, domain);
  sqlInsertTimeline->execute();
}
