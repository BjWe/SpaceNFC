#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/DatagramSocket.h>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <iostream>
#include <random>

#include "lib/include/global.h"
#include "lib/include/serialbarcodereader.h"

using namespace std;

namespace po = boost::program_options;

string destinationaddr = "";
int destinationport = 0;

void barcodeScanned(string barcode){
  cout << barcode << "\r\n"; 
  
  try{
    spdlog::debug("sending a datagram to {}:{}", destinationaddr, destinationport);
    Poco::Net::SocketAddress sa(destinationaddr, destinationport);
    Poco::Net::DatagramSocket dgs;
    dgs.connect(sa);
    dgs.sendBytes(barcode.data(), barcode.size());
  } catch(Poco::IOException &ex){
    spdlog::error("Exception {0:s}", ex.displayText());
  } catch(exception &ex){
    spdlog::error("Exception {0:s}", ex.what());
  }
  
}

int main(int argc, char *argv[]) {
  po::options_description desc("Alle Optionen");
  desc.add_options()("help", "hilfe")("verbose,v", po::value<int>()->implicit_value(1), "verbose")("configfile,c", po::value<string>(), "pfad zur konfiguration");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << desc << "\n";
    return 1;
  }

  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_st>("logs/barcodereader", 23, 59));
  auto combined_logger = std::make_shared<spdlog::logger>("barcodereader", begin(sinks), end(sinks));
  combined_logger->flush_on(spdlog::level::debug);
  spdlog::set_default_logger(combined_logger);

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

  // Quick and dirty - set packetdestination as global variable | configuration not available in callback
  destinationaddr = config.get<string>("barcodereader.destinationaddr");
  destinationport = config.get<int>("barcodereader.destinationport");

  SerialBarcodeReader sbr = SerialBarcodeReader(config.get<string>("barcodereader.device"), &barcodeScanned);
  if (sbr.openDevice()) {
    while(1){
      sbr.update();
      usleep(200);
    }
  } else {
    return -1;
  }


  return 0;
}