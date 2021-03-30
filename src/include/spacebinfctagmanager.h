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

  bool prepareAppKey(int keyno, MifareDESFireKey *key);
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

  bool readDoorFile(FreefareTag tag, spacebi_card_doorfile_t *doorfile);
};

#endif