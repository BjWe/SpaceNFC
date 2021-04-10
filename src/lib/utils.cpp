#include "include/utils.h"

#include <spdlog/spdlog.h>

#include <inttypes.h>
#include <iostream>
#include <regex>

using namespace std;

string hex_str(uint8_t *data, int len) {
  string s(len * 2, ' ');
  for (int i = 0; i < len; ++i) {
    s[2 * i] = hexmap[(data[i] & 0xF0) >> 4];
    s[2 * i + 1] = hexmap[data[i] & 0x0F];
  }
  return s;
}

uint64_t hexstr_to_uint64(string hexstr) {
  // whitespaces rausschmeißen
  string plainhex = regex_replace(hexstr, regex("\\s"), "");

  uint64_t result;

  if (sscanf(plainhex.c_str(), "%" SCNx64, &result) != 1) {
    return 0;
  }

  return result;
}

bool hexstr_to_chararray(string hexstr, uint8_t *buff, size_t bufflen){
  // whitespaces rausschmeißen
  string plainhex = regex_replace(hexstr, regex("\\s"), "");

  // Zeichen prüfen
  if(plainhex.find_first_not_of("0123456789abcdefABCDEF") != string::npos){
    spdlog::error("hexkey contains additional characters");
    return false;
  }

  if(plainhex.length() != (bufflen * 2)){
    spdlog::error("hexstr has to be " + to_string(bufflen * 2) + " chars long");
    return false;
  }

  for (uint8_t i = 0; i < (bufflen * 2); i += 2) {
    string byteString = plainhex.substr(i, 2);
     *buff = (char) strtol(byteString.c_str(), NULL, 16);
     buff++;
  }
  
  return true;
}