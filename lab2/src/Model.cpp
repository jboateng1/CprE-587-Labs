#include "Model.h"

#include <cassert>

namespace ML {

// Run infrence on the entire model using the inData and outputting the outData
// infType can be used to determine the infrence function to call
const LayerData& Model::infrence(const LayerData& inData, const Layer::InfType infType) const {
    assert(layers.size() > 0 && "There must be at least 1 layer to perform infrence");
    infrenceLayer(inData, 0, infType);

    for (std::size_t i = 1; i < layers.size(); i++) {
        infrenceLayer(layers[i - 1]->getOutputData(), i, infType);
    }

    return layers.back()->getOutputData();
}

// Run infrence on a single layer of the model using the inData and outputting the outData
// infType can be used to determine the infrence function to call
const LayerData& Model::infrenceLayer(const LayerData& inData, const int layerNum, const Layer::InfType infType) const {
    Layer& layer = *layers[layerNum];

    assert(layer.getInputParams().isCompatible(inData.getParams()) && "Input data is not compatible with layer");
    assert(layer.isOutputBufferAlloced() && "Output buffer must be allocated prior to infrence");

    switch (infType) {
    case Layer::InfType::NAIVE:
        layer.computeNaive(inData);
        break;
    case Layer::InfType::THREADED:
        layer.computeThreaded(inData);
        break;
    case Layer::InfType::TILED:
        layer.computeTiled(inData);
        break;
    case Layer::InfType::SIMD:
        layer.computeSIMD(inData);
        break;
    default:
        assert(false && "Infrence Type not implemented");
    }

    return layer.getOutputData();
}

}  // namespace ML
