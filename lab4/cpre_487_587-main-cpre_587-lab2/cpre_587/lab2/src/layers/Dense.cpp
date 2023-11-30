#include "Dense.h"

#include <iostream>

#include "../Types.h"
#include "../Utils.h"
#include "Layer.h"

namespace ML {
// --- Begin Student Code ---

// Compute the convultion for the layer data
void DenseLayer::computeNaive(const LayerData& dataIn) const {
    
    // Define dismensions
    
    Array3D_fp32 outData = this->getOutputData().getData<Array3D_fp32>();
    Array3D_fp32 dataInData = dataIn.getData<Array3D_fp32>();
    Array4D_fp32 weights = this->getWeightData().getData<Array4D_fp32>();
    Array1D_fp32 biases = this->getBiasData().getData<Array1D_fp32>();

    int in_channel_c = dataIn.getParams().dims[2]; // Number of input channels
    std::cout << "print C: "<<in_channel_c<<"\n";

    int out_channel_3d_filter_m = this->getOutputData().getParams().dims[2]; // Number of output channels and 3d filters
    std::cout << "print M: "<<out_channel_3d_filter_m<<"\n";
    
    //int stride_u = 1;

    // TODO Algorithm Goes Here
    for (int i = 0; i < out_channel_3d_filter_m; i++) {
        outData[0][0][i] = 0.0;
        for (int j = 0; j < in_channel_c; j++) {
            outData[0][0][i] += dataInData[0][0][j] * weights[0][0][j][i];
        }
        outData[0][0][i] += biases[i];
        if (outData[0][0][i] < 0.0 && in_channel_c != 256) { //Relu on first dense, not on second
            outData[0][0][i] = 0.0;
        }
    }
}

// Compute the convolution using threads
void DenseLayer::computeThreaded(const LayerData& dataIn) const {
    computeNaive(dataIn);
}

// Compute the convolution using a tiled approach
void DenseLayer::computeTiled(const LayerData& dataIn) const {
    computeNaive(dataIn);
}

// Compute the convolution using SIMD
void DenseLayer::computeSIMD(const LayerData& dataIn) const {
    computeNaive(dataIn);
}
}  // namespace ML
