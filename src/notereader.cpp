#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>

#include "lib/include/global.h"
#include "lib/include/nv10.h"
#include "lib/include/spacebinfctagmanager.h"
#include "lib/include/spacerestapi.h"

using namespace std;

using boost::property_tree::ptree;
namespace po = boost::program_options;

int main(int argc, char** argv) {
  po::options_description desc("Alle Optionen");
  desc.add_options()("help", "hilfe")("verbose,v", po::value<int>()->implicit_value(1), "verbose")("configfile,c", po::value<string>(), "pfad zur konfiguration")("token,t", po::value<string>(), "token")("payintype,p", po::value<string>(), "payintype (snackshop | membershipfee | donation)");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << desc << "\n";
    return 1;
  }

  std::vector<spdlog::sink_ptr> sinks;
  sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
  sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_st>("logs/notereader", 23, 59));
  auto combined_logger = std::make_shared<spdlog::logger>("notereader", begin(sinks), end(sinks));
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

  if (!vm.count("token")) {
    spdlog::error("Token missing");
    cout << desc << "\n";
    return 1;
  }

  if (!vm.count("payintype")) {
    spdlog::error("payintype missing");
    cout << desc << "\n";
    return 1;
  }

  /*
  string payintypestr = vm["payintype"].as<string>();
  if((payintype != "snackshop") || (payintype != "membershipfee") || (payintype != "donation")){
    spdlog::error("unknown payintype '{}'", payintype);
    cout << desc << "\n";
    return 1;
  }
  */
  string payintypestr = vm["payintype"].as<string>();
  payin_e payintype;
  if (payintypestr == "snackshop") {
    payintype = payin_e::snackshop;
  } else if (payintypestr == "membershipfee") {
    payintype = payin_e::membershipfee;
  } else if (payintypestr == "donation") {
    payintype = payin_e::donation;
  } else {
    spdlog::error("unknown payintype '{}'", payintype);
    cout << desc << "\n";
    return 1;
  }

  boost::property_tree::ptree jsonout;

  SpaceRestApi rapi(config.get<string>("spaceapi.endpoint"), config.get<string>("spaceapi.apikey"), config.get<string>("spaceapi.token"));

  NV10SIO nv10 = NV10SIO(config.get<string>("nv10.device"));
  if (nv10.openDevice()) {
    for (uint i = 1; i <= 16; i++) {
      string key = "nv10.channel" + (boost::format("%02u") % i).str() + "_enable";
      spdlog::trace("looking for config key '{}'", key);
      boost::optional<bool> channelenable = config.get_optional<bool>(key);
      if (channelenable.has_value()) {
        spdlog::trace("channel {} has value", i);
        if (channelenable.value()) {
          nv10.enableNote(i);
        } else {
          nv10.inhibitNote(i);
        }
      } else {
        nv10.inhibitNote(i);
      }
    }
  

  

  nv10.enableEscrow();
  nv10.enable();

  // Falls noch was hängt
  nv10.escrowReject();

  bool terminate = false;

  while (!terminate) {
    nv10.update();
    if (nv10.getValidChannel() > 0) {
      cout << "----NOTE PRESENT----" << endl;
      uint currentChannel = nv10.getValidChannel();
      string key = "nv10.channel" + (boost::format("%02u") % nv10.getValidChannel()).str() + "_value";
      spdlog::trace("looking for config key '{}'", key);
      boost::optional<int> channelvalue = config.get_optional<int>(key);
      if (!channelvalue.has_value()) {
        spdlog::error("channelvalue for channel {} not found", nv10.getValidChannel());
        jsonout.add("error", "internal_error");
        nv10.escrowReject();
      } else {
        ptree data;
        cout << "----DATAEXCH S1----" << endl;
        try {
          if (rapi.payInNoteS1(vm["token"].as<string>(), payin_e::snackshop, channelvalue.value(), data)) {
            auto code = data.get_optional<string>("code");
            if (code.has_value()) {
              spdlog::warn("note accept");
              // Channel leeren, um nach dem Accept auf die Bestätigung zu warten
              nv10.clearValidChannel();
              nv10.escrowAccept();

              uint tries = 10;
              while (tries > 0) {
                spdlog::trace("read try run. {} from {} left", tries, 10);
                tries--;
                sleep(1);
                nv10.update();
                if (nv10.fraudDetected() || nv10.strimmingDetected()) {
                  string handled = "";
                  if (nv10.strimmingDetected()) {
                    handled = "STRIMMING";
                    jsonout.add("error", "strimming");
                  } else {
                    handled = "FRAUD";
                    jsonout.add("error", "fraud");
                  }
                  spdlog::error("{} handled", handled);
                  
                  cout << "----DATAEXCH S2----" << endl;
                  try {
                    rapi.payInNoteS2(code.value(), handled, data);
                  } catch (...) {
                    spdlog::error("transaction finalize ({}) failed (request not trow exception)", handled);
                  }
                  nv10.clearFraud();
                  nv10.clearStrimming();

                  tries = 0;
                } else if (nv10.getValidChannel() > 0) {
                  // Hier muss der Kanal derselbe sein, wie vor dem "Accept"
                  if (currentChannel == nv10.getValidChannel()) {
                    jsonout.add("error", "");
                    try {
                      rapi.payInNoteS2(code.value(), "OK", data);
                      jsonout.add("oldbalance", data.get<double>("oldBalance"));
                      jsonout.add("newbalance", data.get<double>("newBalance")); 
                      jsonout.add("notevalue", channelvalue.value());
                    } catch (...) {
                      spdlog::error("transaction finalize (OK) failed (request not throw exception)");
                    }
                  } else {
                    jsonout.add("error", "note_check_missmatch");
                    try {
                      rapi.payInNoteS2(code.value(), "ERROR", data);
                    } catch (...) {
                      spdlog::error("transaction finalize (ERROR) failed (request not throw exception)");
                    }
                  }
                  spdlog::trace("done. clearing");
                  nv10.clearValidChannel();
                  tries = 0;
                } else if(tries == 1){
                  spdlog::error("transaction finalize (TIMEOUT) failed (reader timeout)");
                  jsonout.add("error", "timeout");
                  try {
                      rapi.payInNoteS2(code.value(), "TIMEOUT", data);
                      
                    } catch (...) {
                      spdlog::error("transaction finalize (TIMEOUT) failed (request not throw exception)");
                    }
                }
              }

            } else {
              spdlog::warn("note rejected (code missing)");
              jsonout.add("error", "internal_error");
              nv10.escrowReject();
            }

          } else {
            spdlog::warn("note rejected (request not successful)");
            jsonout.add("error", "internal_error");
            nv10.escrowReject();
          }
        } catch (...) {
          spdlog::warn("note rejected (request not throw exception)");
          jsonout.add("error", "internal_error");
          nv10.escrowReject();
        }
      }

      //nv10.update();
      //sleep(1);
      nv10.disable();
      terminate = true;
    }
    sleep(1);
  }
  } else {
    jsonout.add("error", "device_error");
  }

  stringstream ss;
  boost::property_tree::json_parser::write_json(ss, jsonout, vm.count("verbose"));
  cout << "----BEGIN JSON----" << endl
       << ss.str() << "----END JSON----" << endl;

}
