#include <spdlog/spdlog.h>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>

#include "lib/include/global.h"
#include "lib/include/nv10.h"
#include "lib/include/spacebinfctagmanager.h"
#include "lib/include/spacerestapi.h"

using namespace std;

using boost::property_tree::ptree;
namespace po = boost::program_options;

int main(int argc, char** argv) {
  po::options_description desc("Alle Optionen");
  desc.add_options()("help", "hilfe")("verbose,v", po::value<int>()->implicit_value(1), "verbose")("configfile,c", po::value<string>(), "pfad zur konfiguration")("token,t", po::value<string>(), "token")("payintype,p", po::value<string>(), "payintype (snackshop | membershipfee | donation)");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << desc << "\n";
    return 1;
  }

  if (vm.count("verbose")) {
    spdlog::set_level(spdlog::level::trace);
    spdlog::trace("set verbose level ({})", vm["verbose"].as<int>());
  }

  spdlog::info("Startup");

  // Konfigurationsdaten lesen
  if (!vm.count("configfile")) {
    spdlog::error("Configfile missing");
    cout << desc << "\n";
    return 1;
  }

  string configfilename = vm["configfile"].as<string>();
  if (!boost::filesystem::exists(configfilename)) {
    cout << "Configfile '" << configfilename << " not found'\n";
    return 1;
  }

  boost::property_tree::ptree config;
  boost::property_tree::ini_parser::read_ini(configfilename, config);

  if (!vm.count("token")) {
    spdlog::error("Token missing");
    cout << desc << "\n";
    return 1;
  }

  if (!vm.count("payintype")) {
    spdlog::error("payintype missing");
    cout << desc << "\n";
    return 1;
  }

  /*
  string payintypestr = vm["payintype"].as<string>();
  if((payintype != "snackshop") || (payintype != "membershipfee") || (payintype != "donation")){
    spdlog::error("unknown payintype '{}'", payintype);
    cout << desc << "\n";
    return 1;
  }
  */
  string payintypestr = vm["payintype"].as<string>();
  payin_e payintype;
  if (payintypestr == "snackshop") {
    payintype = payin_e::snackshop;
  } else if (payintypestr == "membershipfee") {
    payintype = payin_e::membershipfee;
  } else if (payintypestr == "donation") {
    payintype = payin_e::donation;
  } else {
    spdlog::error("unknown payintype '{}'", payintype);
    cout << desc << "\n";
    return 1;
  }

  SpaceRestApi rapi(config.get<string>("spaceapi.endpoint"), config.get<string>("spaceapi.apikey"), config.get<string>("spaceapi.token"));

  NV10SIO nv10 = NV10SIO(config.get<string>("nv10.device"));
  if (nv10.openDevice()) {
    for (uint i = 1; i <= 16; i++) {
      string key = "nv10.channel" + (boost::format("%02u") % i).str() + "_enable";
      spdlog::trace("looking for config key '{}'", key);
      boost::optional<bool> channelenable = config.get_optional<bool>(key);
      if (channelenable.has_value()) {
        spdlog::trace("channel {} has value", i);
        if (channelenable.value()) {
          nv10.enableNote(i);
        } else {
          nv10.inhibitNote(i);
        }
      } else {
        nv10.inhibitNote(i);
      }
    }
  }

  nv10.enableEscrow();
  nv10.enable();

  bool terminate = false;

  while (!terminate) {
    nv10.update();
    if (nv10.getValidChannel() > 0) {
      string key = "nv10.channel" + (boost::format("%02u") % nv10.getValidChannel()).str() + "_value";
      spdlog::trace("looking for config key '{}'", key);
      boost::optional<int> channelvalue = config.get_optional<int>(key);
      if (!channelvalue.has_value()) {
        spdlog::error("channelvalue for channel {} not found", nv10.getValidChannel());
        nv10.escrowReject();
      } else {
        ptree data;
        try {
          if (rapi.payInNote(vm["token"].as<string>(), payin_e::snackshop, channelvalue.value(), data)) {
            nv10.escrowAccept();
          } else {
            nv10.escrowReject();
          }
        } catch (...) {
          nv10.escrowReject();
        }
      }

      nv10.update();
      sleep(1);
      nv10.disable();
      terminate = true;
    }
    sleep(1);
  }
}
