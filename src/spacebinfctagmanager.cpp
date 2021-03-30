

#include "include/spacebinfctagmanager.h"

#include <freefare.h>
#include <nfc/nfc.h>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
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

bool SpacebiNFCTagManager::prepareAppKey(int keyno, MifareDESFireKey *key){
  // ?!?!  Wird zum Glück wegoptimiert
  string lookupalgo = DEF2STR(SPACEBIAPPID);
  lookupalgo += ".algo";

  string lookupkey = DEF2STR(SPACEBIAPPID);
  lookupkey += ".key";
  lookupkey += keyno;

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

  if (!hexstr_to_desfirekey(algo.get(), key_hexstr.get(), key)) {
    cout << "Cannot build desfirekey" << endl;
    return false;
  }
}

bool SpacebiNFCTagManager::loginSpacebiApp(FreefareTag tag, int keyno){

  MifareDESFireKey key;

  if(!prepareAppKey(keyno, &key)){
    cout << "prepare key failed" << endl;
    return false;
  }

  // Anmelden
  if (mifare_desfire_authenticate(tag, keyno, key) < 0) {
    cout << "login on app failed" << endl;
    return false;
  }


  mifare_desfire_key_free(key);
}

bool SpacebiNFCTagManager::selectSpacebiApp(FreefareTag tag) {
  MifareDESFireAID aid = mifare_desfire_aid_new(SPACEBIAPPID);
  if (mifare_desfire_select_application(tag, aid) < 0) {
    cout << "Select spacebi app failed" << endl;
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::createSpacebiApp(FreefareTag tag) {
  MifareDESFireAID aid = mifare_desfire_aid_new(SPACEBIAPPID);

  /* Mifare DESFire Application settings
    * bit 7 - 4: Number of key needed to change application keys (key 0 - 13; 0 = master key; 14 = key itself required for key change; 15 = all keys are frozen)
    * bit 3: Application configuration frozen = 0; Application configuration changeable when authenticated with application master key = 1
    * bit 2: Application master key authentication required for create/delete files = 0; Authentication not required = 1
    * bit 1: GetFileIDs, GetFileSettings and GetKeySettings behavior: Master key authentication required = 0; No authentication required = 1
    * bit 0 = Application master key frozen = 0; Application master key changeable = 1
    */

    uint8_t app_settings = MDAPP_SETTINGS(0, 1, 0, 1, 1);
    if (mifare_desfire_create_application_3k3des(tag, aid, app_settings, 2) < 0) {
      cout << "Create wasn't successful" << endl;
      return false;
    }

    // App auswählen
    mifare_desfire_select_application(tag, aid);
    // Anmelden
    if (mifare_desfire_authenticate(tag, 0, null_key) < 0) {
      cout << "Auth after create wasn't successful" << endl;
      return false;
    }
}



bool SpacebiNFCTagManager::readDoorFile(FreefareTag tag, spacebi_card_doorfile_t *doorfile) {

  if(!loginSpacebiApp(tag, KEYNO_DOORREADER)){
    return false;
  }


  if (mifare_desfire_read_data(tag, FILENO_DOORINFO, 0, sizeof(spacebi_card_doorfile_t), doorfile) < 0) {
    cout << "read from app failed" << endl;
    return false;
  }

  

  return true;
}