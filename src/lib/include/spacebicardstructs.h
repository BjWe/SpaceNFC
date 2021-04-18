#ifndef _SPACEBICARDSTRUCTS_H
#define _SPACEBICARDSTRUCTS_H

#include <iostream>
#include <stdint.h>
#include <inttypes.h>

#include <boost/algorithm/string.hpp>


#include "utils.h"
#include "global.h"

using namespace std;

enum transaction_usage_e : uint8_t
{
  undefined = 0,
  deposit = 1,
  payoff  = 2,
  refund  = 3,
  redeemvoucher = 4,
  buysnack = 11
};

typedef struct  __attribute__((packed)) {
  uint16_t structureversion;
  uint16_t reserved1;
  uint16_t reserved2;
  uint16_t reserved3;
} spacebi_card_metainfofile_t;

typedef struct  __attribute__((packed)) {
  uint32_t memberid;
  uint64_t issuedts;
  uint32_t reserved1;
  uint32_t reserved2;
  uint32_t reserved3;
  uint32_t reserved4;
  uint32_t reserved5;
} spacebi_card_personalinfofile_t;

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
  uint8_t token[32];
  uint64_t lasttransaction;
  uint64_t reserved1;
  uint64_t reserved2;
} spacebi_card_creditmetafile_t;

typedef struct __attribute__((packed)) {
  uint64_t id;
  uint64_t timestamp;
  uint32_t flags;
  uint32_t amount;
  uint32_t balance;
  uint8_t usage;
  uint8_t reserved1;
  uint16_t reserved2;
} spacebi_card_transaction_record_t;

// Convert

inline static string doorfile_to_hexstring(spacebi_card_doorfile_t df){
  return hex_str(&df.token[0], sizeof(df.token)); 
}

inline static string randomuserid_to_hexstring(spacebi_card_unique_randomfile_t rf){
  return hex_str((uint8_t *)&rf.randomid, sizeof(rf.randomid)); 
}

// Helper

inline static transaction_usage_e str_to_transaction_usage(string usage){
  if (boost::iequals(usage, "deposit")){
    return deposit;
  } else if (boost::iequals(usage, "payoff")){
    return payoff;
  } else if (boost::iequals(usage, "refund")){
    return refund;
  } else if (boost::iequals(usage, "redeemvoucher")){
    return redeemvoucher;
  } else if (boost::iequals(usage, "buysnack")){
    return buysnack;
  }  else {
    return undefined;
  }
}

inline static string transaction_usage_to_str(transaction_usage_e tu){
  switch(tu){
    case undefined: return "undefined";
    case deposit: return "deposit";
    case payoff: return "payoff";
    case refund: return "refund";
    case redeemvoucher: return "redeemvoucher";
    case buysnack: return "buysnack";
  }
}

// Init

inline static void init_metainfofile(spacebi_card_metainfofile_t *mf){
  memset(mf, 0, sizeof(spacebi_card_metainfofile_t));
  mf->structureversion = CURRENTAPPSTRUCTUREVERSION;
}

inline static void init_personalinfofile(spacebi_card_personalinfofile_t *pf){
  memset(pf, 0, sizeof(spacebi_card_personalinfofile_t));
}

inline static void init_ldapuserfile(spacebi_card_ldapuserfile_t *lf){
  memset(lf, 0, sizeof(spacebi_card_ldapuserfile_t));
}

inline static void init_creditmetafile(spacebi_card_creditmetafile_t *cf){
  memset(cf, 0, sizeof(spacebi_card_creditmetafile_t));
}

inline static void init_transactionrecord(spacebi_card_transaction_record_t *tr){
  memset(tr, 0, sizeof(spacebi_card_transaction_record_t));
}

// Dump

inline static void dump_metainfofile(spacebi_card_metainfofile_t mf){
  cout << "=== METAFILE ===" << endl;
  cout << "StructVer: 0x" << hex_str((uint8_t *)&mf.structureversion, 2) << endl;
  cout << "================" << endl;
}

inline static void dump_personalinfofile(spacebi_card_personalinfofile_t pf){
  cout << "=== PERSFILE ===" << endl;
  cout << "Timestamp: " << to_string(pf.issuedts) << endl;
  cout << "Memberid: " << to_string(pf.memberid) << endl;
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
  printf("%" PRIx64 "\n", rf.randomid);
  cout << endl << "================" << endl;
}

inline static void dump_creditmetafile(spacebi_card_creditmetafile_t cf){
  cout << "=== CREDITMETAFILE ===" << endl;
  cout << "Last transaction: " << to_string(cf.lasttransaction) << endl;
  for(uint8_t i; i < 32; i++){
    printf("%02x", cf.token[i]);
  }
  cout << "================" << endl;
}

inline static void dump_transaction(spacebi_card_transaction_record_t tr){
  cout << "Transaction: #" << to_string(tr.id) << " - " << to_string(tr.timestamp) << " - Usage: " << transaction_usage_to_str((transaction_usage_e) tr.usage) << " - Amount: " << to_string(tr.amount) << " - Balance: " << to_string(tr.balance) << endl;
}

#endif