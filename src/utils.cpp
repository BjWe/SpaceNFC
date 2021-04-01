#include "include/utils.h"

#include <inttypes.h>

#include <iostream>
#include <regex>

using namespace std;

string hex_str(unsigned char *data, int len) {
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