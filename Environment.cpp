#include "Environment.h"
using namespace std;

Environment::Environment() : truePurity(1.0), contaminationEvent(false) {
    random_device rd;
    gen.seed(rd());
}

void Environment::tick() {
    if (contaminationEvent) {
        truePurity -= 0.05; // Achanak tezi se ganda hona
    } else {
        truePurity -= 0.0001; // Normal wear and tear
    }

    if (truePurity < 0) truePurity = 0;
}

void Environment::injectLeak() {
    contaminationEvent = true;
    cout << "[ENV] !!! WARNING: Contaminant Leaked into Pipes !!!" << endl;
}

void Environment::fixSystem() {
    contaminationEvent = false;
    truePurity = 1.0;
    cout << "[ENV] System Flushed and Filters Replaced." << endl;
}

double Environment::getResistivitySignal() {
    // 18.2 M-Ohm is perfect. Hum thora random noise add karenge
    uniform_real_distribution<> noise(-0.05, 0.05);
    return (truePurity * 18.2) + noise(gen);
}

double Environment::getTOCSignal() {
    // Low TOC is good. Purity kam hogi toh TOC barhay ga.
    uniform_real_distribution<> noise(0.0, 0.02);
    return ((1.0 - truePurity) * 50.0) + noise(gen); 
}

