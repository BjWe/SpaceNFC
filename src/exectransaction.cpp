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
#include "lib/include/utils.h"

using namespace std;

namespace po = boost::program_options;

void writeTransaction(SpacebiNFCTagManager tm, po::variables_map vm, SpaceRestApi rapi) {
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

              if (vm.count("cart")) {
                if (vm.count("price")) {
                  ptree data;
                  if (rapi.transmitSnackshopCart(financetoken, vm["cart"].as<vector<ProductAmountPair>>(), vm["price"].as<double>(), data)) {
                    jsonout.add("error", "");
                    jsonout.add("oldBalance", data.get<double>("oldBalance"));
                    jsonout.add("newBalance", data.get<double>("newBalance"));
                  } else {
                    jsonout.add("error", data.get<string>("reason"));
                    spdlog::error("req failed");
                  }

                } else {
                  jsonout.add("error", "noprice");
                  spdlog::error("no clientprice provided");
                }
              } else if (vm.count("redeem")) {
                ptree data;
                if (rapi.redeemSnackshopVoucher(financetoken, vm["redeem"].as<string>(), data)) {
                  jsonout.add("error", "");
                  jsonout.add("oldBalance", data.get<double>("oldBalance"));
                  jsonout.add("newBalance", data.get<double>("newBalance"));
                } else {
                  jsonout.add("error", data.get<string>("reason"));
                  spdlog::error("req failed");
                }
              } else {
                jsonout.add("error", "internalerror");
                spdlog::error("no option");
              }
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

void validate(boost::any &v, const std::vector<std::string> &values, ProductAmountPair *, int) {
  //vector<ProductAmountPair> result = vector<ProductAmountPair>();
  spdlog::trace("Validate Size: {}", values.size());
  for (size_t i = 0; i < values.size(); ++i) {
    spdlog::trace("Parse {}", values[i]);
    int pos = values[i].find(':');
    if (pos < 1) {
      throw "invalid cart";
    }
    string left = values[i].substr(0, pos);
    string right = values[i].substr(pos + 1, string::npos);
    spdlog::trace("Parse Cart Pos: {} / Left: {} / Right: {}", pos, left, right);

    unsigned int id = stoul(left, nullptr);
    unsigned int amount = stoul(right, nullptr);

    spdlog::trace("Parse '{}' as Id: {} / Amount: {}", values[i], id, amount);

    //result.push_back(ProductAmountPair(id, amount));
    v = ProductAmountPair(id, amount);
  }
  //v = result;
}

int main(int argc, char *argv[]) {
  // Um den Validator zu testen
  //spdlog::set_level(spdlog::level::trace);

  po::options_description desc("Alle Optionen");
  desc.add_options()("help", "hilfe")("verbose,v", po::value<int>()->implicit_value(1), "verbose")("keyfile,k", po::value<string>(), "pfad zur keyfile")("configfile,c", po::value<string>(), "pfad zur konfiguration")("cart", po::value<vector<ProductAmountPair>>()->multitoken(), "warenkorb positionen [id:anzahl, ]")("price", po::value<double>(), "preis der dem client anzeigt wird")("redeem", po::value<string>(), "redeem code");

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

  writeTransaction(tm, vm, rapi);

  //tm.~SpacebiNFCTagManager();

  return 0;
}