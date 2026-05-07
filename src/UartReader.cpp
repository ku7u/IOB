
#include "UartReader.h"
#include "Throttle.h"

extern HardwareSerial Serial1;
extern Throttle throttle;

#define START_SYNC 126
#define END_SYNC 127

/*
Read the POL (Position On Layout) UART value derived from sensor on loco tank
Two one byte POL values are provided by the track at each waypoint about 2cm apart
One is west side and the other is eastside
MSB of byte depicts the direction, 0 = west, 1 = east
Remainder of byte is the waypoint ID, 127 possible, zero excluded (TBD)
If waypoint is detected, provide to throttle object for further use
Apparently this scheme does not result in telemetry messages being generated directly

Algorithm:
Read from UART
If ID value different from last, then set lastValue = byte read
If same, ignore and return

*/

bool UartReader::check()
{
    uint8_t inByte;
    char candidateByte;

    char buffer[10];
    char msgChars[20];
    char topicChars[50];
    strcpy(topicChars, "tlm/ols/pol");

    // inByte = 0;

    if (Serial1.available())
    {
        // read a byte
        inByte = Serial1.read();

        if (inByte == START_SYNC)
        {
            // always set to stage 1 if START_SYNC encountered
            stage = 1;
            return false;
        }

        switch (stage)
        {
        case 1:
            // check for valid data byte, not sync bytes (or zero, not sure why that sometimes appears as data TBD)
            if ((inByte == START_SYNC) || (inByte == END_SYNC) || (inByte == 0))
            {
                stage = 0;
                counter = 0;
                candidate = 0;
                return false;
            }
            candidate = inByte;
            stage = 2;
            return false;
        case 2:
            if (inByte == END_SYNC)
            // found 3 bytes in proper sequence, now analyze
            {
                // compare against previous whole byte
                // TBD this part is superfluous if checking for a string of several later
                if (candidate == currentPOL)
                {
                    // multiple identical values received, ignore
                    stage = 0;
                    counter = 0;
                    return false;
                }
                // compare POL part disregarding direction bit 7
                // else if ((0b01111111 && inByte) != (0b01111111 && lastValue))
                else
                // there is something different about this candidate
                {
                    counter++;
                    if (counter == 2)
                    {
                        counter = 0;
                        // this is new and a good one
                        // currentPOL = candidate;
                        itoa(currentPOL, buffer, 10);
                        strcpy(msgChars, buffer);

                        // TBD do something here, this is new info
                        if (inByte == currentPOL)
                        {
                            // this is a repeater so ignore
                            return false;
                        }
                        // compare to currentPOL, numeric part
                        else if ((0b01111111 && inByte) != (0b01111111 && currentPOL))
                        {
                            // if bits 0-6 are different, reset, save as currentPOL
                            currentPOL = candidate;
                            return false;
                        }
                        else
                        {
                            // compare to currentPOL, direction bit
                            // if we get here bits 0-6 match but bit 7 is different, bingo
                            // determine direction, if bit 7=0, heading west, else east
                            // shift bits 7 places to the right, filling left end with zeros
                            directionEast = candidate >> 7;
                            // notify throttle object
                            throttle.setWaypoint(currentPOL, directionEast);
                        }
                        return true;
                    }
                }
            }
            else
            {
                // garbage encountered so reset
                candidate = 0;
                stage = 0;
                return false;
            }
            break;
        default:
            return false;
        }

        //         if ((0b01111111 && inByte) != (0b01111111 && lastValue))
        //         {
        //             // TBD here, seeing a new waypoint so start over from scratch
        //             // deal with garbage bytes
        //             lastValue = inByte;
        //             return false;
        //         }
        //         else
        //         {
        //             // must be the EW bit then
        //             // TBA statements
        //             lastValue = inByte;
        //             return false;
        //         }

        //         return false;   // TBD on returning anything from this routine
        //     }
    }
    return false;
}