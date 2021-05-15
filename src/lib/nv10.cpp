#include "include/nv10.h"

#include <spdlog/spdlog.h>
#include <termios.h>

#include <boost/filesystem.hpp>
#include <iostream>

using namespace std;

void NV10SIO::writeCommand(uint8_t cmd) {
  spdlog::trace("nv10 send command {0:d} / {0:x}", cmd, cmd);
  write(this->fd, &cmd, 1);
}

NV10SIO::NV10SIO(string devicename) {
  this->devicename = devicename;
  this->validChannel = 0;
  this->busyFlag = false;
  this->isStrimmingDetected = false;
  this->isFraudDetected = false;
  this->isStackerBlocked = false;

  spdlog::trace("new nv10sio: Device is: {}", devicename);
}

NV10SIO::~NV10SIO() {
}

bool NV10SIO::openDevice() {
  // Open the Port. We want read/write, no "controlling tty" status, and open it no matter what state DCD is in
  this->fd = open(this->devicename.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
  if (this->fd == -1) {
    spdlog::error("nv10 open_port: Unable to open {}", this->devicename);
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

bool NV10SIO::closeDevice() {}

void NV10SIO::enable() {
  spdlog::debug("nv10 enable");
  writeCommand(ENABLE_ALL);
}

void NV10SIO::disable() {
  spdlog::debug("nv10 disable");
  writeCommand(DISABLE_ALL);
}

void NV10SIO::enableNote(uint channel) {
  if (channel < 1 || channel > 16) {
    spdlog::error("nv10 invalid channel (enable) {}", channel);
    return;
  }
  spdlog::debug("nv10 enable channel {}", channel);
  writeCommand(ENABLE_CHANNEL_01 + channel - 1);
}
void NV10SIO::inhibitNote(uint channel) {
  if (channel < 1 || channel > 16) {
    spdlog::error("nv10 invalid channel (inhibit) {}", channel);
    return;
  }
  spdlog::debug("nv10 inhibit channel {}", channel);
  writeCommand(INHIBIT_CHANNEL_01 + channel - 1);
}

void NV10SIO::enableEscrow() {
  spdlog::debug("nv10 enable escrow");
  writeCommand(ENABLE_ESCROW_MODE);
}
void NV10SIO::disableEscrow() {
  spdlog::debug("nv10 disable escrow");
  writeCommand(DISABLE_ESCROW_MODE);
}
void NV10SIO::escrowAccept() {
  spdlog::debug("nv10 accept escrow");
  writeCommand(ACCEPT_ESCROW);
}
void NV10SIO::escrowReject() {
  spdlog::debug("nv10 reject escrow");
  writeCommand(REJECT_ESCROW);
}

uint NV10SIO::getValidChannel(){
    return this->validChannel;
}

void NV10SIO::clearValidChannel(){
    this->validChannel = 0;
}

bool NV10SIO::isBusy() {
  return this->busyFlag;
}

bool NV10SIO::fraudDetected() {
  return this->isFraudDetected;
}

bool NV10SIO::strimmingDetected() {
  return this->isStrimmingDetected;
}

bool NV10SIO::stackerBlocked() {
  return this->isStackerBlocked;
}

void NV10SIO::clearStrimming() {
  this->isStrimmingDetected = false;
}

void NV10SIO::clearFraud() {
  this->isFraudDetected = false;
}

void NV10SIO::processMessage(uint8_t msg) {
  spdlog::trace("nv10 process message {0:d} / 0x{0:x}", msg, msg);

  switch (msg) {
    case NOTE_NOT_RECOGNISED: {
      spdlog::trace("nv10 note not recognized");
      this->validChannel = 0;
      break;
    }
    case MECHANISCM_RUNNING_SLOW: {
      spdlog::warn("nv10 running slow");
      break;
    }
    case STRIMMING_ATTEMPTED: {
      spdlog::error("nv10 strimming attempted");
      this->isStrimmingDetected = true;
      break;
    }
    case NOTE_REJECTED_FRAUD: {
      spdlog::error("nv10 fraud rejected");
      this->isFraudDetected = true;
      break;
    }
    case STACKER_BLOCKED: {
      spdlog::error("nv10 stacker blocked");
      this->isStackerBlocked = true;
      break;
    }
    case ABORT_WHILE_ESCROW: {
      spdlog::warn("nv10 escrow aborted");
      break;
    }
    case NOTE_TAKEN_CLEAR_BLOCK: {
      spdlog::warn("nv10 escrow aborted while block");
      break;
    }
    case VALIDATOR_BUSY: {
      spdlog::debug("nv10 validator busy");
      this->busyFlag = true;
      break;
    }
    case VALIDATOR_NOT_BUSY: {
      spdlog::trace("nv10 validator ready");
      this->busyFlag = false;
      break;
    }
    case NOTE_ACCEPT_CHANNEL_01: {
      spdlog::info("nv10 note detected on channel 1");
      this->validChannel = 1;
      break;
    }
    case NOTE_ACCEPT_CHANNEL_02: {
      spdlog::info("nv10 note detected on channel 2");
      this->validChannel = 2;
      break;
    }
    case NOTE_ACCEPT_CHANNEL_03: {
      spdlog::info("nv10 note detected on channel 3");
      this->validChannel = 3;
      break;
    }
    case NOTE_ACCEPT_CHANNEL_04: {
      spdlog::info("nv10 note detected on channel 4");
      this->validChannel = 4;
      break;
    }
    case NOTE_ACCEPT_CHANNEL_05: {
      spdlog::info("nv10 note detected on channel 5");
      this->validChannel = 5;
      break;
    }
    case NOTE_ACCEPT_CHANNEL_06: {
      spdlog::info("nv10 note detected on channel 6");
      this->validChannel = 6;
      break;
    }
    case NOTE_ACCEPT_CHANNEL_07: {
      spdlog::info("nv10 note detected on channel 7");
      this->validChannel = 7;
      break;
    }
    case NOTE_ACCEPT_CHANNEL_08: {
      spdlog::info("nv10 note detected on channel 8");
      this->validChannel = 8;
      break;
    }
    case NOTE_ACCEPT_CHANNEL_09: {
      spdlog::info("nv10 note detected on channel 9");
      this->validChannel = 9;
      break;
    }
    case NOTE_ACCEPT_CHANNEL_10: {
      spdlog::info("nv10 note detected on channel 10");
      this->validChannel = 10;
      break;
    }
    case NOTE_ACCEPT_CHANNEL_11: {
      spdlog::info("nv10 note detected on channel 11");
      this->validChannel = 11;
      break;
    }
    case NOTE_ACCEPT_CHANNEL_12: {
      spdlog::info("nv10 note detected on channel 12");
      this->validChannel = 12;
      break;
    }
    case NOTE_ACCEPT_CHANNEL_13: {
      spdlog::info("nv10 note detected on channel 13");
      this->validChannel = 13;
      break;
    }
    case NOTE_ACCEPT_CHANNEL_14: {
      spdlog::info("nv10 note detected on channel 14");
      this->validChannel = 14;
      break;
    }
    case NOTE_ACCEPT_CHANNEL_15: {
      spdlog::info("nv10 note detected on channel 15");
      this->validChannel = 15;
      break;
    }
    case NOTE_ACCEPT_CHANNEL_16: {
      spdlog::info("nv10 note detected on channel 16");
      this->validChannel = 16;
      break;
    }

    case ENABLE_CHANNEL_01: {
      spdlog::debug("nv10 channel 1 enabled");
      break;
    }
    case ENABLE_CHANNEL_02: {
      spdlog::debug("nv10 channel 2 enabled");
      break;
    }
    case ENABLE_CHANNEL_03: {
      spdlog::debug("nv10 channel 3 enabled");
      break;
    }
    case ENABLE_CHANNEL_04: {
      spdlog::debug("nv10 channel 4 enabled");
      break;
    }
    case ENABLE_CHANNEL_05: {
      spdlog::debug("nv10 channel 5 enabled");
      break;
    }
    case ENABLE_CHANNEL_06: {
      spdlog::debug("nv10 channel 6 enabled");
      break;
    }
    case ENABLE_CHANNEL_07: {
      spdlog::debug("nv10 channel 7 enabled");
      break;
    }
    case ENABLE_CHANNEL_08: {
      spdlog::debug("nv10 channel 8 enabled");
      break;
    }
    case ENABLE_CHANNEL_09: {
      spdlog::debug("nv10 channel 9 enabled");
      break;
    }
    case ENABLE_CHANNEL_10: {
      spdlog::debug("nv10 channel 10 enabled");
      break;
    }
    case ENABLE_CHANNEL_11: {
      spdlog::debug("nv10 channel 11 enabled");
      break;
    }
    case ENABLE_CHANNEL_12: {
      spdlog::debug("nv10 channel 12 enabled");
      break;
    }
    case ENABLE_CHANNEL_13: {
      spdlog::debug("nv10 channel 13 enabled");
      break;
    }
    case ENABLE_CHANNEL_14: {
      spdlog::debug("nv10 channel 14 enabled");
      break;
    }
    case ENABLE_CHANNEL_15: {
      spdlog::debug("nv10 channel 15 enabled");
      break;
    }
    case ENABLE_CHANNEL_16: {
      spdlog::debug("nv10 channel 16 enabled");
      break;
    }
    case INHIBIT_CHANNEL_01: {
      spdlog::debug("nv10 channel 1 inhibited");
      break;
    }
    case INHIBIT_CHANNEL_02: {
      spdlog::debug("nv10 channel 2 inhibited");
      break;
    }
    case INHIBIT_CHANNEL_03: {
      spdlog::debug("nv10 channel 3 inhibited");
      break;
    }
    case INHIBIT_CHANNEL_04: {
      spdlog::debug("nv10 channel 4 inhibited");
      break;
    }
    case INHIBIT_CHANNEL_05: {
      spdlog::debug("nv10 channel 5 inhibited");
      break;
    }
    case INHIBIT_CHANNEL_06: {
      spdlog::debug("nv10 channel 6 inhibited");
      break;
    }
    case INHIBIT_CHANNEL_07: {
      spdlog::debug("nv10 channel 7 inhibited");
      break;
    }
    case INHIBIT_CHANNEL_08: {
      spdlog::debug("nv10 channel 8 inhibited");
      break;
    }
    case INHIBIT_CHANNEL_09: {
      spdlog::debug("nv10 channel 9 inhibited");
      break;
    }
    case INHIBIT_CHANNEL_10: {
      spdlog::debug("nv10 channel 10 inhibited");
      break;
    }
    case INHIBIT_CHANNEL_11: {
      spdlog::debug("nv10 channel 11 inhibited");
      break;
    }
    case INHIBIT_CHANNEL_12: {
      spdlog::debug("nv10 channel 12 inhibited");
      break;
    }
    case INHIBIT_CHANNEL_13: {
      spdlog::debug("nv10 channel 13 inhibited");
      break;
    }
    case INHIBIT_CHANNEL_14: {
      spdlog::debug("nv10 channel 14 inhibited");
      break;
    }
    case INHIBIT_CHANNEL_15: {
      spdlog::debug("nv10 channel 15 inhibited");
      break;
    }
    case INHIBIT_CHANNEL_16: {
      spdlog::debug("nv10 channel 16 inhibited");
      break;
    }
  }
}

void NV10SIO::update() {
  uint8_t buff[20];

  int n = read(fd, (void*)buff, sizeof(buff));
  if (n < 0) {
    perror("Read failed - ");
  } else if (n == 0) {
  } else {
    for (int i = 0; i < n; i++) {
      printf("key loop %i\n", i);
      processMessage(buff[i]);
    }
  }
}