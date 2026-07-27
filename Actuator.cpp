#include "Actuator.h"

void MainProcessValve::trigger(double aiScore) {
    bool previousState = isOpen;

    if (aiScore < 0.7) {
        isOpen = false;
    } else {
        isOpen = true;
    }

    if (previousState != isOpen) {
        cout << "[ACTUATOR] Main Process Valve is now " 
             << (isOpen ? "OPEN (Flowing to Fab)" : "CLOSED (Safety Stop)") << endl;
    }
}

void DiverterValve::trigger(double aiScore) {
    bool previousState = isOpen;

    if (aiScore < 0.4) {
        isOpen = true;
    } else {
        isOpen = false;
    }

    if (previousState != isOpen) {
        cout << "[ACTUATOR] Diverter Valve is now " 
             << (isOpen ? "ACTIVE (Recycling Water)" : "INACTIVE (Normal Mode)") << endl;
    }
}