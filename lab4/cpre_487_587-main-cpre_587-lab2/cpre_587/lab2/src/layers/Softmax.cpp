#include "Softmax.h"

#include <iostream>

#include "../Types.h"
#include "../Utils.h"
#include "Layer.h"

namespace ML {
// --- Begin Student Code ---

// Compute the convultion for the layer data
void SoftmaxLayer::computeNaive(const LayerData& dataIn) const {

    // Define dismensions
    
    Array3D_fp32 outData = this->getOutputData().getData<Array3D_fp32>();
    Array3D_fp32 dataInData = dataIn.getData<Array3D_fp32>();

    int in_channel_c = dataIn.getParams().dims[2]; // Number of input channels

    //int stride_u = 1;

    // TODO Algorithm Goes Here
    fp32 sum = 0.0;
    fp32 maximum = -INFINITY;

    for (int i = 0; i < in_channel_c; i++) {
        if (maximum < dataInData[0][0][i]) {
            maximum = dataInData[0][0][i];
        }
    }

    for (int i = 0; i < in_channel_c; i++) {
        outData[0][0][i] = std::exp(dataInData[0][0][i] - maximum);
        sum += outData[0][0][i];
    }

    for (int i = 0; i < in_channel_c; i++) {
        outData[0][0][i] /= sum;
    }

}

// Compute the convolution using threads
void SoftmaxLayer::computeThreaded(const LayerData& dataIn) const {
    computeNaive(dataIn);
}

// Compute the convolution using a tiled approach
void SoftmaxLayer::computeTiled(const LayerData& dataIn) const {
    computeNaive(dataIn);
}

// Compute the convolution using SIMD
void SoftmaxLayer::computeSIMD(const LayerData& dataIn) const {
    computeNaive(dataIn);
}
}  // namespace ML
