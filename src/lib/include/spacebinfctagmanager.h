#ifndef __SPACEBINFCTAGMANAGER_H__
#define __SPACEBINFCTAGMANAGER_H__

#include <freefare.h>
#include <nfc/nfc.h>

#include <boost/property_tree/ptree.hpp>
#include <iostream>

#include "spacebicardstructs.h"



#define SNTM_NOTOKEN        -1
#define SNTM_INVALIDTAGTYPE -2

#define SNTM_APP_OK          0
#define SNTM_APP_MISSING    -3


class SpacebiNFCTagManager {
 protected:
  boost::property_tree::ptree keys;

  nfc_device *device = NULL;
  nfc_connstring devices[8];
  size_t device_count;

  nfc_context *context;

  bool prepareAppKey(int keyno, uint8_t keyversion, MifareDESFireKey *key);
  bool changeAppKey(FreefareTag tag, int keyno, MifareDESFireKey *fromkey, MifareDESFireKey *tokey);

 public:
  SpacebiNFCTagManager(boost::property_tree::ptree *_keys);
  ~SpacebiNFCTagManager();
  void initNFC();
  bool tagPresent();
  enum freefare_tag_type getTagType(FreefareTag tag);
  bool isTagSupported(FreefareTag tag);
  bool getFirstPresentTag(FreefareTag **tag);
  bool connectTag(FreefareTag tag);
  bool disconnectTag(FreefareTag tag);
  int hasSpacebiApp(FreefareTag tag);
  bool loginSpacebiApp(FreefareTag tag, int keyno, bool usenullkey);
  bool selectSpacebiApp(FreefareTag tag);
  bool createSpacebiApp(FreefareTag tag);
  bool deleteSpacebiApp(FreefareTag tag);

  bool readMetaFile(FreefareTag tag, spacebi_card_metainfofile_t *metafile);
  bool createMetaFile(FreefareTag tag, spacebi_card_metainfofile_t metafile);
  
  bool readDoorFile(FreefareTag tag, spacebi_card_doorfile_t *doorfile);
  bool createDoorFile(FreefareTag tag, spacebi_card_doorfile_t doorfile);

  bool readLDAPFile(FreefareTag tag, spacebi_card_ldapuserfile_t *ldapfile);
  bool createLDAPFile(FreefareTag tag, spacebi_card_ldapuserfile_t ldapfile);
  
  bool readRandomIDFile(FreefareTag tag, int id, spacebi_card_unique_randomfile_t *randomfile);
  bool createRandomIDFile(FreefareTag tag, int id, spacebi_card_unique_randomfile_t randomfile);

  
  bool readCreditMetaFile(FreefareTag tag, spacebi_card_creditmetafile_t *creditmetafile);
  bool createCreditMetaFile(FreefareTag tag, spacebi_card_creditmetafile_t creditmetafile);
  bool updateCreditMetaFile(FreefareTag tag, spacebi_card_creditmetafile_t creditmetafile);
  bool readCreditFile(FreefareTag tag, int *credit);
  bool createCreditFile(FreefareTag tag, int credit, int lower_limit, int upper_limit);
  bool creditFileIncrLimited(FreefareTag tag, int amount);
  bool creditFileIncr(FreefareTag tag, int amount);
  bool creditFileDecr(FreefareTag tag, int amount);
  bool createTransactionFile(FreefareTag tag);
  bool readTransaction(FreefareTag tag, int offset, spacebi_card_transaction_record_t *record);
  bool storeTransaction(FreefareTag tag, int offset, spacebi_card_transaction_record_t record);
};

#endif