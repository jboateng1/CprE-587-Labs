#include "Flatten.h"

#include <iostream>

#include "../Types.h"
#include "../Utils.h"
#include "Layer.h"

namespace ML {
// --- Begin Student Code ---

// Compute the convultion for the layer data
void FlattenLayer::computeNaive(const LayerData& dataIn) const {

    // Define dismensions

    Array3D_fp32 outData = this->getOutputData().getData<Array3D_fp32>();
    Array3D_fp32 dataInData = dataIn.getData<Array3D_fp32>();

    int in_channel_c = dataIn.getParams().dims[2]; // Number of input channels
    std::cout << "print C: "<<in_channel_c<<"\n";

    int ifmap_h = dataIn.getParams().dims[0]; // Ifmap spatial height
    int ifmap_w = dataIn.getParams().dims[1]; // Ifmap spatial width
    std::cout << "print P: "<<ifmap_h<<"\n";
    std::cout << "print Q: "<<ifmap_w<<"\n";

    int ofmap_p = this->getOutputData().getParams().dims[0]; // Ofmap spatial height
    std::cout << "print P: "<<ofmap_p<<"\n";

    // TODO Algorithm Goes Here
    int i = 0;

    for (int h = 0; h < ifmap_h; h++) {
        for (int w = 0; w < ifmap_w; w++) {
            for (int c = 0; c < in_channel_c; c++) {
                fp32 in = dataInData[h][w][c];
                outData[0][0][i] = in;
                i++;
            }
        }
    }


}

// Compute the convolution using threads
void FlattenLayer::computeThreaded(const LayerData& dataIn) const {
    computeNaive(dataIn);
}

// Compute the convolution using a tiled approach
void FlattenLayer::computeTiled(const LayerData& dataIn) const {
    computeNaive(dataIn);
}

// Compute the convolution using SIMD
void FlattenLayer::computeSIMD(const LayerData& dataIn) const {
    computeNaive(dataIn);
}
}  // namespace ML
