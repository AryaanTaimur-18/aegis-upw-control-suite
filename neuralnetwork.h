#ifndef NEURALNETWORK_H
#define NEURALNETWORK_H

#include "Layer.h"
#include <vector>

class NeuralNetwork {
private:
    std::vector<Layer> layers; // Composition: NN has multiple layers

public:
    // Add a new layer to the brain
    void addLayer(int inputs, int neurons) {
        layers.push_back(Layer(inputs, neurons));
    }

    // Forward pass through the WHOLE network
    Matrix predict(Matrix input) {
        Matrix currentOutput = input;
        for (Layer& layer : layers) {
            currentOutput = layer.forward(currentOutput);
        }
        return currentOutput;
    }

    // Extract activation values from all layers for Neural Glow visualization
    std::vector<std::vector<double>> getNetworkTelemetry() const {
        std::vector<std::vector<double>> telemetry;
        for (const Layer& layer : layers) {
            telemetry.push_back(layer.getActivations());
        }
        return telemetry;
    }
};

#endif