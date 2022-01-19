#include "include/serialbarcodereader.h"

#include <spdlog/spdlog.h>
#include <termios.h>

#include <boost/range/adaptor/indexed.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/filesystem.hpp>
#include <iostream>

using namespace std;

void SerialBarcodeReader::writeCommand(uint8_t cmd) {
  spdlog::trace("SerialBarcodeReader send command {0:d} / {0:x}", cmd, cmd);
  write(this->fd, &cmd, 1);
}

SerialBarcodeReader::SerialBarcodeReader(string devicename, f_barcode_scanned_cb barcodeScannedCB) {
  this->dataInBuff = boost::circular_buffer<char>(BUFFSIZE);

  this->devicename = devicename;
  this->barcodeScannedCB = barcodeScannedCB;
  this->busyFlag = false;

  spdlog::trace("new SerialBarcodeReader: Device is: {}", devicename);
}

SerialBarcodeReader::~SerialBarcodeReader() {
}

bool SerialBarcodeReader::openDevice() {
  // Open the Port. We want read/write, no "controlling tty" status, and open it no matter what state DCD is in
  this->fd = open(this->devicename.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
  if (this->fd == -1) {
    spdlog::error("SerialBarcodeReader open_port: Unable to open {}", this->devicename);
    return false;
  }

  struct termios options;
  tcgetattr(fd, &options);
  options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
  options.c_iflag = IGNPAR;
  options.c_oflag = 0;
  options.c_lflag = 0;
  options.c_cc[VTIME] = 5;
  options.c_lflag &= ~ICANON;
  tcflush(fd, TCIFLUSH);
  tcsetattr(fd, TCSANOW, &options);

  int val = fcntl(fd, F_GETFL, 0);
  printf("post-open file status = 0x%x\n", val);

  fcntl(fd, F_SETFL, 0);
  val = fcntl(fd, F_GETFL, 0 | O_NONBLOCK);
  printf("post-fcntl file status = 0x%x\n", val);

  return true;
}

bool SerialBarcodeReader::closeDevice() {
  close(this->fd);
}

void SerialBarcodeReader::enable() {
  spdlog::debug("SerialBarcodeReader enable");
  writeCommand(CMD_ENABLE);
}

void SerialBarcodeReader::disable() {
  spdlog::debug("SerialBarcodeReader disable");
  writeCommand(CMD_DISABLE);
}

bool SerialBarcodeReader::isBusy() {
  return this->busyFlag;
}

void SerialBarcodeReader::processMessage(uint8_t msg) {
  spdlog::trace("SerialBarcodeReader process message", msg);

}

void SerialBarcodeReader::update() {
  uint8_t buff[1024];

  // Read everything into circular buffer
  int n = read(fd, (void*)buff, sizeof(buff));
  if (n < 0) {
    perror("Read failed - ");
  } else if (n == 0) {

  } else {
    for (int i = 0; i < n; i++) {
      dataInBuff.push_back(buff[i]);
      spdlog::trace("serial recv {0:x}", buff[i]);
    }
  }
  
  // Lookup for Divider
  //dataInBuff.linearize(); really needed?
  int dividerPos = -1;
  for(auto const& c : dataInBuff | boost::adaptors::indexed(0)){
    if(c.value() == DIVIDERCHAR){
      dividerPos = c.index();
      break;
    }
  }

  // If something is found, extract it
  if(dividerPos >= 0){
    string barcode = "";
    for(int i = 0; i < dividerPos; i++){
      barcode.push_back(dataInBuff.front());
      dataInBuff.pop_front();
    }
    // Pop divider
    dataInBuff.pop_front();

    // Quick Fix (QuantumT adds 0x0a behind configured divider)
    if(dataInBuff[0] == 0x0a){
      dataInBuff.pop_front();
      spdlog::debug("removed 0x0a from QuantumT");
    }

    cout << "BuffEmpty: " << std::boolalpha << dataInBuff.empty() << "\r\n";
    if(!dataInBuff.empty()){
      for(auto const& c : dataInBuff | boost::adaptors::indexed(0)){
        spdlog::trace("leftovers in buff at {0:d} {0:x}", c.index(), c.value());
      }
    }
    //cout << "'" << barcode << "'\r\n";
    spdlog::debug("scanned code: '{0:s}'", barcode);
    this->barcodeScannedCB(barcode);
  }
}