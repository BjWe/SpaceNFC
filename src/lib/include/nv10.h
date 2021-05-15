#ifndef __NV10_H__
#define __NV10_H__

#define INHIBIT_CHANNEL_01      131
#define INHIBIT_CHANNEL_02      132
#define INHIBIT_CHANNEL_03      133
#define INHIBIT_CHANNEL_04      134
#define INHIBIT_CHANNEL_05      135
#define INHIBIT_CHANNEL_06      136
#define INHIBIT_CHANNEL_07      137
#define INHIBIT_CHANNEL_08      138
#define INHIBIT_CHANNEL_09      139
#define INHIBIT_CHANNEL_10      140
#define INHIBIT_CHANNEL_11      141
#define INHIBIT_CHANNEL_12      142
#define INHIBIT_CHANNEL_13      143
#define INHIBIT_CHANNEL_14      144
#define INHIBIT_CHANNEL_15      145
#define INHIBIT_CHANNEL_16      146
#define ENABLE_CHANNEL_01       151
#define ENABLE_CHANNEL_02       152
#define ENABLE_CHANNEL_03       153
#define ENABLE_CHANNEL_04       154
#define ENABLE_CHANNEL_05       155
#define ENABLE_CHANNEL_06       156
#define ENABLE_CHANNEL_07       157
#define ENABLE_CHANNEL_08       158
#define ENABLE_CHANNEL_09       159
#define ENABLE_CHANNEL_10       160
#define ENABLE_CHANNEL_11       161
#define ENABLE_CHANNEL_12       162
#define ENABLE_CHANNEL_13       163
#define ENABLE_CHANNEL_14       164
#define ENABLE_CHANNEL_15       165
#define ENABLE_CHANNEL_16       166
#define ENABLE_ESCROW_MODE      170
#define DISABLE_ESCROW_MODE     171
#define ACCEPT_ESCROW           172
#define REJECT_ESCROW           173
#define STATUS_REQUEST          182
#define ENABLE_ALL              184
#define DISABLE_ALL             185
#define DISABLE_ESCROW_TO       190
#define ENABLE_ESCROW_TO        191

#define NOTE_ACCEPT_CHANNEL_01    1
#define NOTE_ACCEPT_CHANNEL_02    2
#define NOTE_ACCEPT_CHANNEL_03    3
#define NOTE_ACCEPT_CHANNEL_04    4
#define NOTE_ACCEPT_CHANNEL_05    5
#define NOTE_ACCEPT_CHANNEL_06    6
#define NOTE_ACCEPT_CHANNEL_07    7
#define NOTE_ACCEPT_CHANNEL_08    8
#define NOTE_ACCEPT_CHANNEL_09    9
#define NOTE_ACCEPT_CHANNEL_10   10
#define NOTE_ACCEPT_CHANNEL_11   11
#define NOTE_ACCEPT_CHANNEL_12   12
#define NOTE_ACCEPT_CHANNEL_13   13
#define NOTE_ACCEPT_CHANNEL_14   14
#define NOTE_ACCEPT_CHANNEL_15   15
#define NOTE_ACCEPT_CHANNEL_16   16
#define NOTE_NOT_RECOGNISED      20
#define MECHANISCM_RUNNING_SLOW  30
#define STRIMMING_ATTEMPTED      40
#define NOTE_REJECTED_FRAUD      50
#define STACKER_BLOCKED          60
#define ABORT_WHILE_ESCROW       70
#define NOTE_TAKEN_CLEAR_BLOCK   80
#define VALIDATOR_BUSY          120
#define VALIDATOR_NOT_BUSY      121
#define COMMAND_ERROR           255

#include <iostream>
using namespace std;


class NV10SIO {
 protected:
   string devicename;
   int fd;

   bool busyFlag;
   bool isStrimmingDetected;
   bool isFraudDetected;
   bool isStackerBlocked;
   int validChannel;

   void writeCommand(uint8_t cmd);
   void processMessage(uint8_t msg);

 public:
  NV10SIO(string devicename);
  ~NV10SIO();

  bool openDevice();
  bool closeDevice();

  void enable();
  void disable();

  void enableNote(uint channel);
  void inhibitNote(uint channel);

  void enableEscrow();
  void disableEscrow();
  void escrowAccept();
  void escrowReject();

  uint getValidChannel();
  void clearValidChannel();

  bool isBusy();
  bool fraudDetected();
  bool strimmingDetected();
  bool stackerBlocked();
  void clearStrimming();
  void clearFraud();


  void update();

};

#endif