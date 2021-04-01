#ifndef __MIFAREUTILS_H__
#define __MIFAREUTILS_H__

#include <iostream>
#include <freefare.h>
#include <nfc/nfc.h>

using namespace std;



extern string get_tag_str(enum freefare_tag_type tag_type);
extern int desfire_has_application(FreefareTag *tag, uint32_t lookup_aid);
extern bool hexstr_to_desfirekey(string algo, string hexstr, MifareDESFireKey *desfirekey);
extern bool null_desfirekey(string algo, MifareDESFireKey *desfirekey);
#endif