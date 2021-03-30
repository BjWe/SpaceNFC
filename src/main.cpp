#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>

#include "include/cachemanager.h"
#include "include/spacebinfctagmanager.h"

using namespace std;

namespace po = boost::program_options;


void enterDoorReaderMode(boost::property_tree::ptree config, SpacebiNFCTagManager tm) {
  // Debugoutput
  cout << "Using SQLite CacheDB: " << config.get<string>("cache.db") << endl;

  Cachemanager cm(config.get<std::string>("cache.db"));

  bool terminate = false;
  while (!terminate) {
    if (tm.tagPresent()) {
      cout << "Tag present" << endl;

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
            cout << "SpacebiAPP found" << endl;

            // Türtoken lesen
            spacebi_card_doorfile_t doorfile;
            if(tm.readDoorFile(*currentTag, &doorfile)){

            } else {
              cout << "Doorfile cannot be read" << endl;
            }

          } else {
            cout << "No SpaicebiApp" << endl;
          }

          // Tag wieder loslassen
          tm.disconnectTag(*currentTag);
        } else {
          cout << "Tag not supported" << endl;
        }
      }

      usleep(1500 * 1000);  //1,5 sek
    } else {
      cout << "No Tag present" << endl;
      usleep(1500 * 1000);  //1,5 sek
    }
  }

  //cm.tokenInDB("123456789");

  cm.~Cachemanager();
}

void enterInfoReaderMode() {
}

int main(int argc, char *argv[]) {
  po::options_description desc("Alle Optionen");
  desc.add_options()("help", "hilfe")("mode", po::value<string>(), "Modus (door/info/writer)")("keyfile", po::value<string>(), "pfad zur keyfile")("configfile", po::value<string>(), "pfad zur konfiguration");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << desc << "\n";
    return 1;
  }

  // Konfigurationsdaten lesen
  if (!vm.count("configfile")) {
    cout << "Configfile missing\n";
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
    cout << "Keyfile missing\n";
    cout << desc << "\n";
    return 1;
  }

  string keyfilename = vm["keyfile"].as<string>();
  if (!boost::filesystem::exists(keyfilename)) {
    cout << "Keyfile '" << keyfilename << " not found'\n";
    return 1;
  }

  boost::property_tree::ptree keys;
  boost::property_tree::ini_parser::read_ini(keyfilename, keys);

  
  SpacebiNFCTagManager tm(&keys);
  tm.initNFC();

  // Modus prüfen
  if (!vm.count("mode")) {
    cout << "Mode missing\n";
    cout << desc << "\n";
    return 1;
  }

  string runmode = vm["mode"].as<string>();
  if (runmode == "door") {
    enterDoorReaderMode(config, tm);
  } else if (runmode == "info") {
    enterInfoReaderMode();
  } else if (runmode == "writer") {

  } else {
    cout << "invalid mode";
  }

  tm.~SpacebiNFCTagManager();

  return 0;
}