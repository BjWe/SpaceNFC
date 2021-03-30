#include "include/cachemanager.h"

#include <sqlite3.h>

#include <boost/filesystem.hpp>
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

Cachemanager::~Cachemanager(){
    sqlite3_close(db);
}

bool Cachemanager::tokenInDB(string token) {
    sqlite3_stmt *stmt;

    int rc;
    rc = sqlite3_prepare_v2(db, "select count(*) from doortokens where token = ?", -1, &stmt, 0);
    if(rc != SQLITE_OK){
      cout << "Prepare failed\n";
      return false; 
    } 

    sqlite3_bind_text(stmt, 1, token.c_str(), -1, NULL);
    rc = sqlite3_step(stmt);
    //cout << rc << endl;
    if(rc==SQLITE_ROW) {
        int colcount = sqlite3_column_int(stmt, 0);
        cout << "Found " << colcount << " entries\n";
     
        return colcount > 0;
    } else {
        cout << "Error while searching\n";
        return false;
    }
  return false;
}