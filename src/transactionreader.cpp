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

using namespace std;

namespace po = boost::program_options;

void readTransactions(SpacebiNFCTagManager tm) {

  while (!tm.tagPresent()) {
    spdlog::info("waiting for tag");
    sleep(2);
  }
  spdlog::info("tag present");

  FreefareTag *currentTag = NULL;

  // Den ersten Tag am Leser holen
  // Pointer wird auf den Tag gesetzt
  if (tm.getFirstPresentTag(&currentTag)) {
    // Wird der Tag unterstützt?
    if (tm.isTagSupported(*currentTag)) {
      // Mit Desfire Karte verbinden
      tm.connectTag(*currentTag);

      // Ist auf dem Tag die SpaceBi App installiert?
      if (tm.hasSpacebiApp(*currentTag) == SNTM_APP_OK) {
        spdlog::info("SpacebiAPP found");

        tm.selectSpacebiApp(*currentTag);

        // Meta lesen
        spacebi_card_metainfofile_t infofile;
        if (tm.readMetaFile(*currentTag, &infofile)) {
          dump_metainfofile(infofile);
        } else {
          spdlog::error("infofile cannot be read");
        }


      } else {
        spdlog::error("no spacebi app found");
      }

      // Tag wieder loslassen
      tm.disconnectTag(*currentTag);
    } else {
      spdlog::error("tag not supported");
    }
  }
}


int main(int argc, char *argv[]) {
  po::options_description desc("Alle Optionen");
  desc.add_options()
  ("help", "hilfe")("verbose,v", po::value<int>()->implicit_value(1), "verbose")
  ("keyfile,k", po::value<string>(), "pfad zur keyfile")("configfile,c", po::value<string>(), "pfad zur konfiguration");

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


  readTransactions(tm);

  //tm.~SpacebiNFCTagManager();

  return 0;
}