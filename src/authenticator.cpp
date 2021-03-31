#include <iostream>

#include <sqlite3.h>

#include <boost/filesystem.hpp>
#include <chrono>
#include <iostream>

#include "include/authenticator.h"

using namespace std;

Authenticator::Authenticator(string databasefile, Cachemanager cm) {
  int rc;
  rc = sqlite3_open(databasefile.c_str(), &db);
  if (rc) {
    cout << "can't open db " << sqlite3_errmsg(db);
    sqlite3_close(db);
    exit(1);
  } else {
    cout << "openend auth db" << endl;
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
    cout << "Prepare failed (" << to_string(rc) << ") " << sqlite3_errmsg(db) << "\n";
    return false;
  }

  sqlite3_bind_text(stmt, 1, token.c_str(), -1, NULL);
  rc = sqlite3_step(stmt);
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