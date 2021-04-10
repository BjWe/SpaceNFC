#ifndef __CACHEMANAGER_H__
#define __CACHEMANAGER_H__

#include <iostream>
#include <sqlite3.h>
using namespace std;


class Cachemanager {
 protected:
   sqlite3 *db;
 public:
  Cachemanager(string databasefile);
  ~Cachemanager();
  bool tokenInDB(string token);
  bool hasAccess(string token);
  bool addToken(string token, bool access);
  bool updateToken(string token, bool access);
};

#endif