#include "MaxPooling.h"

#include <iostream>

#include "../Types.h"
#include "../Utils.h"
#include "Layer.h"

namespace ML {
// --- Begin Student Code ---

fp32 maxim(fp32 num0, fp32 num1, fp32 num2, fp32 num3) {
    fp32 max = num0;

    if (num1 > max) {
        max = num1;
    }
    if (num2 > max) {
        max = num2;
    }
    if (num3 > max) {
        max = num3;
    }

    return max;
}

// Compute the convultion for the layer data
void MaxPoolingLayer::computeNaive(const LayerData& dataIn) const {

    // Define dismensions

    Array3D_fp32 outData = this->getOutputData().getData<Array3D_fp32>();
    Array3D_fp32 dataInData = dataIn.getData<Array3D_fp32>();

    int in_channel_c = dataIn.getParams().dims[2]; // Number of input channels
    std::cout << "print C: "<<in_channel_c<<"\n";
   
    int ofmap_p = this->getOutputData().getParams().dims[0]; // Ofmap spatial height
    int ofmap_q = this->getOutputData().getParams().dims[1]; // Ofmap spatial width
    std::cout << "print P: "<<ofmap_p<<"\n";
    std::cout << "print Q: "<<ofmap_q<<"\n";

    //int stride_u = 1;

    // TODO Algorithm Goes Here
    for (int c = 0; c < in_channel_c; c++) {
        for (int p = 0; p < ofmap_p; p++) {
            for (int q = 0; q < ofmap_q; q++) {
                //std::cout << "print index: "<<c<<" "<<p<<" "<<q<<"\n";
                outData[p][q][c] = maxim(dataInData[2*p+1][2*q+1][c], dataInData[2*p+1][2*q][c], dataInData[2*p][2*q+1][c], dataInData[2*p][2*q][c]);
                //std::cout << outData[p][q][c];
            }
        }
    }
}

// Compute the convolution using threads
void MaxPoolingLayer::computeThreaded(const LayerData& dataIn) const {
    computeNaive(dataIn);
}

// Compute the convolution using a tiled approach
void MaxPoolingLayer::computeTiled(const LayerData& dataIn) const {
    computeNaive(dataIn);
}

// Compute the convolution using SIMD
void MaxPoolingLayer::computeSIMD(const LayerData& dataIn) const {
    computeNaive(dataIn);
}
}  // namespace ML
