#ifndef DATALOGGER_H
#define DATALOGGER_H
#include <fstream>
#include <string>
#include <chrono>
using namespace std;

class DataLogger {
private:
    ofstream file;
    bool headerWritten;

public:
    DataLogger(const string& filename);
    void logDecision(double sensorValue, double aiScore, const string& action);
    ~DataLogger();
};

#endif
