#include "include/cachemanager.h"

#include <sqlite3.h>

#include <boost/filesystem.hpp>
#include <chrono>
#include <iostream>

using namespace std;

Cachemanager::Cachemanager(string databasefile) {
  int rc;
  rc = sqlite3_open(databasefile.c_str(), &db);
  if (rc) {
    cout << "can't open db " << sqlite3_errmsg(db);
    sqlite3_close(db);
    exit(1);
  }
}

Cachemanager::~Cachemanager() {
  sqlite3_close(db);
}

bool Cachemanager::tokenInDB(string token) {
  sqlite3_stmt *stmt;

  int rc;
  rc = sqlite3_prepare_v2(db, "select count(*) from doortokens where token = ?", -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    cout << "Prepare failed\n";
    return false;
  }

  sqlite3_bind_text(stmt, 1, token.c_str(), -1, NULL);
  rc = sqlite3_step(stmt);
  //cout << rc << endl;
  if (rc == SQLITE_ROW) {
    int colcount = sqlite3_column_int(stmt, 0);
    cout << "Found " << colcount << " entries\n";

    return colcount > 0;
  } else {
    cout << "Error while searching\n";
    return false;
  }
  return false;
}

bool Cachemanager::addToken(string token, bool access) {
  sqlite3_stmt *stmt;

  int rc;
  rc = sqlite3_prepare_v2(db, "insert into doortokens VALUES (?, ?, ?)", -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    cout << "Prepare failed\n";
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
  rc = sqlite3_prepare_v2(db, "update doortokens set access = ?, lastseen = ? where token = ?", -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    cout << "Prepare failed\n";
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