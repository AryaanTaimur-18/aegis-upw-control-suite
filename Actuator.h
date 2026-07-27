#ifndef ACTUATOR_H
#define ACTUATOR_H

#include <iostream>
#include <string>
using namespace std;

class Actuator {
protected:
    string name;
    bool isOpen;

public:
    Actuator(string n) : name(n), isOpen(true) {}
    virtual ~Actuator() {}

    virtual void trigger(double aiScore) = 0;
    
    bool getState() const { return isOpen; }
    string getName() const { return name; }
};

class MainProcessValve : public Actuator {
public:
    MainProcessValve() : Actuator("Main Fab Line") {}
    void trigger(double aiScore) override;
};

class DiverterValve : public Actuator {
public:
    DiverterValve() : Actuator("Recycle Diverter") { isOpen = false; }
    void trigger(double aiScore) override;
};

#endif
