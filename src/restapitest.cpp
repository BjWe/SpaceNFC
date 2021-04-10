#include <spdlog/spdlog.h>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>

#include "lib/include/global.h"
#include "lib/include/spacerestapi.h"

using namespace std;

namespace po = boost::program_options;


int main(int argc, char *argv[]) {
  po::options_description desc("Alle Optionen");
  desc.add_options()
  ("help", "hilfe")("verbose,v", po::value<int>()->implicit_value(1), "verbose")
  ("configfile,c", po::value<string>(), "pfad zur konfiguration");

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

  SpaceRestApi rapi(config.get<string>("spaceapi.endpoint"), config.get<string>("spaceapi.apikey"),config.get<string>("spaceapi.token"));
  boost::property_tree::ptree userdata;
  rapi.fetchInitDate(10032, userdata);

  cout << rapi.checkDoorAccess("a58864385626542e75d8ff756a676c72c3876a48e7f62d4c86a5d62c6f5e95a7") << endl;

  return 0;
}