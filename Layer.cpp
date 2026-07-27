#include "Layer.h"
#include "matrix.h"
#include <random>
#include <iostream>
using namespace std;

Layer::Layer(int inputs, int neurons)
    : weights(inputs, neurons),
      biases(1, neurons),
      lastOutput(1, neurons) {
    
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dis(-1.0, 1.0);

    for(int i = 0; i < inputs; i++) {
        for(int j = 0; j < neurons; j++) {
            weights.set(i, j, dis(gen));
        }
    }

    for(int j = 0; j < neurons; j++) {
        biases.set(0, j, 0.01);
        lastOutput.set(0, j, 0.0);
    }
}

Matrix Layer::forward(const Matrix& input) {
    Matrix result = (input * weights) + biases;

    for(int j = 0; j < result.cols; j++) {
        double val = result.get(0, j);
        result.set(0, j, sigmoid(val));
    }

    lastOutput = result;
    return result;
}

vector<double> Layer::getActivations() const {
    vector<double> activations;
    for(int j = 0; j < lastOutput.cols; j++) {
        activations.push_back(lastOutput.get(0, j));
    }
    return activations;
}


void Layer::printLayerInfo() const {
    cout << "Layer Info - Weights: " << weights.rows << "x" << weights.cols
         << ", Biases: " << biases.cols << ", Last Output: " << lastOutput.cols << endl;
}