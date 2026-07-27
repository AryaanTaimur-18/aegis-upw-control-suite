#include "DataLogger.h"
#include <iostream>

DataLogger::DataLogger(const string& filename) : headerWritten(false) {
    file.open(filename, ios::app);
    if (!file.is_open()) {
        throw runtime_error("Could not open file: " + filename);
    }
}

void DataLogger::logDecision(double sensorValue, double aiScore, const string& action) {
    if (!headerWritten) {
        file << "Timestamp,SensorValue,AIScore,Action\n";
        headerWritten = true;
    }
    
    auto now = chrono::system_clock::now();
    auto time = chrono::system_clock::to_time_t(now);
    
    file << time << "," << sensorValue << "," << aiScore << "," << action << "\n";
    file.flush();
}

DataLogger::~DataLogger() {
    if (file.is_open()) {
        file.close();
    }
}
