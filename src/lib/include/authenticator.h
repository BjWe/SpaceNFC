#ifndef __AUTHENTICATOR_H_
#define __AUTHENTICATOR_H_

#include <iostream>
#include "spacerestapi.h"
#include "cachemanager.h"

using namespace std;

class Authenticator {
 protected:
   Cachemanager *cache;
   SpaceRestApi *rapi;
 public:
  Authenticator(SpaceRestApi &rapi, Cachemanager &cm);
  ~Authenticator();
  bool checkDoorToken(string token);
};

#endif