#ifndef _SPACEBICARDSTRUCTS_H
#define _SPACEBICARDSTRUCTS_H

#include <stdint.h>

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



#endif