#pragma once
#include <vector>

#include "layers/Convolutional.h"
#include "layers/Dense.h"
#include "layers/Layer.h"
#include "layers/MaxPooling.h"
#include "layers/Softmax.h"

namespace ML {
class Model {
   public:
    // Constructors
    Model() : layers() {}  //, checkFinal(true), checkEachLayer(false) {}

    // Functions
    const LayerData& infrence(const LayerData& inData, const Layer::InfType infType = Layer::InfType::NAIVE) const;
    const LayerData& infrenceLayer(const LayerData& inData, const int layerNum, const Layer::InfType infType = Layer::InfType::NAIVE) const;

    // Internal memory management
    // Allocate the internal output buffers for each layer in the model
    template <typename T> void allocLayers();

    // Free all layers
    template <typename T> void freeLayers();

    // Getter Functions
    const std::size_t getNumLayers() const { return layers.size(); }

    // Add a layer to the model
    void addLayer(Layer* l) { layers.push_back(l); }

    // Insert a layer into the model
    void insertLayer(Layer* l, std::size_t idx) { layers.insert(layers.begin() + idx, l); }

    // Remove a layer from the model
    void removeLayer(const std::size_t idx) { layers.erase(layers.begin() + idx); }

    // Get layer from the model
    Layer*& getLayer(const std::size_t idx) { return layers[idx]; }
    const Layer* getLayer(const std::size_t idx) const { return layers[idx]; }

    // Get the last layer from the model
    Layer*& getOutputLayer() { return layers[layers.size() - 1]; }
    const Layer* getOutputLayer() const { return layers[layers.size() - 1]; }

    // Array operator (get the layer index)
    Layer*& operator[](const std::size_t idx) { return layers[idx]; }
    const Layer* operator[](const std::size_t idx) const { return layers[idx]; }

    // Call operators (run infrence)
    const LayerData& operator()(const LayerData& inData, const Layer::InfType infType = Layer::InfType::NAIVE) const;
    const LayerData& operator()(const LayerData& inData, const int layerNum, const Layer::InfType infType = Layer::InfType::NAIVE) const;

   private:
    std::vector<Layer*> layers;
};

// Allocate the internal output buffers for each layer in the model
template <typename T> void Model::allocLayers() {
    for (std::size_t i = 0; i < layers.size(); i++) {
        // Virtual templated functions are not allowed, so we have this
        switch (layers[i]->getLType()) {
        case Layer::LayerType::CONVOLUTIONAL:
            ((ConvolutionalLayer*)layers[i])->allocateLayer<T>();
            break;
        case Layer::LayerType::DENSE:
            assert(false && "Cannot allocate unimplemented layer");
        //     ((DenseLayer*) layers[i])->allocateLayer<T>();
        //     break;
        case Layer::LayerType::SOFTMAX:
            assert(false && "Cannot allocate unimplemented layer");
        //     ((SoftmaxLayer*) layers[i])->allocateLayer<T>();
        //     break;
        case Layer::LayerType::MAX_POOLING:
            assert(false && "Cannot allocate unimplemented layer");
        //     ((MaxPoolingLayer*) layers[i])->allocateLayer<T>();
        //     break;
        case Layer::LayerType::NONE:
            [[fallthrough]];
        default:
            assert(false && "Cannot allocate layer of type none");
            break;
        }
    }
}

// Free all layers in the model
template <typename T> void Model::freeLayers() {
    // Free all of the layer buffers first
    // Free the internal output buffers for each layer in the model
    for (std::size_t i = 0; i < layers.size(); i++) {
        // Virtual templated functions are not allowed, so we have this
        switch (layers[i]->getLType()) {
        case Layer::LayerType::CONVOLUTIONAL:
            ((ConvolutionalLayer*)layers[i])->freeLayer<T>();
            break;
        case Layer::LayerType::DENSE:
        //     ((DenseLayer*) layers[i])->freeLayer<T>();
        //     break;
        case Layer::LayerType::SOFTMAX:
        //     ((SoftmaxLayer*) layers[i])->freeLayer<T>();
        //     break;
        case Layer::LayerType::MAX_POOLING:
        //     ((MaxPoolingLayer*) layers[i])->freeLayer<T>();
        //     break;
        case Layer::LayerType::NONE:
            [[fallthrough]];
        default:
            assert(false && "Cannot clear layer of type none");
            break;
        }
    }

    // Free layer pointers
    for (std::size_t i = 0; i < layers.size(); i++) {
        delete layers[i];
    }

    // Remove the dangeling pointers from the array
    layers.clear();
}
}  // namespace ML
