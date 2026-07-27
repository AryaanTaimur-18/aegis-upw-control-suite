#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <random>
#include <iostream>

class Environment {
private:
    double truePurity;       // Actual state of water (0.0 to 1.0)
    bool contaminationEvent; // Is there a leak right now?
    
    std::mt19937 gen;        // Standard random engine

public:
    Environment();
    
    void tick();             // Simulation ka ek qadam aagay
    void injectLeak();       // Accident trigger karne ke liye
    void fixSystem();        // Sab theek karne ke liye

    // Sensors ko milne wala data (with noise)
    double getResistivitySignal();
    double getTOCSignal();
};

#endif