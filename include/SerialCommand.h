/**********************************************************************

SerialCommand.h
COPYRIGHT (c) 2013-2016 Gregg E. Berman

Part of DCC++ BASE STATION for the Arduino

**********************************************************************/

#ifndef SerialCommand_h
#define SerialCommand_h

#include "PacketRegister.h"
// #include "CurrentMonitor.h"  //TBD remove current monitor

#define  MAX_COMMAND_LENGTH         30

struct SerialCommand{
  static char commandString[MAX_COMMAND_LENGTH+1];
  static volatile RegisterList *mRegs, *pRegs;
  // static CurrentMonitor *mMonitor; //TBD remove current monitor
  // static void init(volatile RegisterList *, volatile RegisterList *, CurrentMonitor *);
  static void init(volatile RegisterList *, volatile RegisterList *);   //TBD remove current monitor
  static void parse(char *);
  static void process();
}; // SerialCommand
  
#endif




