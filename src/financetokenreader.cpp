#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <iostream>
#include <random>

#include "lib/include/global.h"
#include "lib/include/spacebinfctagmanager.h"
#include "lib/include/spacerestapi.h"

using namespace std;

namespace po = boost::program_options;

void readFinanceKey(SpacebiNFCTagManager tm, po::variables_map vm) {
  boost::property_tree::ptree jsonout;

  int timeout = 10;
  while (!tm.tagPresent() & (timeout >= 0)) {
    spdlog::info("waiting for tag");
    timeout--;
    sleep(1);
  }

  if (timeout < 0) {
    jsonout.add("error", "timeout");
    spdlog::warn("timeout tag read");
  } else {
    spdlog::info("tag present");
    cout << "----TAG PRESENT----" << endl;

    FreefareTag *currentTag = NULL;

    // Den ersten Tag am Leser holen
    // Pointer wird auf den Tag gesetzt
    if (tm.getFirstPresentTag(&currentTag)) {
      // Wird der Tag unterstützt?
      if (tm.isTagSupported(*currentTag)) {
        // Mit Desfire Karte verbinden
        if (tm.connectTag(*currentTag)) {
          // Ist auf dem Tag die SpaceBi App installiert?
          if (tm.hasSpacebiApp(*currentTag) == SNTM_APP_OK) {
            spdlog::info("SpacebiAPP found");

            tm.selectSpacebiApp(*currentTag);

            // Payment-Meta-Lesen
            spacebi_card_creditmetafile_t cmf;
            if (tm.readCreditMetaFile(*currentTag, &cmf)) {
              dump_creditmetafile(cmf);
              string financetoken = creditmetafile_to_hexstring(cmf);
              jsonout.add("error", "");
              jsonout.add("token", financetoken);

            } else {
              jsonout.add("error", "readfailed");
            }

          } else {
            jsonout.add("error", "noapp");
            spdlog::error("no spacebi app found");
          }

          // Tag wieder loslassen
          tm.disconnectTag(*currentTag);
        } else {
          jsonout.add("error", "connfailed");
          spdlog::error("connect failed");
        }
      } else {
        jsonout.add("error", "unsupported");
        spdlog::error("tag not supported");
      }
    }
  }

  stringstream ss;
  boost::property_tree::json_parser::write_json(ss, jsonout, vm.count("verbose"));
  cout << "----BEGIN JSON----" << endl
       << ss.str() << "----END JSON----" << endl;
}

int main(int argc, char *argv[]) {
  po::options_description desc("Alle Optionen");
  desc.add_options()("help", "hilfe")("verbose,v", po::value<int>()->implicit_value(1), "verbose")("keyfile,k", po::value<string>(), "pfad zur keyfile")("configfile,c", po::value<string>(), "pfad zur konfiguration");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << desc << "\n";
    return 1;
  }

  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_st>("logs/financetokenreader", 23, 59));
  auto combined_logger = std::make_shared<spdlog::logger>("financetokenreader", begin(sinks), end(sinks));
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

  // Keyfile lesen
  if (!vm.count("keyfile")) {
    spdlog::error("Keyfile missing");
    cout << desc << "\n";
    return 1;
  }

  string keyfilename = vm["keyfile"].as<string>();
  if (!boost::filesystem::exists(keyfilename)) {
    spdlog::error("Keyfile '" + keyfilename + "' not found");
    return 1;
  }

  boost::property_tree::ptree keys;
  boost::property_tree::ini_parser::read_ini(keyfilename, keys);

  SpacebiNFCTagManager tm(&keys);
  tm.initNFC();

  readFinanceKey(tm, vm);

  //tm.~SpacebiNFCTagManager();

  return 0;
}