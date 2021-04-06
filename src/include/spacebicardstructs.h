#ifndef _SPACEBICARDSTRUCTS_H
#define _SPACEBICARDSTRUCTS_H

#include <iostream>
#include <stdint.h>
#include <inttypes.h>

#include "utils.h"

using namespace std;

typedef struct  __attribute__((packed, aligned(16))) {
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

typedef struct __attribute__((packed)) {
  uint32_t id;
  uint64_t datetime;
  uint32_t flags;
  uint32_t amount;
  uint32_t balance;
  uint64_t ean;
} spacebi_card_transaction_record_t;

// Convert

inline static string doorfile_to_hexstring(spacebi_card_doorfile_t df){
  return hex_str(&df.token[0], sizeof(df.token)); 
}

// Dump

inline static void dump_metainfofile(spacebi_card_metainfofile_t mf){
  cout << "=== METAFILE ===" << endl;
  cout << "Timestamp: " << to_string(mf.issuedts) << endl;
  cout << "Memberid: " << to_string(mf.memberid) << endl;
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
  printf("%" PRIx64 " / %" PRIu64, rf.randomid, rf.randomid);
  cout << endl << "================" << endl;
}

inline static void dump_transaction(spacebi_card_transaction_record_t tr){
  cout << "Transaction: #" << to_string(tr.id) << " - " << to_string(tr.datetime) << " - EAN: " << tr.ean << " - Amount: " << to_string(tr.amount) << " - Balance: " << to_string(tr.balance) << endl;
}

#endif