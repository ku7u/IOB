#include "DCCFormatter.h"
#include "RBot.h"
#include "SerialCommand.h"

void DCCFormatter::setFunction(int dccAddress, int function, bool onOff)
{
    static bool fMap[29];
    uint8_t byte1;
    uint8_t byte2;
    char dummyChars[31];

    byte2 = 0;

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

    // build the command string
    String dummyString = "f ";
    dummyString.concat(String(dccAddress) + " ");
    dummyString.concat(String(byte1));

    if (function >= 13)
    {
        dummyString.concat(" ");
        dummyString.concat(String(byte2));
    }

    strcpy(dummyChars, dummyString.c_str());
    SerialCommand::parse(dummyChars);
    // if (callbackCommandFifoDCCFunction) callbackCommandFifoDCCFunction(dummyChars);
}

void DCCFormatter::setThrottle(int dccAddress, int currentSpeed, bool direction)
{
    char dummyChars[31];

    // build the command string
    char buffer[20];
    strcpy(dummyChars, "t 1 ");

    itoa(dccAddress, buffer, 10);
    strcat(dummyChars, buffer);
    strcat(dummyChars, " ");

    itoa(currentSpeed, buffer, 10);
    strcat(dummyChars, buffer);
    strcat(dummyChars, " ");

    if (direction)
        strcat(dummyChars, "1");
    else
        strcat(dummyChars, "0");

    SerialCommand::parse(dummyChars);
}

void DCCFormatter::setCV(int dccAddress, int cv, int value)
{
    char dummyChars[31];

    // build the command string
    char buffer[20];
    strcpy(dummyChars, "w ");

    itoa(dccAddress, buffer, 10);
    strcat(dummyChars, buffer);
    strcat(dummyChars, " ");

    itoa(cv, buffer, 10);
    strcat(dummyChars, buffer);
    strcat(dummyChars, " ");

    itoa(value, buffer, 10);
    strcat(dummyChars, buffer);

    SerialCommand::parse(dummyChars);
}