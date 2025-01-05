#pragma once

#include "Arduino.h"
#include <vector>

class Train
{
public:
    Train(void);

    struct RailCar
    {
        const char * id;
        uint8_t tons;
        uint8_t length;
        const char * color;
        const char * aar;
    };
    
    void addCar(char* jsonMsg, uint segmentID);
    void removeCar(char* jsonMsg, uint segmentID);
    uint trainMass(void);   // tons
    uint trainLength(void);     // feet
    uint carCount(void);

private:
    std::vector<RailCar> theTrain;
    uint _trainMass;
    uint _trainLength;
    uint _carCount;

    /*
    // json document structure for cars in train
    // json string stored in spiffs
    {
        [
            "i":id
                {
                    "w":weight (tons),
                    "l":length (feet),
                    "c":color,
                    "t",aar type
                }
        ]
    }
    */
};