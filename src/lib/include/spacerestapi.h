#ifndef __SPACERESTAPI_H__
#define __SPACERESTAPI_H__

#include <iostream>

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"

#include "Poco/Net/HTTPRequest.h"

#include "utils.h"

using namespace std;
using boost::property_tree::ptree;

class SpaceRestApi {
    protected:
      string endpoint;
      string apikey;
      string token;

      string buildUri(string path);
      void setApiHeader(Poco::Net::HTTPRequest& req);
      int fetchDataFromApi(string method, string path, ptree datain, ptree& dataout);

    public:
      SpaceRestApi(string endpoint, string apikey, string token);
      ~SpaceRestApi();

      void fetchInitDate(int memberid, ptree& data);
      bool checkDoorAccess(string doortoken);
      bool transmitSnackshopCart(string financetoken, vector<ProductAmountPair> cart, double clientprice, ptree& dataout);

      string getEndpoint(){
        return endpoint;
      }

};

#endif