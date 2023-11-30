#include "Convolutional.h"

#include <iostream>

#include "../Types.h"
#include "../Utils.h"
#include "Layer.h"

namespace ML {
// --- Begin Student Code ---

// Compute the convultion for the layer data
void ConvolutionalLayer::computeNaive(const LayerData& dataIn) const {
    
    
    // Define dismensions

    Array3D_fp32 outData = this->getOutputData().getData<Array3D_fp32>();
    Array3D_fp32 dataInData = dataIn.getData<Array3D_fp32>();
    Array4D_fp32 weights = this->getWeightData().getData<Array4D_fp32>();
    Array1D_fp32 biases = this->getBiasData().getData<Array1D_fp32>();

    int in_channel_c = dataIn.getParams().dims[2]; // Number of input channels
    std::cout << "print C: "<<in_channel_c<<"\n";

    int out_channel_3d_filter_m = this->getOutputData().getParams().dims[2]; // Number of output channels and 3d filters
    std::cout << "print M: "<<out_channel_3d_filter_m<<"\n";

    int filter_r = this->getWeightData().getParams().dims[0]; // Filter spatial height
    int filter_s = this->getWeightData().getParams().dims[1]; // Filter spatial width
    std::cout << "print R: "<<filter_r<<"\n";
    std::cout << "print S: "<<filter_s<<"\n";
   
    int ofmap_p = this->getOutputData().getParams().dims[0]; // Ofmap spatial height
    int ofmap_q = this->getOutputData().getParams().dims[1]; // Ofmap spatial width
    std::cout << "print P: "<<ofmap_p<<"\n";
    std::cout << "print Q: "<<ofmap_q<<"\n";

    //int stride_u = 1;
    
    
    
    for (int p = 0; p < ofmap_p; p++){
        for (int q = 0; q < ofmap_q; q++){
            for (int m = 0; m < out_channel_3d_filter_m; m++){
                fp32 sum = 0.0;
                for (int r = 0; r < filter_r; r++) {
                    for (int s = 0; s < filter_s; s++) {
                        for (int c = 0; c < in_channel_c; c++) {
                            sum += (dataInData[p + r][q + s][c] * weights[r][s][c][m]);
                        }
                    }
                }
                fp32 total = sum + biases[m];

                if (total > 0.0) {
                    outData[p][q][m] = total;
                }
                else {
                    outData[p][q][m] = 0.0;
                }
            }   
        }   
    }   
    
    //std::cout << "Made It!\n";
   
}

// Compute the convolution using threads
void ConvolutionalLayer::computeThreaded(const LayerData& dataIn) const {
    computeNaive(dataIn);
}

// Compute the convolution using a tiled approach
void ConvolutionalLayer::computeTiled(const LayerData& dataIn) const {
    computeNaive(dataIn);
}

// Compute the convolution using SIMD
void ConvolutionalLayer::computeSIMD(const LayerData& dataIn) const {
    computeNaive(dataIn);
}
}  // namespace ML
