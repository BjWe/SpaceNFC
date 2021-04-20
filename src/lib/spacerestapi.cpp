#include "include/spacerestapi.h"

#include <spdlog/spdlog.h>

#include <boost/optional.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>

#include "Poco/Exception.h"
#include "Poco/Net/AcceptCertificateHandler.h"
#include "Poco/Net/Context.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/HTTPSStreamFactory.h"
#include "Poco/Net/HTTPStreamFactory.h"
#include "Poco/Net/InvalidCertificateHandler.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/StreamCopier.h"
#include "Poco/URI.h"
#include "Poco/URIStreamOpener.h"
#include "boost/property_tree/json_parser.hpp"
#include "boost/property_tree/ptree.hpp"

#include "include/utils.h"

using namespace std;
using boost::property_tree::ptree;

SpaceRestApi::SpaceRestApi(string endpoint, string apikey, string token) {
  this->endpoint = endpoint;
  this->apikey = apikey;
  this->token = token;
}

SpaceRestApi::~SpaceRestApi() {
}

string SpaceRestApi::buildUri(string path) {
  return "https://" + endpoint + path;
}

void SpaceRestApi::setApiHeader(Poco::Net::HTTPRequest& req) {
  req.set("X-Api-Key", apikey);
  req.set("X-Servicetoken", token);
  req.setContentType("application/json");
}

int SpaceRestApi::fetchDataFromApi(string method, string path, ptree datain, ptree& dataout) {
  string struri = buildUri(path);
  spdlog::trace("make {} http request to '{}'", method, struri);
  Poco::URI uri(struri);

  Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort());
  Poco::Net::HTTPRequest request;

  request.setURI(uri.getPathAndQuery());

  session.setTimeout(Poco::Timespan(2, 0));

  session.setKeepAlive(true);
  request.setKeepAlive(true);

  setApiHeader(request);

  request.setMethod(method);
  if (method == Poco::Net::HTTPRequest::HTTP_POST) {
    std::stringstream outss;
    boost::property_tree::json_parser::write_json(outss, datain, false);
    request.setContentLength(outss.str().length());

    spdlog::trace("JSON to send: {}", outss.str());

    std::ostream& os = session.sendRequest(request);
    os << outss.str();
  } else {
    session.sendRequest(request);
  }

  Poco::Net::HTTPResponse response;
  istream& rs = session.receiveResponse(response);

  spdlog::trace("request done. returncode: {}", response.getStatus());

  boost::property_tree::read_json(rs, dataout);

  return response.getStatus();
}

void SpaceRestApi::fetchInitDate(int memberid, ptree& data) {
  ptree datain;
  datain.add("memberid", memberid);

  int returncode = fetchDataFromApi(Poco::Net::HTTPRequest::HTTP_POST, "/service/mifare/initdata", datain, data);
  spdlog::debug("HTTP Returncode: {}", returncode);

  std::stringstream ss;
  boost::property_tree::json_parser::write_json(ss, data);

  std::cout << ss.str() << std::endl;
}

bool SpaceRestApi::checkDoorAccess(string doortoken) {
  ptree datain;
  datain.add("token", doortoken);

  ptree dataout;

  int returncode = fetchDataFromApi(Poco::Net::HTTPRequest::HTTP_POST, "/service/mifare/doorcheck", datain, dataout);
  spdlog::debug("HTTP Returncode: {}", returncode);

  if (returncode != 200) {
    throw returncode;
  }

  auto access = dataout.get_optional<bool>("access");
  if (access) {
    return access.value();
  }

  return false;
}

bool SpaceRestApi::transmitSnackshopCart(string financetoken, vector<ProductAmountPair> cart, double clientprice, ptree &dataout) {
  
  ptree datain;
  datain.add("token", financetoken);
  datain.add("price", clientprice);

  ptree products;
  for(size_t i = 0; i < cart.size(); i++){
    ptree entry;
    entry.add("id", cart[i].id);
    entry.add("amount", cart[i].amount);
    products.push_back(make_pair("", entry));
  }

  datain.add_child("products", products);  

  int returncode = fetchDataFromApi(Poco::Net::HTTPRequest::HTTP_POST, "/service/mifare/checkoutsnackshopcart", datain, dataout);
  spdlog::debug("HTTP Returncode: {}", returncode);

  if (returncode != 200) {
    throw returncode;
  }

  auto result = dataout.get_optional<bool>("result");
  if (result) {
    return result.value();
  }

  return false;
}

/*
void SpaceRestApi::fetchInitDate(int memberid, boost::property_tree::ptree& data) {
  string struri = buildUri("/service/mifare/initdata");
  Poco::URI uri(struri);

  Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort());
  Poco::Net::HTTPRequest request;

  request.setURI(uri.getPathAndQuery());

  session.setKeepAlive(true);
  request.setKeepAlive(true);

  setApiHeader(request);

  request.setMethod(Poco::Net::HTTPRequest::HTTP_GET);
  

  string tosend = "{\"memberid\": 10032}";
  request.setContentLength(tosend.length());

  std::ostream& os = session.sendRequest(request);
  os << tosend;

  Poco::Net::HTTPResponse response;
  istream& rs = session.receiveResponse(response);
  string sResponseReason = string(response.getReason());
  ostringstream oss;
  Poco::StreamCopier::copyStream(rs, oss);
  string sRet = oss.str();
  std::cout << "Response Status : " << response.getStatus() << std::endl;

  if (sResponseReason.compare("OK") == 0) {
    printf("HTTPReponse OK :::  %s for <%s> \n", sResponseReason.c_str(), struri.c_str());
  } else {
    printf("HTTPReponse NOT OK ::  %s:<%s> for <%s> \n", sResponseReason.c_str(), sRet.c_str(), struri.c_str());
    sRet = "";
  }
}
*/