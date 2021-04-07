
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <iostream>
#include <random>

#include "include/authenticator.h"
#include "include/cachemanager.h"
#include "include/global.h"
#include "include/spacebinfctagmanager.h"

using namespace std;

namespace po = boost::program_options;

void enterDoorReaderMode(boost::property_tree::ptree config, SpacebiNFCTagManager tm, Cachemanager cm, Authenticator at) {
  bool terminate = false;
  while (!terminate) {
    if (tm.tagPresent()) {
      spdlog::debug("Tag present");

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
            spdlog::debug("SpacebiAPP found");

            tm.selectSpacebiApp(*currentTag);

            // Türtoken lesen
            spacebi_card_doorfile_t doorfile;
            if (tm.readDoorFile(*currentTag, &doorfile)) {
              dump_doorfile(doorfile);
              string doortoken = doorfile_to_hexstring(doorfile);

              string execcmd;
              if (at.checkDoorToken(doortoken)) {
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
            spdlog::warn("No SpaicebiApp");
          }

          // Tag wieder loslassen
          tm.disconnectTag(*currentTag);
        } else {
          spdlog::warn("Tag not supported");
        }
      }

      usleep(3000 * 1000);  //1,5 sek
    } else {
      spdlog::trace("No Tag present");
      usleep(1000 * 1000);  //1,5 sek
    }
  }
}

void enterInfoReaderMode(SpacebiNFCTagManager tm) {
  spdlog::trace("enter info reader mode");
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

        // Türtoken lesen
        spacebi_card_doorfile_t doorfile;
        if (tm.readDoorFile(*currentTag, &doorfile)) {
          dump_doorfile(doorfile);
        } else {
          spdlog::error("doorfile cannot be read");
        }

        // LDAP lesen
        spacebi_card_ldapuserfile_t ldapfile;
        if (tm.readLDAPFile(*currentTag, &ldapfile)) {
          dump_ldapuserfile(ldapfile);
        } else {
          spdlog::error("LDAPFile cannot be read");
        }

        for (uint8_t i = 1; i <= 4; i++) {
          spacebi_card_unique_randomfile_t randfile;
          if (tm.readRandomIDFile(*currentTag, i, &randfile)) {
            dump_uniquerandomfile(randfile);
          } else {
            spdlog::error("randfile #{} cannot be read", i);
          }
        }

        int credit;
        if (tm.readCreditFile(*currentTag, &credit)) {
          cout << "==== Credit ====" << endl;
          cout << to_string(credit) << " cents" << endl;
          cout << "================" << endl;
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

void enterFixMode(po::variables_map vm, SpacebiNFCTagManager tm) {
  spdlog::trace("enter fix mode");
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

void enterCreditMode(po::variables_map vm, SpacebiNFCTagManager tm) {
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

          if (!vm.count("amount")) {
            spdlog::error("missing amount");
            return;
          }

          int amount = vm["amount"].as<int>();

          spacebi_card_transaction_record_t ctrans;
          ctrans.amount = amount;
          ctrans.balance = amount;
          ctrans.ean = 0;
          ctrans.id = 1;

          if(!tm.storeTransaction(*currentTag, 0, ctrans)){
            spdlog::error("store was not successful");
          }

          if (amount < 0) {
            tm.creditFileDecr(*currentTag, amount * -1);
          } else if (amount > 0) {
            tm.creditFileIncr(*currentTag, amount);
          }

          int credit;
          if (tm.readCreditFile(*currentTag, &credit)) {
            cout << "==== Credit ====" << endl;
            cout << to_string(credit) << " cents" << endl;
            cout << "================" << endl;
          }

          spacebi_card_transaction_record_t transactions[SPACEBITRANSACTIONHISTORY];
          if (tm.readTransaction(*currentTag, 0, &transactions[0])) {
            cout << "==== Transactions ====" << endl;
            //for (uint8_t i = 0; i < SPACEBITRANSACTIONHISTORY; i++) {
              dump_transaction(transactions[0]);
            //}
            cout << "================" << endl;
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
    spdlog::error("no tag present");
  }
}

void enterWriterMode(po::variables_map vm, boost::property_tree::ptree config, SpacebiNFCTagManager tm, Cachemanager cm, Authenticator at) {
  spdlog::trace("entered writer mode");
  auto timenow = std::chrono::system_clock::now();

  // Random initialisieren
  std::random_device rand_device;
  std::mt19937_64 rand_64_generator(rand_device());
  std::uniform_int_distribution<uint64_t> rand_64_distributor;

  std::mt19937 rand_32_generator(rand_device());
  std::uniform_int_distribution<uint8_t> rand_8_distributor;

  // Metadaten vorbereiten
  if (!vm.count("memberid")) {
    spdlog::error("memberid missing");
  }
  spacebi_card_metainfofile_t meta;
  meta.issuedts = std::chrono::duration_cast<std::chrono::seconds>(timenow.time_since_epoch()).count();
  meta.memberid = vm["memberid"].as<int>();
  meta.structureversion = CURRENTAPPSTRUCTUREVERSION;
  meta.reserved = 0xFFFF;

  spdlog::trace("reading memberid from arg ({})", meta.memberid);

  // Türtoken vorbereiten
  spacebi_card_doorfile_t door;
  if (vm.count("doortoken")) {
    spdlog::trace("doortoken from arg present");

    string doortoken_arg = vm["doortoken"].as<string>();
    spdlog::trace("reading doortoken from arg '{}'", doortoken_arg);
    if (hexstr_to_chararray(doortoken_arg, door.token, sizeof(door.token))) {
      spdlog::trace("converted doortoken from args");
    } else {
      spdlog::error("no doortoken");
      exit(1);
    }
  } else {
    for (uint8_t i = 0; i < sizeof(door.token); i++) {
      door.token[i] = rand_8_distributor(rand_32_generator);
    }
  }

  // LDAP Username vorbereiten
  spacebi_card_ldapuserfile_t ldap;
  string ldapusername = vm["ldapusername"].as<string>();
  sprintf(ldap.username, "%.63s", ldapusername.c_str());

  spacebi_card_unique_randomfile_t rid[4];

  if (vm.count("rnd1")) {
    rid[0].randomid = hexstr_to_uint64(vm["rnd1"].as<string>());
  } else {
    rid[0].randomid = rand_64_distributor(rand_64_generator);
  }

  if (vm.count("rnd2")) {
    rid[1].randomid = hexstr_to_uint64(vm["rnd2"].as<string>());
  } else {
    rid[1].randomid = rand_64_distributor(rand_64_generator);
  }

  rid[2].randomid = rand_64_distributor(rand_64_generator);
  rid[3].randomid = rand_64_distributor(rand_64_generator);

  int creditval = 0;
  if (vm.count("amount")) {
    creditval = vm["amount"].as<int>();
  }

  spdlog::trace("enter info reader mode");
  while (!tm.tagPresent()) {
    spdlog::info("waiting for tag");
    sleep(2);
  }

  spdlog::info("Tag present");

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

        // Die App wird nun gelöscht und neu angelegt
        // Zuerst wird der bekannte Schlüssel benutzt, wenn das
        // Fehlschlägt, wird der NULLKEY versucht
        if (tm.deleteSpacebiApp(*currentTag)) {
          spdlog::info("Delete OK");

        } else {
          spdlog::error("Delete failed");
        }

      } else {
        spdlog::info("No SpaicebiApp");
      }

      tm.createSpacebiApp(*currentTag);
      if (tm.loginSpacebiApp(*currentTag, 0, false)) {
        // Daten anlegen
        tm.createMetaFile(*currentTag, meta);
        tm.createDoorFile(*currentTag, door);
        tm.createLDAPFile(*currentTag, ldap);
        for (uint8_t i = 1; i <= 4; i++) {
          tm.createRandomIDFile(*currentTag, i, rid[i]);
        }

        tm.createCreditFile(*currentTag, creditval, 0, 20000);

        tm.createTransactionFile(*currentTag);

        dump_metainfofile(meta);
        dump_doorfile(door);
        dump_ldapuserfile(ldap);
        dump_uniquerandomfile(rid[0]);
        dump_uniquerandomfile(rid[1]);
        dump_uniquerandomfile(rid[2]);
        dump_uniquerandomfile(rid[3]);

        if (vm.count("register")) {
          at.registerUser(meta.memberid, hex_str(door.token, sizeof(door.token)), rid[0].randomid, rid[1].randomid);
        }

      } else {
        spdlog::error("relogin after create app failed");
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
  desc.add_options()("help", "hilfe")("verbose,v", po::value<int>()->implicit_value(1), "verbose")("mode,m", po::value<string>(), "Modus (door/info/writer/credit)")("keyfile,k", po::value<string>(), "pfad zur keyfile")("configfile,c", po::value<string>(), "pfad zur konfiguration")("ldapusername", po::value<string>(), "ldapusername for writer")("memberid", po::value<int>(), "memberid for writer")("doortoken", po::value<string>(), "doortoken for writer")("rnd1", po::value<string>(), "rnd1 for writer")("rnd2", po::value<string>(), "rnd2 for writer")("register", po::value<string>(), "autoregister for writer")("amount", po::value<int>(), "amount for creditfile");

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

  // Modus prüfen
  if (!vm.count("mode")) {
    spdlog::error("Mode missing");
    cout << desc << "\n";
    return 1;
  }

  // Debugoutput
  spdlog::info("Using SQLite CacheDB: " + config.get<string>("cache.db"));
  spdlog::info("Using SQLite AuthDB: " + config.get<string>("auth.db"));

  Cachemanager cm(config.get<string>("cache.db"));
  Authenticator at(config.get<string>("auth.db"), cm);

  string runmode = vm["mode"].as<string>();
  if (runmode == "door") {
    enterDoorReaderMode(config, tm, cm, at);
  } else if (runmode == "info") {
    enterInfoReaderMode(tm);
  } else if (runmode == "info") {
    enterFixMode(vm, tm);
  } else if (runmode == "credit") {
    enterCreditMode(vm, tm);
  } else if (runmode == "writer") {
    enterWriterMode(vm, config, tm, cm, at);
  } else {
    spdlog::error("invalid mode");
  }

  at.~Authenticator();
  cm.~Cachemanager();

  //tm.~SpacebiNFCTagManager();

  return 0;
}