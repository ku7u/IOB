// Wiring.h
#pragma once

// Forward declarations (cheap, no #include needed here)
class Throttle;
class BrakeSystem;
class Fifo;
class SerialCommand;
class DCCFormatter;

// One function to rule them all
void connectSystems(Throttle& t, BrakeSystem& b, Fifo& f, DCCFormatter& d);

