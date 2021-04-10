

#include "include/spacebinfctagmanager.h"

#include <freefare.h>
#include <nfc/nfc.h>
#include <spdlog/spdlog.h>

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
  spdlog::trace("init spbi nfctagmanager");

  nfc_init(&context);
  if (context == NULL)
    spdlog::error("Unable to init libnfc (malloc)");

  device_count = nfc_list_devices(context, devices, 8);
  if (device_count <= 0)
    spdlog::error("No NFC device found");

  spdlog::trace("open devices");

  for (size_t d = 0; d < device_count; d++) {
    spdlog::debug("open nfc dev #{}", d);
    device = nfc_open(context, devices[d]);
    if (!device) {
      spdlog::error("nfc_open() failed.");
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
    spdlog::error("get tags error while tag reading");
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
    spdlog::error("error while tag reading");
    return false;
  }
}

bool SpacebiNFCTagManager::connectTag(FreefareTag tag) {
  // Verbindung zur Karte herstellen
  if (mifare_desfire_connect(tag) < 0) {
    spdlog::error("can't connect to Mifare DESFire target");
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::disconnectTag(FreefareTag tag) {
  // Verbindung von der Karte trennen
  if (mifare_desfire_disconnect(tag) < 0) {
    spdlog::error("can't disconnect from Mifare DESFire target");
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

bool SpacebiNFCTagManager::prepareAppKey(int keyno, uint8_t keyversion, MifareDESFireKey *key) {
  spdlog::trace("preparing key #{}", keyno);
  // ?!?!  Wird zum Glück wegoptimiert

  string lookupalgo = DEF2STR(SPACEBIAPPID);
  lookupalgo += ".algo";

  boost::optional<string> algo = keys.get_optional<string>(lookupalgo);
  if (!algo) {
    spdlog::error("algorithm for app not found in keyfile");
    return false;
  }

  string lookupkey;
  boost::optional<string> key_hexstr;

  if (keyno >= 0) {
    lookupkey = DEF2STR(SPACEBIAPPID);
    lookupkey += ".key";
    lookupkey += to_string(keyno);
    lookupkey += '-';
    lookupkey += hex_str(&keyversion, 1);

    key_hexstr = keys.get_optional<string>(lookupkey);
    if (!key_hexstr) {
      spdlog::error("key '{}' not found", lookupkey);
      return false;
    }
  }

  if (keyno < 0) {
    if (!null_desfirekey(algo.get(), key)) {
      spdlog::error("cannot build null desfirekey");
      return false;
    }
  } else {
    if (!hexstr_to_desfirekey(algo.get(), key_hexstr.get(), key)) {
      spdlog::error("cannot build desfirekey");
      return false;
    }
  }

  return true;
}

bool SpacebiNFCTagManager::changeAppKey(FreefareTag tag, int keyno, MifareDESFireKey *fromkey, MifareDESFireKey *tokey) {
  int ret = mifare_desfire_change_key(tag, keyno, *tokey, *fromkey);
  if (ret < 0) {
    spdlog::error("Keychange #{} after create wasn't successful ({})", keyno, ret);
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::loginSpacebiApp(FreefareTag tag, int keyno, bool usenullkey) {
  MifareDESFireKey key;

  if (usenullkey) {
    if (!prepareAppKey(-1, 0, &key)) {
      spdlog::error("prepare key failed");
      return false;
    }
  } else {
    if (!prepareAppKey(keyno, 0, &key)) {
      spdlog::error("prepare key failed");
      return false;
    }
  }

  // Anmelden
  if (mifare_desfire_authenticate(tag, keyno, key) < 0) {
    spdlog::error("login on app (keyno: {})failed", keyno);
    return false;
  }

  mifare_desfire_key_free(key);

  return true;
}

bool SpacebiNFCTagManager::selectSpacebiApp(FreefareTag tag) {
  MifareDESFireAID aid = mifare_desfire_aid_new(SPACEBIAPPID);
  if (mifare_desfire_select_application(tag, aid) < 0) {
    spdlog::error("Select spacebi app failed");
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
  int ret = mifare_desfire_create_application_3k3des(tag, aid, app_settings, SPACEBIAPPKEYNUM);
  if (ret < 0) {
    spdlog::error("Create wasn't successful ({})", freefare_strerror(tag));
    return false;
  }

  // App auswählen
  mifare_desfire_select_application(tag, aid);

  // Mit null key anmelden
  if (!loginSpacebiApp(tag, 0, true)) {
    spdlog::error("login with null key after create failed");
    return false;
  } else {
    spdlog::info("login with null key after create ok");
  }

  MifareDESFireKey nullkey;
  prepareAppKey(-1, 0, &nullkey);

  // Application Keys erzeugen
  MifareDESFireKey appkeys[SPACEBIAPPKEYNUM];
  for (uint8_t i = 0; i < SPACEBIAPPKEYNUM; i++) {
    prepareAppKey(i, 0, &appkeys[i]);
  }

  // Rückwärtsgang
  // Wenn Key 0 geändert wird müssen wir uns neu anmelden
  //
  // Für den Fall, dass du hier zum debuggen vorbei schaust, weil sich
  // die Schlüssel > 0 nicht ändern lassen: Schmeiß das China Token
  // weg. Es ist kaputt...
  for (uint8_t i = SPACEBIAPPKEYNUM; i > 0; i--) {
    changeAppKey(tag, i - 1, &nullkey, &appkeys[i - 1]);
  }
  //changeAppKey(tag, 0, &nullkey, &appkeys[0]);

  // Speicher freigeben
  for (uint8_t i = 0; i < SPACEBIAPPKEYNUM; i++) {
    mifare_desfire_key_free(appkeys[i]);
  }

  mifare_desfire_key_free(nullkey);

  return true;
}

bool SpacebiNFCTagManager::deleteSpacebiApp(FreefareTag tag) {
  spdlog::trace("going to delete spacebiapp");
  MifareDESFireAID aid = mifare_desfire_aid_new(SPACEBIAPPID);

  if (!selectSpacebiApp(tag)) {
    spdlog::error("app select failed");
    return false;
  }

  if (!loginSpacebiApp(tag, 0, false)) {
    if (!loginSpacebiApp(tag, 0, true)) {
      // Last-Resort: Versuchen wir den Login auf der PICC anwendung
      mifare_desfire_select_application(tag, mifare_desfire_aid_new(0));
      if (!loginSpacebiApp(tag, 0, true)) {
        spdlog::error("weder richtiger noch null-key hat funktioniert");
        return false;
      }
    }
  }

  if (mifare_desfire_delete_application(tag, aid) < 0) {
    spdlog::error("delete failed");
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::readMetaFile(FreefareTag tag, spacebi_card_metainfofile_t *metafile) {
  spdlog::trace("going to read from metafile");
  if (!loginSpacebiApp(tag, KEYNO_APPMASTER, false)) {
    return false;
  }

  if (mifare_desfire_read_data(tag, FILENO_METADATA, 0, sizeof(spacebi_card_metainfofile_t), metafile) < 0) {
    spdlog::error("read from app failed");
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::createMetaFile(FreefareTag tag, spacebi_card_metainfofile_t metafile) {
  spdlog::trace("going to create metafile");
  //                     READ. WRITE, READWRITE, CHANGEACCESS
  uint16_t access = MDAR(KEYNO_APPMASTER, KEYNO_APPMASTER, KEYNO_APPMASTER, KEYNO_APPMASTER);

  if (mifare_desfire_create_std_data_file(tag, FILENO_METADATA, MDCM_ENCIPHERED, access, sizeof(spacebi_card_metainfofile_t)) < 0) {
    spdlog::error("create metafile failed");
    return false;
  }

  if (mifare_desfire_write_data(tag, FILENO_METADATA, 0, sizeof(spacebi_card_metainfofile_t), &metafile) < 0) {
    spdlog::error("write metafile failed");
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::readDoorFile(FreefareTag tag, spacebi_card_doorfile_t *doorfile) {
  spdlog::trace("going to read from doorfile");
  if (!loginSpacebiApp(tag, KEYNO_DOORREADER, false)) {
    return false;
  }

  if (mifare_desfire_read_data(tag, FILENO_DOORINFO, 0, sizeof(spacebi_card_doorfile_t), doorfile) < 0) {
    spdlog::error("read from app failed");
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::createDoorFile(FreefareTag tag, spacebi_card_doorfile_t doorfile) {
  spdlog::trace("going to create doorfile");
  //                     READ. WRITE, READWRITE, CHANGEACCESS
  uint16_t access = MDAR(KEYNO_DOORREADER, KEYNO_APPMASTER, KEYNO_APPMASTER, KEYNO_APPMASTER);

  if (mifare_desfire_create_std_data_file(tag, FILENO_DOORINFO, MDCM_ENCIPHERED, access, sizeof(spacebi_card_doorfile_t)) < 0) {
    spdlog::error("create doorfile failed");
    return false;
  }

  if (mifare_desfire_write_data(tag, FILENO_DOORINFO, 0, sizeof(spacebi_card_doorfile_t), &doorfile) < 0) {
    spdlog::error("write doorfile failed");
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::readLDAPFile(FreefareTag tag, spacebi_card_ldapuserfile_t *ldapfile) {
  spdlog::trace("going to read from ldapfile");
  if (!loginSpacebiApp(tag, KEYNO_LDAPINFO, false)) {
    return false;
  }

  if (mifare_desfire_read_data(tag, FILENO_LDAPINFO, 0, sizeof(spacebi_card_ldapuserfile_t), ldapfile) < 0) {
    spdlog::error("read ldapfile failed");
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::createLDAPFile(FreefareTag tag, spacebi_card_ldapuserfile_t ldapfile) {
  spdlog::trace("going to create ldapfile");
  //                     READ. WRITE, READWRITE, CHANGEACCESS
  uint16_t access = MDAR(KEYNO_LDAPINFO, KEYNO_APPMASTER, KEYNO_APPMASTER, KEYNO_APPMASTER);

  if (mifare_desfire_create_std_data_file(tag, FILENO_LDAPINFO, MDCM_ENCIPHERED, access, sizeof(spacebi_card_ldapuserfile_t)) < 0) {
    spdlog::error("create ldapfile failed");
    return false;
  }

  if (mifare_desfire_write_data(tag, FILENO_LDAPINFO, 0, sizeof(spacebi_card_ldapuserfile_t), &ldapfile) < 0) {
    spdlog::error("write ldapfile failed");
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::readRandomIDFile(FreefareTag tag, int id, spacebi_card_unique_randomfile_t *randomfile) {
  spdlog::trace("going to read from randomid file #{}", id);
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
    spdlog::error("read from app failed");
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::createRandomIDFile(FreefareTag tag, int id, spacebi_card_unique_randomfile_t randomfile) {
  spdlog::trace("going to create randomid file #{}", id);
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
    spdlog::error("create randomfile ID '{}' F'{}' failed", id, fileno);
    return false;
  }

  if (mifare_desfire_write_data(tag, fileno, 0, sizeof(spacebi_card_unique_randomfile_t), &randomfile) < 0) {
    spdlog::error("write randomfile ID '{}' F'{}' failed", id, fileno);
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::readCreditMetaFile(FreefareTag tag, spacebi_card_creditmetafile_t *creditmetafile) {
  spdlog::trace("going to read from creditmetafile");
  if (!loginSpacebiApp(tag, KEYNO_CREDIT_RD, false)) {
    return false;
  }

  if (mifare_desfire_read_data(tag, FILENO_CREDITMETA, 0, sizeof(spacebi_card_creditmetafile_t), creditmetafile) < 0) {
    spdlog::error("read creditmetafile failed");
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::createCreditMetaFile(FreefareTag tag, spacebi_card_creditmetafile_t creditmetafile) {
  spdlog::trace("going to create creditmetafile");
  //                    
  uint16_t access = FACCESS_CREDITMETA;

  if (mifare_desfire_create_std_data_file(tag, FILENO_CREDITMETA, MDCM_ENCIPHERED, access, sizeof(spacebi_card_creditmetafile_t)) < 0) {
    spdlog::error("create creditmetafile failed");
    return false;
  }

  if (mifare_desfire_write_data(tag, FILENO_LDAPINFO, 0, sizeof(spacebi_card_creditmetafile_t), &creditmetafile) < 0) {
    spdlog::error("write creditmetafile failed");
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::updateCreditMetaFile(FreefareTag tag, spacebi_card_creditmetafile_t creditmetafile) {
  spdlog::trace("going to update creditmetafile");
  if (!loginSpacebiApp(tag, KEYNO_CREDIT_WR, false)) {
    return false;
  }

  if (mifare_desfire_write_data(tag, FILENO_CREDITMETA, 0, sizeof(spacebi_card_creditmetafile_t), &creditmetafile) < 0) {
    spdlog::error("update creditmetafile failed");
    return false;
  }

  return true;
}


bool SpacebiNFCTagManager::readCreditFile(FreefareTag tag, int *credit) {
  spdlog::trace("read credit");
  if (!loginSpacebiApp(tag, KEYNO_CREDIT_RD, false)) {
    return false;
  }

  
  mifare_desfire_file_settings fs;
  mifare_desfire_get_file_settings(tag, FILENO_CREDITS, &fs);
  spdlog::info("limited credit value is: {}", fs.settings.value_file.limited_credit_value);
  spdlog::info("credit limits:({}/{})", fs.settings.value_file.lower_limit, fs.settings.value_file.upper_limit);
  

  if (mifare_desfire_get_value(tag, FILENO_CREDITS, credit) < 0) {
    spdlog::error("read credit failed");
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::createCreditFile(FreefareTag tag, int credit, int lower_limit, int upper_limit) {
  spdlog::trace("create credit: {} ({}/{})", credit, lower_limit, upper_limit);
  //                     READ. WRITE, READWRITE, CHANGEACCESS
  uint16_t access = MDAR(KEYNO_CREDIT_RD, KEYNO_CREDIT_WR, KEYNO_CREDIT_RW, KEYNO_APPMASTER);

  if (mifare_desfire_create_value_file(tag, FILENO_CREDITS, MDCM_ENCIPHERED, access, lower_limit, upper_limit, credit, 1) < 0) {
    spdlog::error("create credit failed");
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::creditFileIncrLimited(FreefareTag tag, int amount) {
  spdlog::debug("incr credit by {}", amount);

  if (!loginSpacebiApp(tag, KEYNO_CREDIT_WR, false)) {
    return false;
  }

  if (mifare_desfire_limited_credit(tag, FILENO_CREDITS, amount) < 0) {
    spdlog::error("incr credit failed");
    return false;
  }

  if (mifare_desfire_commit_transaction(tag) < 0) {
    spdlog::error("incr credit commit failed");
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::creditFileIncr(FreefareTag tag, int amount) {
  spdlog::debug("incr credit by {}", amount);

  if (!loginSpacebiApp(tag, KEYNO_CREDIT_RW, false)) {
    return false;
  }

  if (mifare_desfire_credit(tag, FILENO_CREDITS, amount) < 0) {
    spdlog::error("incr credit failed");
    return false;
  }

  if (mifare_desfire_commit_transaction(tag) < 0) {
    spdlog::error("incr credit commit failed");
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::creditFileDecr(FreefareTag tag, int amount) {
  spdlog::debug("decr credit by {}", amount);

  if (!loginSpacebiApp(tag, KEYNO_CREDIT_WR, false)) {
    return false;
  }

  if (mifare_desfire_debit(tag, FILENO_CREDITS, amount) < 0) {
    spdlog::error("decr credit failed");
    return false;
  }

  if (mifare_desfire_commit_transaction(tag) < 0) {
    spdlog::error("decr credit commit failed");
    return false;
  }

  return true;
}


bool SpacebiNFCTagManager::createTransactionFile(FreefareTag tag) {
  spdlog::trace("create transaction file");
  //                     READ. WRITE, READWRITE, CHANGEACCESS
  uint16_t access = MDAR(KEYNO_CREDIT_RD, KEYNO_CREDIT_WR, KEYNO_CREDIT_RW, KEYNO_APPMASTER);

  if (mifare_desfire_create_cyclic_record_file(tag, FILENO_TRANSACTIONS, MDCM_ENCIPHERED, access, sizeof(spacebi_card_transaction_record_t), SPACEBITRANSACTIONHISTORY) < 0) {
    spdlog::error("create transactionfile failed ({})", freefare_strerror(tag));
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::readTransaction(FreefareTag tag, int offset, spacebi_card_transaction_record_t *record) {
  spdlog::trace("read transaction file");
  if (!loginSpacebiApp(tag, KEYNO_CREDIT_RW, false)) {
    return false;
  }
// FIXME
  int ret = mifare_desfire_read_records(tag, FILENO_TRANSACTIONS, 0, 1, record);
  if (ret < 0) {
    spdlog::error("read transactionfile failed ({})", freefare_strerror(tag));
    return false;
  }

  return true;
}

bool SpacebiNFCTagManager::storeTransaction(FreefareTag tag, int offset, spacebi_card_transaction_record_t record) {
  spdlog::trace("store transaction");
  if (!loginSpacebiApp(tag, KEYNO_CREDIT_WR, false)) {
    return false;
  }

  if (mifare_desfire_write_record(tag, FILENO_TRANSACTIONS, 0, sizeof(spacebi_card_transaction_record_t), &record) < 0) {
    spdlog::error("add to transactionfile failed");
    return false;
  }

  if (mifare_desfire_commit_transaction(tag) < 0) {
    spdlog::error("store transaction commit failed");
    return false;
  }

  return true;
}