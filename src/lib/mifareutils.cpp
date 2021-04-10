#include <iostream>
#include <freefare.h>
#include <nfc/nfc.h>
#include <unistd.h>
#include <regex>

#include <spdlog/spdlog.h>

using namespace std;

#define ALGO_3K3DES_SIZE 24

bool null_desfirekey(string algo, MifareDESFireKey *desfirekey){
  if(algo == "3K3DES"){
    uint8_t buff[ALGO_3K3DES_SIZE]; 
    memset(buff, 0, sizeof(buff));
    *desfirekey = mifare_desfire_3k3des_key_new(buff);
    //mifare_desfire_key_set_version(desfirekey, 0);
  } else {
    spdlog::error("algorythm '" + algo + "' not supported");
    return false;
  }
  return true;
}

bool hexstr_to_desfirekey(string algo, string hexstr, MifareDESFireKey *desfirekey){
  

  // whitespaces rausschmeißen
  string plainhex = regex_replace(hexstr, regex("\\s"), "");

  // Zeichen prüfen
  if(plainhex.find_first_not_of("0123456789abcdefABCDEF") != string::npos){
    spdlog::error("hexkey contains additional characters");
    return false;
  }

  if(algo == "3K3DES"){
    // Länge prüfen
    if(plainhex.length() != (ALGO_3K3DES_SIZE * 2)){
      spdlog::error("Key has to be " + to_string(ALGO_3K3DES_SIZE) + " bytes long");
      return false;
    }

    // Bytearray für mifare_desfire_3k3des_key_new_with_version zusammen basteln
    uint8_t buff[ALGO_3K3DES_SIZE]; 
    for (uint8_t i = 0; i < (sizeof(buff) * 2); i += 2) {
      string byteString = plainhex.substr(i, 2);
      buff[i/2] = (char) strtol(byteString.c_str(), NULL, 16);
    }

    *desfirekey = mifare_desfire_3k3des_key_new(buff);

    return true;
  } else {
    spdlog::error("algorythm '" + algo + "' not supported");
    return false;
  }
}

string get_tag_str(enum freefare_tag_type tag_type) {
  switch (tag_type) {
    case MIFARE_MINI: {
      return "Mifare Mini";
    }
    case MIFARE_CLASSIC_1K: {
      return "Mifare Classic 1K";
    }
    case MIFARE_CLASSIC_4K: {
      return "Mifare Classic 4K";
    }
    case MIFARE_DESFIRE: {
      return "Mifare Desfire";
    }
    case MIFARE_ULTRALIGHT: {
      return "Mifare Ultralight";
    }
    case NTAG_21x: {
      return "NTAG 21x";
    }
    default: {
      return "Unknown";
    }
  }
}

// Sucht auf dem Tag eine ApplicationID
// Return: Error = -1; Nicht gefunden: 0; Gefunden: 1
int desfire_has_application(FreefareTag *tag, uint32_t lookup_aid) {
  MifareDESFireAID *aids = NULL;
  size_t aid_count;
  if (mifare_desfire_get_application_ids(*tag, &aids, &aid_count) < 0) {
    spdlog::error("Can't read DESFIRE applications.");
    return -1;
  }

  if (!aids) {
    spdlog::error("Can't interpret DESFIRE applications.");
    return -1;
  }

  spdlog::info("found {} applicaton/s", aid_count);

  bool found = false;

  for (int i = 0; aids[i]; i++) {
    uint32_t int_aid = mifare_desfire_aid_get_aid(aids[i]);
    spdlog::debug("check {} == {}", int_aid, lookup_aid);
     if(int_aid == lookup_aid){
         found = true;
         break;
     }  
  }

  mifare_desfire_free_application_ids(aids);

  return found ? 1 : 0;
}