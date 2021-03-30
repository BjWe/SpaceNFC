

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

bool SpacebiNFCTagManager::prepareAppKey(int keyno, MifareDESFireKey *key) {
  cout << "preparing key " << keyno << endl;
  // ?!?!  Wird zum Glück wegoptimiert

  string lookupalgo = DEF2STR(SPACEBIAPPID);
  lookupalgo += ".algo";

  boost::optional<string> algo = keys.get_optional<string>(lookupalgo);
  if (!algo) {
    cout << "Algo not found" << endl;
    return false;
  }

  string lookupkey;
  boost::optional<string> key_hexstr;

  if (keyno >= 0) {
    lookupkey = DEF2STR(SPACEBIAPPID);
    lookupkey += ".key";
    lookupkey += to_string(keyno);

    key_hexstr = keys.get_optional<string>(lookupkey);
    if (!key_hexstr) {
      cout << "Key '" << lookupkey << "'not found" << endl;
      return false;
    }
  }

  if (keyno < 0) {
    if (!null_desfirekey(algo.get(), key)) {
      cout << "Cannot build null desfirekey" << endl;
      return false;
    }
  } else {
    if (!hexstr_to_desfirekey(algo.get(), key_hexstr.get(), key)) {
      cout << "Cannot build desfirekey" << endl;
      return false;
    }
  }

  return true;
}

bool SpacebiNFCTagManager::changeAppKey(FreefareTag tag, int keyno, MifareDESFireKey *fromkey, MifareDESFireKey *tokey) {
  if (mifare_desfire_change_key(tag, keyno, *tokey, *fromkey) < 0) {
    cout << "Keychange " << keyno << " after create wasn't successful" << endl;
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::loginSpacebiApp(FreefareTag tag, int keyno, bool usenullkey) {
  MifareDESFireKey key;

  if (usenullkey) {
    if (!prepareAppKey(-1, &key)) {
      cout << "prepare key failed" << endl;
      return false;
    }
  } else {
    if (!prepareAppKey(keyno, &key)) {
      cout << "prepare key failed" << endl;
      return false;
    }
  }

  // Anmelden
  if (mifare_desfire_authenticate(tag, keyno, key) < 0) {
    cout << "login on app failed" << endl;
    return false;
  }

  mifare_desfire_key_free(key);

  return true;
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
  if (mifare_desfire_create_application_3k3des(tag, aid, app_settings, SPACEBIAPPKEYNUM) < 0) {
    cout << "Create wasn't successful" << endl;
    return false;
  }

  // App auswählen
  mifare_desfire_select_application(tag, aid);

  // Mit null key anmelden
  loginSpacebiApp(tag, 0, true);

  MifareDESFireKey nullkey;
  prepareAppKey(-1, &nullkey);

  // Application Keys erzeugen  
  MifareDESFireKey appkeys[SPACEBIAPPKEYNUM];
  for (uint8_t i = 0; i < SPACEBIAPPKEYNUM; i++) {
    prepareAppKey(i, &appkeys[i]);
  }

  // Rückwärtsgang
  // Wenn Key 0 geändert wird müssen wir uns neu anmelden
  for (uint8_t i = SPACEBIAPPKEYNUM; i > 0; i--) {
    changeAppKey(tag, i - 1, &nullkey, &appkeys[i - 1]);
  }

  // Speicher freigeben
  for (uint8_t i = 0; i < SPACEBIAPPKEYNUM; i++) {
    mifare_desfire_key_free(appkeys[i]);
  }

  mifare_desfire_key_free(nullkey);

  return true;
}

bool SpacebiNFCTagManager::deleteSpacebiApp(FreefareTag tag) {
  MifareDESFireAID aid = mifare_desfire_aid_new(SPACEBIAPPID);

  if (!selectSpacebiApp(tag)) {
    cout << "app select ist fehlgeschlagen" << endl;
    return false;
  }

  if (!loginSpacebiApp(tag, 0, false)) {
    if (!loginSpacebiApp(tag, 0, true)) {
      // Last-Resort: Versuchen wir den Login auf der PICC anwendung
      mifare_desfire_select_application(tag, mifare_desfire_aid_new(0));
      if (!loginSpacebiApp(tag, 0, true)) {
        cout << "weder richtiger noch null-key hat funktioniert" << endl;
        return false;
      }
    }
  }

  if (mifare_desfire_delete_application(tag, aid) < 0) {
    cout << "löschen ist fehlgeschlagen" << endl;
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::readMetaFile(FreefareTag tag, spacebi_card_metainfofile_t *metafile) {
  if (!loginSpacebiApp(tag, KEYNO_APPMASTER, false)) {
    return false;
  }

  if (mifare_desfire_read_data(tag, FILENO_METADATA, 0, sizeof(spacebi_card_metainfofile_t), metafile) < 0) {
    cout << "read from app failed" << endl;
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::createMetaFile(FreefareTag tag, spacebi_card_metainfofile_t metafile) {
  //                     READ. WRITE, READWRITE, CHANGEACCESS
  uint16_t access = MDAR(KEYNO_APPMASTER, KEYNO_APPMASTER, KEYNO_APPMASTER, KEYNO_APPMASTER);

  if (mifare_desfire_create_std_data_file(tag, FILENO_METADATA, MDCM_ENCIPHERED, access, sizeof(spacebi_card_metainfofile_t)) < 0) {
    cout << "create doorfile failed" << endl;
    return false;
  }

  if (mifare_desfire_write_data(tag, FILENO_METADATA, 0, sizeof(spacebi_card_metainfofile_t), &metafile) < 0) {
    cout << "write doorfile failed" << endl;
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::readDoorFile(FreefareTag tag, spacebi_card_doorfile_t *doorfile) {
  if (!loginSpacebiApp(tag, KEYNO_DOORREADER, false)) {
    return false;
  }

  if (mifare_desfire_read_data(tag, FILENO_DOORINFO, 0, sizeof(spacebi_card_doorfile_t), doorfile) < 0) {
    cout << "read from app failed" << endl;
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::createDoorFile(FreefareTag tag, spacebi_card_doorfile_t doorfile) {
  //                     READ. WRITE, READWRITE, CHANGEACCESS
  uint16_t access = MDAR(KEYNO_DOORREADER, KEYNO_APPMASTER, KEYNO_APPMASTER, KEYNO_APPMASTER);

  if (mifare_desfire_create_std_data_file(tag, FILENO_DOORINFO, MDCM_ENCIPHERED, access, sizeof(spacebi_card_doorfile_t)) < 0) {
    cout << "create doorfile failed" << endl;
    return false;
  }

  if (mifare_desfire_write_data(tag, FILENO_DOORINFO, 0, sizeof(spacebi_card_doorfile_t), &doorfile) < 0) {
    cout << "write doorfile failed" << endl;
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::readLDAPFile(FreefareTag tag, spacebi_card_ldapuserfile_t *ldapfile) {
  if (!loginSpacebiApp(tag, KEYNO_LDAPINFO, false)) {
    return false;
  }

  if (mifare_desfire_read_data(tag, FILENO_LDAPINFO, 0, sizeof(spacebi_card_ldapuserfile_t), ldapfile) < 0) {
    cout << "read from app failed" << endl;
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::createLDAPFile(FreefareTag tag, spacebi_card_ldapuserfile_t ldapfile) {
  //                     READ. WRITE, READWRITE, CHANGEACCESS
  uint16_t access = MDAR(KEYNO_LDAPINFO, KEYNO_APPMASTER, KEYNO_APPMASTER, KEYNO_APPMASTER);

  if (mifare_desfire_create_std_data_file(tag, FILENO_LDAPINFO, MDCM_ENCIPHERED, access, sizeof(spacebi_card_ldapuserfile_t)) < 0) {
    cout << "create ldapfile failed" << endl;
    return false;
  }

  if (mifare_desfire_write_data(tag, FILENO_LDAPINFO, 0, sizeof(spacebi_card_doorfile_t), &ldapfile) < 0) {
    cout << "write ldapfile failed" << endl;
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::readRandomIDFile(FreefareTag tag, int id, spacebi_card_unique_randomfile_t *randomfile) {
  uint8_t keyno;
  uint8_t fileno;
  switch (id) {
    case 1: {
      keyno = KEYNO_RANDOMUID1;
      fileno = FILENO_RANDOMUID1;
      break;
    }
    case 2: {
      keyno = KEYNO_RANDOMUID2;
      fileno = FILENO_RANDOMUID2;
      break;
    }
    case 3: {
      keyno = KEYNO_RANDOMUID3;
      fileno = FILENO_RANDOMUID3;
      break;
    }
    case 4: {
      keyno = KEYNO_RANDOMUID4;
      fileno = FILENO_RANDOMUID4;
      break;
    }
  }

  if (!loginSpacebiApp(tag, keyno, false)) {
    return false;
  }

  if (mifare_desfire_read_data(tag, fileno, 0, sizeof(spacebi_card_unique_randomfile_t), randomfile) < 0) {
    cout << "read from app failed" << endl;
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::createRandomIDFile(FreefareTag tag, int id, spacebi_card_unique_randomfile_t randomfile) {
  uint8_t keyno;
  uint8_t fileno;
  switch (id) {
    case 1: {
      keyno = KEYNO_RANDOMUID1;
      fileno = FILENO_RANDOMUID1;
      break;
    }
    case 2: {
      keyno = KEYNO_RANDOMUID2;
      fileno = FILENO_RANDOMUID2;
      break;
    }
    case 3: {
      keyno = KEYNO_RANDOMUID3;
      fileno = FILENO_RANDOMUID3;
      break;
    }
    case 4: {
      keyno = KEYNO_RANDOMUID4;
      fileno = FILENO_RANDOMUID4;
      break;
    }

  }
  
  //                     READ. WRITE, READWRITE, CHANGEACCESS
  uint16_t access = MDAR(keyno, KEYNO_APPMASTER, KEYNO_APPMASTER, KEYNO_APPMASTER);

  if (mifare_desfire_create_std_data_file(tag, fileno, MDCM_ENCIPHERED, access, sizeof(spacebi_card_unique_randomfile_t)) < 0) {
    cout << "create randomfile ID '" << id << "' F'" << fileno << "' failed" << endl;
    return false;
  }

  if (mifare_desfire_write_data(tag, fileno, 0, sizeof(spacebi_card_unique_randomfile_t), &randomfile) < 0) {
    cout << "write randomfile ID '" << id << "' F'" << fileno << "' failed" << endl;
    return false;
  }

  return true;
}