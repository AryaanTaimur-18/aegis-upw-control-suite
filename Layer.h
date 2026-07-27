#ifndef LAYER_H
#define LAYER_H
#include <iostream>
#include <cmath>
#include <vector>
#include <random>
#include "matrix.h"
using namespace std;

class Layer {
private:
    Matrix weights;
    Matrix biases;
    Matrix lastOutput;

    double sigmoid(double x) {
        return 1.0 / (1.0 + exp(-x));
    }

public:
    Layer(int inputs, int neurons);
    Matrix forward(const Matrix& input);
    vector<double> getActivations() const;
    void printLayerInfo() const;
};

#endif