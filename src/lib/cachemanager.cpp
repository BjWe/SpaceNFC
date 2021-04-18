#include "include/cachemanager.h"

#include <sqlite3.h>

#include <spdlog/spdlog.h>

#include <boost/filesystem.hpp>
#include <chrono>
#include <iostream>

#include "include/global.h"

using namespace std;

Cachemanager::Cachemanager(string databasefile) {
  int rc;
  rc = sqlite3_open(databasefile.c_str(), &db);
  if (rc) {
    spdlog::error("can't open db ({})", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(1);
  }

  sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS \"doortokencache\"(\"token\" TEXT, \"access\" INTEGER, \"lastseen\" INTEGER);", NULL, NULL, NULL);

}

Cachemanager::~Cachemanager() {
  sqlite3_close(db);
}

bool Cachemanager::tokenInDB(string token) {
  sqlite3_stmt *stmt;

  int rc;
  rc = sqlite3_prepare_v2(db, "select count(*) from doortokencache where token = ?", -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    spdlog::error("sqlite prepare failed");
    return false;
  }

  sqlite3_bind_text(stmt, 1, token.c_str(), -1, NULL);
  rc = sqlite3_step(stmt);
  //cout << rc << endl;
  if (rc == SQLITE_ROW) {
    int colcount = sqlite3_column_int(stmt, 0);
    spdlog::debug("Found {} entries", colcount);

    return colcount > 0;
  } else {
    spdlog::error("Error while searching");
    return false;
  }
  return false;
}

bool Cachemanager::hasAccess(string token) {
  sqlite3_stmt *stmt;

  int rc;
  rc = sqlite3_prepare_v2(db, "select * from doortokencache where token = ?", -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    spdlog::error("sqlite prepare failed");
    return false;
  }

  sqlite3_bind_text(stmt, 1, token.c_str(), -1, NULL);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    int access = sqlite3_column_int(stmt, 1);
    int64_t lastseen = sqlite3_column_int64(stmt, 2);
    spdlog::debug("Found entry access: {}  lastseen: {}", access, lastseen);

    auto timenow = std::chrono::system_clock::now();
    int64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(timenow.time_since_epoch()).count();

    if((lastseen + DEFAULT_CACHE_GRACE_PERIOD) > timestamp){
      updateToken(token, access);
      return access == 1;
    } else {
      return false;
    }

  } else {
    spdlog::error("Error while searching");
    return false;
  }
  return false;
}

bool Cachemanager::addToken(string token, bool access) {
  sqlite3_stmt *stmt;

  int rc;
  rc = sqlite3_prepare_v2(db, "insert into doortokencache VALUES (?, ?, ?)", -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    spdlog::error("Prepare failed");
    return false;
  }

  auto timenow = std::chrono::system_clock::now();
  uint_fast64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(timenow.time_since_epoch()).count();


  sqlite3_bind_text(stmt, 1, token.c_str(), -1, NULL);
  sqlite3_bind_int(stmt, 2, access ? 1 : 0);
  sqlite3_bind_int64(stmt, 3, timestamp);

  sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return false;
}

bool Cachemanager::updateToken(string token, bool access) {
  sqlite3_stmt *stmt;

  int rc;
  rc = sqlite3_prepare_v2(db, "update doortokencache set access = ?, lastseen = ? where token = ?", -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    spdlog::error("Prepare failed");
    return false;
  }

  auto timenow = std::chrono::system_clock::now();
  uint_fast64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(timenow.time_since_epoch()).count();


  sqlite3_bind_text(stmt, 3, token.c_str(), -1, NULL);
  sqlite3_bind_int(stmt, 1, access ? 1 : 0);
  sqlite3_bind_int64(stmt, 2, timestamp);

  sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return false;
}