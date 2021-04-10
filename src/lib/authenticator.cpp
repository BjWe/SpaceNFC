#include <iostream>

#include <sqlite3.h>

#include <boost/filesystem.hpp>
#include <chrono>
#include <iostream>

#include <spdlog/spdlog.h>

#include "include/authenticator.h"

using namespace std;

Authenticator::Authenticator(string databasefile, Cachemanager cm) {
  int rc;
  rc = sqlite3_open(databasefile.c_str(), &db);
  if (rc) {
    spdlog::error("can't open db ({})", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(1);
  } else {
    spdlog::debug("openend auth db");
  }
}

Authenticator::~Authenticator() {
  sqlite3_close(db);
}

bool Authenticator::checkDoorToken(string token) {
  sqlite3_stmt *stmt;

  int rc;
  rc = sqlite3_prepare_v2(db, "select count(*) from doortoken where token = ? and allowed = 1", -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    spdlog::error("Prepare failed ({}}) {}", rc, sqlite3_errmsg(db));
    return false;
  }

  sqlite3_bind_text(stmt, 1, token.c_str(), -1, NULL);
  rc = sqlite3_step(stmt);
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

bool Authenticator::registerUser(int memberid, string doortoken, uint64_t rndid1, uint64_t rndid2){
  sqlite3_stmt *stmt;

  int rc;
  rc = sqlite3_prepare_v2(db, "insert into user VALUES (?, ?, ?)", -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    spdlog::error("Prepare failed ({}}) {}", rc, sqlite3_errmsg(db));
    return false;
  }

  sqlite3_bind_int(stmt, 1, memberid);
  sqlite3_bind_int64(stmt, 2, rndid1);
  sqlite3_bind_int64(stmt, 3, rndid2);
  sqlite3_step(stmt);

  
  rc = sqlite3_prepare_v2(db, "insert into doortoken VALUES (?, ?, ?)", -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    spdlog::error("Prepare failed ({}}) {}", rc, sqlite3_errmsg(db));
    return false;
  }

  sqlite3_bind_int(stmt, 1, memberid);
  sqlite3_bind_text(stmt, 2, doortoken.c_str(), -1, NULL);
  sqlite3_bind_int(stmt, 3, 1);
  sqlite3_step(stmt);
   

  return false;  
}