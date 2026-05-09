
// this code is called from fifo
// it is only for function commands to DCC, not throttle
// throttle commands are not sent through the fifo so come straight out of throttle
// this scenario should be simplified

#include "Arduino.h"
#include "RBot.h"
#include "SerialCommand.h"
#include "Function.h"
#include "ILocoStatus.h"

static ILocoStatus *_loco = nullptr; // The "Plug"

void setLocoStatusProvider(ILocoStatus *provider)
{
    _loco = provider;
}

void setFunction(int function, bool onOff)
{
    static bool fMap[29];
    uint8_t byte1;
    uint8_t byte2;
    char dummyChars[31];

    byte2 = 0;

    if (!_loco)
        return;

    // int roadNum = throttle.getDccAddress();
    int roadNum = _loco->getDccAddress();

    if ((function > 29) || (function < 0))
        return;

    fMap[function] = onOff;

    if (function < 5)
        byte1 = 128 + fMap[1] * 1 + fMap[2] * 2 + fMap[3] * 4 + fMap[4] * 8 + fMap[0] * 16;
    else if (function < 9)
        byte1 = 176 + fMap[5] * 1 + fMap[6] * 2 + fMap[7] * 4 + fMap[8] * 8;
    else if (function < 13)
        byte1 = 160 + fMap[9] * 1 + fMap[10] * 2 + fMap[11] * 4 + fMap[12] * 8;
    else if (function < 21)
    {
        byte1 = 222;
        byte2 = fMap[13] * 1 + fMap[14] * 2 + fMap[15] * 4 + fMap[16] * 8 +
                fMap[17] * 16 + fMap[18] * 32 + fMap[19] * 64 + fMap[20] * 128;
    }
    else
    {
        byte1 = 223;
        byte2 = fMap[21] * 1 + fMap[22] * 2 + fMap[23] * 4 + fMap[24] * 8 +
                fMap[25] * 16 + fMap[26] * 32 + fMap[27] * 64 + fMap[28] * 128;
    }


    if (function >= 13)
    {
        snprintf(dummyChars, sizeof(dummyChars), "f %d %u %u", roadNum, byte1, byte2);
    }
    else
    {
        snprintf(dummyChars, sizeof(dummyChars), "f %d %u", roadNum, byte1);
    }
    SerialCommand::parse(dummyChars);
}

// void startStop(bool start)
// {
//     if (start)
//     {
//         setFunction(functionPM, true);
//         setFunction(functionNotchingEnable, true);
//     }
//     else
//     {
//         setFunction(functionNotchingEnable, false);
//         setFunction(functionPM, false);
//     }
// }