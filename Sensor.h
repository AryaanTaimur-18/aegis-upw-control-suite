#ifndef SENSOR_H
#define SENSOR_H
#include <string>
using namespace std;


class Sensor {
public:
    virtual ~Sensor() {} 
    virtual double readValue() = 0;
    virtual string getUnit() = 0;
};


class ResistivitySensor : public Sensor {
public:
    double readValue() override {
       
        return 18.2; 
    }
    string getUnit() override { return "M-Ohm"; }
};


class TOCSensor : public Sensor {
public:
    double readValue() override {
        return 0.05;
    }
    string getUnit() override { return "ppb"; }
};

#endif