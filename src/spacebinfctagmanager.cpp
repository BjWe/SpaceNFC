

#include "include/spacebinfctagmanager.h"

#include <freefare.h>
#include <nfc/nfc.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>

#include "include/global.h"
#include "include/mifareutils.h"
#include "include/spacebicardstructs.h"

#define XDEF2STR(def) #def
#define DEF2STR(def) XDEF2STR(def)

using namespace std;

SpacebiNFCTagManager::SpacebiNFCTagManager(boost::property_tree::ptree *_keys) {
  keys = *_keys;
}

void SpacebiNFCTagManager::initNFC() {
  cout << "init" << endl;

  nfc_init(&context);
  if (context == NULL)
    cout << "Unable to init libnfc (malloc)\n";

  cout << "count" << endl;

  device_count = nfc_list_devices(context, devices, 8);
  if (device_count <= 0)
    cout << "No NFC device found.\n";

  cout << "open" << endl;

  for (size_t d = 0; d < device_count; d++) {
    cout << "open no" << d << endl;
    device = nfc_open(context, devices[d]);
    if (!device) {
      cout << "nfc_open() failed.";
      continue;
    }
  }
}

SpacebiNFCTagManager::~SpacebiNFCTagManager() {
  //nfc_close(device);
}

bool SpacebiNFCTagManager::tagPresent() {
  FreefareTag *tags = NULL;
  tags = freefare_get_tags(device);

  // Konnte die Struktur geladen werden?
  // (auch wahr, wenn keine Tags gefunden wurden)
  if (tags) {
    // Wurde mindestens ein Tag gefunden?
    if (!tags[0]) {
      return false;
    } else {
      return true;
    }
  } else {
    cout << "Error while tag reading\n";
    return false;
  }
}

enum freefare_tag_type SpacebiNFCTagManager::getTagType(FreefareTag tag) {
  return freefare_get_tag_type(tag);
}

bool SpacebiNFCTagManager::isTagSupported(FreefareTag tag) {
  return freefare_get_tag_type(tag) == MIFARE_DESFIRE;
}

bool SpacebiNFCTagManager::getFirstPresentTag(FreefareTag **tag) {
  FreefareTag *tags = NULL;
  tags = freefare_get_tags(device);

  // Konnte die Struktur geladen werden?
  // (auch wahr, wenn keine Tags gefunden wurden)
  if (tags) {
    // Wurde mindestens ein Tag gefunden?
    if (!tags[0]) {
      return false;
    } else {
      *tag = &tags[0];
      return true;
    }
  } else {
    cout << "Error while tag reading\n";
    return false;
  }
}

bool SpacebiNFCTagManager::connectTag(FreefareTag tag) {
  // Verbindung zur Karte herstellen
  if (mifare_desfire_connect(tag) < 0) {
    cout << "Can't connect to Mifare DESFire target.";
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::disconnectTag(FreefareTag tag) {
  // Verbindung von der Karte trennen
  if (mifare_desfire_disconnect(tag) < 0) {
    cout << "Can't disconnect from Mifare DESFire target.";
    return false;
  }

  return true;
}

int SpacebiNFCTagManager::hasSpacebiApp(FreefareTag tag) {
  enum freefare_tag_type tag_type = freefare_get_tag_type(tag);

  // Nur DESFIRE erlauben
  if (tag_type != MIFARE_DESFIRE) {
    return SNTM_INVALIDTAGTYPE;
  }

  if (desfire_has_application(&tag, SPACEBIAPPID) == 1) {
    return SNTM_APP_OK;
  } else {
    return SNTM_APP_MISSING;
  }
}

bool SpacebiNFCTagManager::readDoorFile(FreefareTag tag, spacebi_card_doorfile_t *doorfile) {


  // ?!?!  Wird zum Glück wegoptimiert
  string lookupalgo = DEF2STR(SPACEBIAPPID);
  lookupalgo += ".algo";

  string lookupkey = DEF2STR(SPACEBIAPPID);
  lookupkey += ".key";
  lookupkey += DEF2STR(KEYNO_DOORREADER);

  /* Dump all keys
  stringstream ss;
  boost::property_tree::json_parser::write_json(ss, keys);
  cout << ss.str() << std::endl;

  cout << "Lookup: '" << lookup << "'" << endl;
  */

  boost::optional<string> algo = keys.get_optional<string>(lookupalgo);
  if (!algo) {
    cout << "Algo not found" << endl;
    return false;
  }

  boost::optional<string> key_hexstr = keys.get_optional<string>(lookupkey);
  if (!key_hexstr) {
    cout << "Key not found" << endl;
    return false;
  }

  MifareDESFireKey key;
  hexstr_to_desfirekey(algo.get(), key_hexstr.get(), &key);
  

  return true;
}