#include <spdlog/spdlog.h>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <iostream>
#include <random>

#include "lib/include/authenticator.h"
#include "lib/include/global.h"
#include "lib/include/spacebinfctagmanager.h"
#include "lib/include/spacerestapi.h"

using namespace std;

using boost::property_tree::ptree;
namespace po = boost::program_options;

void readAndCheckDoorfile(Authenticator auth, SpacebiNFCTagManager tm, ptree config) {
  if (tm.tagPresent()) {
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

          // Türtoken lesen
          spacebi_card_doorfile_t doorfile;
          if (tm.readDoorFile(*currentTag, &doorfile)) {
            dump_doorfile(doorfile);
            string doortoken = doorfile_to_hexstring(doorfile);

            string execcmd;
            if (auth.checkDoorToken(doortoken)) {
              execcmd = "bash " + config.get<string>("door.exec_on_auth_success") + " &";
              spdlog::trace("Exec Succ: {}", execcmd);
              system(execcmd.c_str());
            } else {
              execcmd = "bash " + config.get<string>("door.exec_on_auth_failed") + " &";
              spdlog::trace("Exec Fail: {}", execcmd);
              system(execcmd.c_str());
            }
            /*
              if(cm.tokenInDB(doortoken)){
                cout << "found in cache" << endl;
              } else {
                cm.addToken(doortoken, true);
              }
*/
            cout << "string rep:" << endl
                 << doorfile_to_hexstring(doorfile) << endl;
            //cm.tokenInDB()

          } else {
            spdlog::error("Doorfile cannot be read");
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
  } else {
    spdlog::debug("waiting for tag");
  }
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

  SpaceRestApi rapi(config.get<string>("spaceapi.endpoint"), config.get<string>("spaceapi.apikey"), config.get<string>("spaceapi.token"));
  Cachemanager cm(config.get<string>("cache.db"));
  Authenticator auth(rapi, cm);

  while (1) {
    readAndCheckDoorfile(auth, tm, config);
    sleep(1);
  }

  return 0;
}