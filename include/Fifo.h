#ifndef FIFO_H
#define FIFO_H

#include <queue>
using namespace std;

class Fifo
{
public:
    void pushCommand(int, bool);
    void pop(void);

private:
    struct _functionParms
    {
        int functionID;
        bool onOff;
    };

    queue<_functionParms> myQ; // for DCC command throttling
};

#endif