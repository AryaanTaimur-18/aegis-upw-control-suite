#include <iostream>
#include <vector>
#include "matrix.h"
#include "Layer.h"
#include "neuralnetwork.h"
#include "Sensor.h"
#include "Environment.h"
#include "Actuator.h"
#include "DataLogger.h"
using namespace std;

int main() {
    Environment upwPlant;
    NeuralNetwork brain;
    brain.addLayer(2, 4);
    brain.addLayer(4, 1);

    MainProcessValve fabValve;
    DiverterValve recycleValve;
    DataLogger logger("fab_telemetry.csv");

    cout << "--- AEGIS-UPW: Silicon Fabrication Water Shield Active ---" << endl;

    for (int tick = 1; tick <= 30; tick++) {
        cout << "\n[TICK " << tick << "]";

        if (tick == 15) {
            upwPlant.injectLeak();
        }

        upwPlant.tick();
        double rValue = upwPlant.getResistivitySignal();
        double tValue = upwPlant.getTOCSignal();

        Matrix input(1, 2);
        input.set(0, 0, rValue / 18.2);
        input.set(0, 1, tValue / 50.0);
        
        Matrix prediction = brain.predict(input);
        double safetyScore = prediction.get(0, 0);

        fabValve.trigger(safetyScore);
        recycleValve.trigger(safetyScore);

        logger.logDecision(rValue, safetyScore, fabValve.getState() ? "FLOWING" : "BLOCKED");

        cout << " | Resistivity: " << rValue << " M-Ohm | AI Safety Score: " << safetyScore;
    }

    cout << "\n\n--- Simulation Complete. Check fab_telemetry.csv for logs. ---" << endl;
    return 0;
}