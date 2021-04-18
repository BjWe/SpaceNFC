#include <spdlog/spdlog.h>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <iostream>
#include <random>

#include "lib/include/global.h"
#include "lib/include/spacebinfctagmanager.h"
#include "lib/include/spacerestapi.h"

using namespace std;

namespace po = boost::program_options;

void writeTransaction(SpacebiNFCTagManager tm, boost::property_tree::ptree config, po::variables_map vm) {
  if (!vm.count("memberid")) {
    spdlog::error("no memberid");
  }

  int memberid = 0;
  try {
    memberid = vm["memberid"].as<int>();
  } catch (const exception ex) {
    spdlog::error("'{}' is no valid memberid", vm["memberid"].as<string>());
    return;
  }

  // Infos ziehen
  SpaceRestApi rapi(config.get<string>("spaceapi.endpoint"), config.get<string>("spaceapi.apikey"), config.get<string>("spaceapi.token"));
  boost::property_tree::ptree userdata;
  try {
    rapi.fetchInitDate(memberid, userdata);
  } catch (const exception ex) {
    spdlog::error("userdata fetch failed ({})", ex.what());
    return;
  }

  if (!userdata.count("doortoken")) {
    spdlog::error("doortoken missing");
    return;
  }

  if (!userdata.count("ldapusername")) {
    spdlog::error("ldapusername missing");
    return;
  }

  if (!userdata.count("rnd1")) {
    spdlog::error("rnd1 missing");
    return;
  }

  if (!userdata.count("rnd2")) {
    spdlog::error("rnd2 missing");
    return;
  }

  if (!userdata.count("snackcreditid")) {
    spdlog::error("snackcreditid missing");
    return;
  }

  // Daten vorbereiten
  auto timenow = std::chrono::system_clock::now();

  // Random initialisieren
  random_device rand_device;
  mt19937_64 rand_64_generator(rand_device());
  uniform_int_distribution<uint64_t> rand_64_distributor;

  mt19937 rand_32_generator(rand_device());
  uniform_int_distribution<uint8_t> rand_8_distributor;

  // Metainfo
  spacebi_card_metainfofile_t meta;
  init_metainfofile(&meta);
  meta.structureversion = CURRENTAPPSTRUCTUREVERSION;

  // Personalien
  spacebi_card_personalinfofile_t personal;
  init_personalinfofile(&personal);
  personal.issuedts = std::chrono::duration_cast<std::chrono::seconds>(timenow.time_since_epoch()).count();
  personal.memberid = memberid;

  // Tür
  spacebi_card_doorfile_t door;
  if (hexstr_to_chararray(userdata.get<string>("doortoken"), door.token, sizeof(door.token))) {
    spdlog::trace("converted doortoken from hexstr");
  } else {
    spdlog::error("db provided doortoken is broken");
  }

  // LDAP Username vorbereiten
  spacebi_card_ldapuserfile_t ldap;
  init_ldapuserfile(&ldap);
  string ldapusername = userdata.get<string>("ldapusername");
  sprintf(ldap.username, "%.63s", ldapusername.c_str());

  spacebi_card_unique_randomfile_t rid[4];

  rid[0].randomid = hexstr_to_uint64(userdata.get<string>("rnd1"));
  rid[1].randomid = hexstr_to_uint64(userdata.get<string>("rnd2"));
  rid[2].randomid = rand_64_distributor(rand_64_generator);
  rid[3].randomid = rand_64_distributor(rand_64_generator);

  spacebi_card_creditmetafile_t creditmeta;
  init_creditmetafile(&creditmeta);
  creditmeta.lasttransaction = 0;
  if (hexstr_to_chararray(userdata.get<string>("snackcreditid"), creditmeta.token, sizeof(creditmeta.token))) {
    spdlog::trace("converted snackcreditid from hexstr");
  } else {
    spdlog::error("db provided snackcreditid is broken");
  }

  int timeout = vm.count("timeout") ? vm["timeout"].as<uint32_t>() : DEFAULT_WAITFORTAG_TIMEOUT;
  while (!tm.tagPresent() & (timeout >= 0)) {
    spdlog::info("waiting for tag");
    timeout--;
    sleep(1);
  }

  if (timeout < 0) {
    spdlog::warn("timeout tag read");
  } else {
    spdlog::info("tag present");

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
            // Die App wird nun gelöscht und neu angelegt
            // Zuerst wird der bekannte Schlüssel benutzt, wenn das
            // Fehlschlägt, wird der NULLKEY versucht
            if (tm.deleteSpacebiApp(*currentTag)) {
              spdlog::info("Delete OK");

            } else {
              spdlog::error("Delete failed");
              return;
            }

          } else {
            spdlog::info("no spacebi app found");
          }
          tm.createSpacebiApp(*currentTag);
          if (tm.loginSpacebiApp(*currentTag, 0, false)) {
            tm.createMetaFile(*currentTag, meta);
            tm.createPersonalInfoFile(*currentTag, personal);
            tm.createDoorFile(*currentTag, door);
            tm.createLDAPFile(*currentTag, ldap);
            for (uint8_t i = 1; i <= 4; i++) {
              tm.createRandomIDFile(*currentTag, i, rid[i]);
            }

            // Ummelden
            //tm.loginSpacebiApp(*currentTag, KEYNO_CREDIT_RW, false);

            tm.createCreditMetaFile(*currentTag, creditmeta);
            tm.createCreditFile(*currentTag, 0, DEFAULT_CREDIT_MIN_VALUE, DEFAULT_CREDIT_MAX_VALUE);
            tm.createTransactionFile(*currentTag);

            dump_metainfofile(meta);
            dump_personalinfofile(personal);
            dump_doorfile(door);
            dump_ldapuserfile(ldap);
            dump_uniquerandomfile(rid[0]);
            dump_uniquerandomfile(rid[1]);
            dump_uniquerandomfile(rid[2]);
            dump_uniquerandomfile(rid[3]);
            dump_creditmetafile(creditmeta);
          } else {
            spdlog::error("app create failed");
          }
          // Tag wieder loslassen
          tm.disconnectTag(*currentTag);
        } else {
          spdlog::error("connect failed");
        }
      } else {
        spdlog::error("tag not supported");
      }
    }
  }
}

int main(int argc, char *argv[]) {
  po::options_description desc("Alle Optionen");
  desc.add_options()("help", "hilfe")("verbose,v", po::value<int>()->implicit_value(1), "verbose")("keyfile,k", po::value<string>(), "pfad zur keyfile")("configfile,c", po::value<string>(), "pfad zur konfiguration")("timeout,t", po::value<int>(), "timeout auf tag warten")("memberid,m", po::value<int>(), "memberid");

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

  writeTransaction(tm, config, vm);

  //tm.~SpacebiNFCTagManager();

  return 0;
}