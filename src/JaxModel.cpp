
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
    "select `id`, `study`, `device`.`blockads`, `device`.`youtube_restrict` "
    "from `device` where (`ipv6` = ?) "
    "limit 1"
  );

  prepareSql(sqlFetchIPv4,
    "select `device`.`id`, `device`.`study`, `device`.`blockads`, `device`.`youtube_restrict` "
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
    domain.group = resultSet->getInt("group");
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

bool JaxModel::fetch(JaxClient& client, JaxDevice& device) {
  prepare();
  Jax::debug("Looking up IP %s from %s", client.local.c_str(), client.remote.c_str());

  if (client.ipv4) {
    return fetchIPv4(client.local, client.remote, device);
  } else {
    return fetchIPv6(client.local, device);
  }
}

bool JaxModel::readDevice(JaxDevice& device, sql::PreparedStatement *statement) {
  if (!statement) {
    throw std::runtime_error("Empty statement encountered");
  } else if (!statement->execute()) {
    throw std::runtime_error("Unable to execute statement");
  }

  sql::ResultSet *results = statement->getResultSet();

  if (!results) {
    return false;
  } else if (!results->next()) {
    delete results;
    return false;
  }

  device.time = (int)time(NULL);
  device.id = results->getInt("id");
  device.study = results->getInt("study");
  device.blockads = results->getInt("blockads");
  device.youtubeRestrict = results->getInt("youtube_restrict");
  device.lastActivity = 0;
  deviceCache[device.id] = device;
  delete results;
  return true;
}

bool JaxModel::fetchIPv6(const std::string address, JaxDevice& device) {
  int now = (int)time(NULL);
  auto idEntry = ipv6cache.find(address);

  if (idEntry != ipv6cache.end()) {
    int id = idEntry->second;
    auto deviceEntry = deviceCache.find(id);

    if (deviceEntry != deviceCache.end()) {
      auto cached = deviceEntry->second;
      int age = now - cached.time;

      if (age < 15) {
        device = cached;
        return true;
      }
    }
  }

  sqlFetchIPv6->setString(1, address);
  return readDevice(device, sqlFetchIPv6);
}

bool JaxModel::fetchIPv4(const std::string local, const std::string remote, JaxDevice& device) {
  // TODO this should be a vector of two strings
  std::string key = local + "\n" + remote;

  int now = (int)time(NULL);
  auto idEntry = ipv4cache.find(key);

  if (idEntry != ipv4cache.end()) {
    int id = idEntry->second;
    auto deviceEntry = deviceCache.find(id);

    if (deviceEntry != deviceCache.end()) {
      auto cached = deviceEntry->second;
      int age = now - cached.time;

      if (age < 15) {
        device = cached;
        return true;
      }
    }
  }

  sqlFetchIPv4->setString(1, local);
  sqlFetchIPv4->setString(2, remote);
  return readDevice(device, sqlFetchIPv4);
}

void JaxModel::insertActivity(int id, bool learnMode) {
  prepare();

  int now = (int)time(NULL);
  auto deviceEntry = deviceCache.find(id);

  if (deviceEntry != deviceCache.end()) {
    auto& device = deviceEntry->second;
    int age = now - device.lastActivity;

    if (age < 15) {
      return;
    }

    device.lastActivity = now;
  }

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
