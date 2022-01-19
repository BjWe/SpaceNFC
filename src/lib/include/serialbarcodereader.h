#ifndef __SERIALBARCODEREADER_H__
#define __SERIALBARCODEREADER_H__

// Divider from Barcodescanner -> TAB
#define DIVIDERCHAR  0x09

#define BUFFSIZE     1024
#define CMD_ENABLE  'E'
#define CMD_DISABLE 'D'

#include <boost/circular_buffer.hpp>
#include <iostream>
using namespace std;

typedef void (*f_barcode_scanned_cb) (string); 

class SerialBarcodeReader {
 protected:
   string devicename;
   int fd;

   boost::circular_buffer<char> dataInBuff;
   f_barcode_scanned_cb barcodeScannedCB;

   bool busyFlag;

   void writeCommand(uint8_t cmd);
   void processMessage(uint8_t msg);

 public:
  SerialBarcodeReader(string devicename, f_barcode_scanned_cb barcodeScannedCB);
  ~SerialBarcodeReader();

  bool openDevice();
  bool closeDevice();

  void enable();
  void disable();

  bool isBusy();

  void update();

};

#endif