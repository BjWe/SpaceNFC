#ifndef _SPACEBICARDSTRUCTS_H
#define _SPACEBICARDSTRUCTS_H

#include <iostream>
#include <stdint.h>
#include <inttypes.h>

#include "utils.h"

using namespace std;

typedef struct  __attribute__((packed)) {
  uint64_t issuedts;
  uint32_t memberid;
  uint32_t reserved;
} spacebi_card_metainfofile_t;

typedef struct  __attribute__((packed)) {
  uint8_t token[32];
} spacebi_card_doorfile_t;

typedef struct  __attribute__((packed)) {
  char username[64];
} spacebi_card_ldapuserfile_t;

typedef struct  __attribute__((packed)) {
  uint64_t randomid;
} spacebi_card_unique_randomfile_t;


// Convert

inline static string doorfile_to_hexstring(spacebi_card_doorfile_t df){
  return hex_str(&df.token[0], sizeof(df.token)); 
}

// Dump

inline static void dump_metainfofile(spacebi_card_metainfofile_t mf){
  cout << "=== METAFILE ===" << endl;
  cout << "Timestamp: " << mf.issuedts << endl;
  cout << "Memberid: " << mf.memberid << endl;
  cout << "================" << endl;
}

inline static void dump_doorfile(spacebi_card_doorfile_t df){
  cout << "=== DOORFILE ===" << endl;
  for(uint8_t i; i < 32; i++){
    printf("%02x", df.token[i]);
  }
  cout << endl << "================" << endl;
}

inline static void dump_ldapuserfile(spacebi_card_ldapuserfile_t lf){
  cout << "=== LDAPFILE ===" << endl;
  cout << "Username: " << lf.username << endl;
  cout << "================" << endl;
}

inline static void dump_uniquerandomfile(spacebi_card_unique_randomfile_t rf){
  cout << "=== RANDFILE ===" << endl;
  printf("%lx / %lu" , rf.randomid, rf.randomid);
  cout << endl << "================" << endl;
}

#endif