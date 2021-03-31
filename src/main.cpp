
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <iostream>
#include <random>

#include "include/authenticator.h"
#include "include/cachemanager.h"
#include "include/spacebinfctagmanager.h"

using namespace std;

namespace po = boost::program_options;

void enterDoorReaderMode(boost::property_tree::ptree config, SpacebiNFCTagManager tm) {
  // Debugoutput
  cout << "Using SQLite CacheDB: " << config.get<string>("cache.db") << endl;
  cout << "Using SQLite AuthDB: " << config.get<string>("auth.db") << endl;

  Cachemanager cm(config.get<string>("cache.db"));
  Authenticator at(config.get<string>("auth.db"), cm);

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

            tm.selectSpacebiApp(*currentTag);

            // Türtoken lesen
            spacebi_card_doorfile_t doorfile;
            if (tm.readDoorFile(*currentTag, &doorfile)) {
              dump_doorfile(doorfile);
              string doortoken = doorfile_to_hexstring(doorfile);

              string execcmd;
              if (at.checkDoorToken(doortoken)) {
                execcmd = "bash " + config.get<string>("door.exec_on_auth_success") + " &";
                cout << "Exec Succ: " << execcmd;
                system(execcmd.c_str());
              } else {
                execcmd = "bash " + config.get<string>("door.exec_on_auth_failed") + " &";
                cout << "Exec Fail: " << execcmd;
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

  at.~Authenticator();
  cm.~Cachemanager();
}

void enterInfoReaderMode(SpacebiNFCTagManager tm) {
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

          tm.selectSpacebiApp(*currentTag);

          // Meta lesen
          spacebi_card_metainfofile_t infofile;
          if (tm.readMetaFile(*currentTag, &infofile)) {
            dump_metainfofile(infofile);
          } else {
            cout << "Infofile cannot be read" << endl;
          }

          // Türtoken lesen
          spacebi_card_doorfile_t doorfile;
          if (tm.readDoorFile(*currentTag, &doorfile)) {
            dump_doorfile(doorfile);
          } else {
            cout << "Doorfile cannot be read" << endl;
          }

          // LDAP lesen
          spacebi_card_ldapuserfile_t ldapfile;
          if (tm.readLDAPFile(*currentTag, &ldapfile)) {
            dump_ldapuserfile(ldapfile);
          } else {
            cout << "LDAPFile cannot be read" << endl;
          }

          for (uint8_t i = 1; i <= 4; i++) {
            spacebi_card_unique_randomfile_t randfile;
            if (tm.readRandomIDFile(*currentTag, i, &randfile)) {
              dump_uniquerandomfile(randfile);
            } else {
              cout << "Randfile " << i << " cannot be read" << endl;
            }
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

  } else {
    cout << "No Tag present" << endl;
  }
}

void enterWriterMode(po::variables_map vm, boost::property_tree::ptree config, SpacebiNFCTagManager tm) {
  auto timenow = std::chrono::system_clock::now();

  // Random initialisieren
  std::random_device rand_device;
  std::mt19937_64 rand_64_generator(rand_device());
  std::uniform_int_distribution<uint64_t> rand_64_distributor;

  std::mt19937 rand_32_generator(rand_device());
  std::uniform_int_distribution<uint8_t> rand_8_distributor;

  // Metadaten vorbereiten
  spacebi_card_metainfofile_t meta;
  meta.issuedts = std::chrono::duration_cast<std::chrono::seconds>(timenow.time_since_epoch()).count();
  meta.memberid = vm["memberid"].as<int>();
  meta.reserved = 0xFFFF;

  // Türtoken vorbereiten
  spacebi_card_doorfile_t door;
  for (uint8_t i = 0; i < sizeof(door.token); i++) {
    door.token[i] = rand_8_distributor(rand_32_generator);
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

          // Die App wird nun gelöscht und neu angelegt
          // Zuerst wird der bekannte Schlüssel benutzt, wenn das
          // Fehlschlägt, wird der NULLKEY versucht
          if (tm.deleteSpacebiApp(*currentTag)) {
            cout << "Delete OK" << endl;

          } else {
            cout << "Delete failed" << endl;
          }

        } else {
          cout << "No SpaicebiApp" << endl;
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

          dump_metainfofile(meta);
          dump_doorfile(door);
          dump_ldapuserfile(ldap);
          dump_uniquerandomfile(rid[0]);
          dump_uniquerandomfile(rid[1]);
          dump_uniquerandomfile(rid[2]);
          dump_uniquerandomfile(rid[3]);

        } else {
          cout << "Relogin nach create fehlgeschlagen" << endl;
        }

        // Tag wieder loslassen
        tm.disconnectTag(*currentTag);

      } else {
        cout << "Tag not supported" << endl;
      }
    }

  } else {
    cout << "No Tag present" << endl;
  }
}

int main(int argc, char *argv[]) {
  po::options_description desc("Alle Optionen");
  desc.add_options()("help", "hilfe")("mode", po::value<string>(), "Modus (door/info/writer)")("keyfile", po::value<string>(), "pfad zur keyfile")("configfile", po::value<string>(), "pfad zur konfiguration")("ldapusername", po::value<string>(), "ldapusername for writer")("memberid", po::value<int>(), "memberid for writer")("rnd1", po::value<string>(), "rnd1 for writer")("rnd2", po::value<string>(), "rnd2 for writer");

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
    enterInfoReaderMode(tm);
  } else if (runmode == "writer") {
    enterWriterMode(vm, config, tm);
  } else {
    cout << "invalid mode";
  }

  tm.~SpacebiNFCTagManager();

  return 0;
}