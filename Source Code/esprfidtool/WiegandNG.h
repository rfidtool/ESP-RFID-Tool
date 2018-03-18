#ifndef _WIEGAND_NG_H
#define _WIEGAND_NG_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

class WiegandNG {

private:
  static void ReadD0();
  static void ReadD1();
  static volatile unsigned long   _lastPulseTime;   // time last bits received
  static volatile unsigned int  _bitCounted;    // number of bits arrived at Interrupt pins
  static unsigned int       _bufferSize;    // memory (bytes) allocated for buffer
  unsigned int          _bitAllocated;    // wiegand bits required
  unsigned int          _packetGap;     // gap between wiegand packet in millisecond
  static volatile unsigned char * _buffer;      // buffer for data retention
  
public:
  bool begin(unsigned int bits, unsigned int packetGap=25);       // default packetGap is 25ms
  bool begin(uint8_t pinD0, uint8_t pinD1, unsigned int bits, unsigned int packetGap);
  bool available();
  void clear();
  void pause();
  unsigned int getBitCounted();
  unsigned int getBitAllocated();
  unsigned int getBufferSize();
  unsigned int getPacketGap();
  volatile unsigned char *getRawData();
  WiegandNG();
  ~WiegandNG();
};

#endif

