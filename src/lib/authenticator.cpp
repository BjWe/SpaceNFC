#include <iostream>

#include <boost/filesystem.hpp>
#include <chrono>
#include <iostream>

#include <spdlog/spdlog.h>

#include "include/authenticator.h"
#include "include/cachemanager.h"
#include "include/spacerestapi.h"

using namespace std;


Authenticator::Authenticator(SpaceRestApi &rapi, Cachemanager &cm){
  this->rapi = &rapi;
  this->cache = &cm;

  spdlog::trace("new Authentictatior: API Endpoint is: {}", rapi.getEndpoint());
}

Authenticator::~Authenticator() {

}

bool Authenticator::checkDoorToken(string token) {
  spdlog::trace("enter checkDoorToken");
  spdlog::trace("using Authentictatior: API Endpoint is: {}", rapi->getEndpoint());
  bool access = false;

  try{
    // FVon der RESTAPI Holen
    access = rapi->checkDoorAccess(token);

    // In den Cache eintragen oder updaten
    if(cache->tokenInDB(token)){
      spdlog::trace("token in cache => update");
      cache->updateToken(token, access);
    } else {
      spdlog::trace(" not in cache => insert");
      cache->addToken(token, access);
    }
  } catch (...){
    spdlog::warn("restapi check failed ... looking up in cache");
    if(cache->tokenInDB(token)){
      spdlog::trace("token found in cache");
      access = cache->hasAccess(token);
    }
  }

  return access;
}
