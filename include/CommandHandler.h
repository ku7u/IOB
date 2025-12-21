#pragma once
#include "UdpTransport.h"
#include "Throttle.h" // whatever your class is called

class Throttle; // <-- forward declaration

struct PendingCommand // enquing
{
    IPAddress ip;
    char topic[32];
    char value[128];
};

class CommandHandler
{
public:
    CommandHandler(UdpTransport &transport, Throttle &throttle);
bool getNext(PendingCommand& out);

    void loop();

private:
    void enqueue(const char *topic, // enquing
                 const char *value,
                 IPAddress ip);

    static constexpr int QUEUE_SIZE = 8;
    PendingCommand _queue[QUEUE_SIZE]; 
    volatile int _head = 0;            
    volatile int _tail = 0;            

    UdpTransport &_transport;
    Throttle &_throttle; // store reference
    const char *getSubstringAfterLastSlash(const char *input);
};
