#include <experimental/filesystem>
#include <iostream>
#include <sstream>
#include <vector>

#include "Config.h"
#include "Model.h"
#include "Types.h"
#include "Utils.h"
#include "layers/Convolutional.h"
#include "layers/Dense.h"
#include "layers/Layer.h"
#include "layers/MaxPooling.h"
#include "layers/Softmax.h"

#ifdef ZEDBOARD
#    include "ML.h"
namespace ML {
#else
using namespace ML;
#endif

// Make our code a bit cleaner
namespace fs = std::experimental::filesystem;

// Build our ML toy model
Model buildToyModel(const fs::path modelPath) {
    Model model;
    logInfo("--- Building Toy Model ---");

    // --- Conv 1: L1 ---
    // Input shape: 64x64x3
    // Output shape: 60x60x32

    // You can pick how you want to implement your layers, both are allowed:

    // LayerParams conv1_inDataParam(sizeof(fp32), {64, 64, 3});
    // LayerParams conv1_outDataParam(sizeof(fp32), {60, 60, 32});
    // LayerParams conv1_weightParam(sizeof(fp32), {5, 5, 3, 32}, modelPath / "conv1_weights.bin");
    // LayerParams conv1_biasParam(sizeof(fp32), {32}, modelPath / "conv1_biases.bin");
    // auto conv1 = new ConvolutionalLayer(conv1_inDataParam, conv1_outDataParam, conv1_weightParam, conv1_biasParam);

    auto conv1 = new ConvolutionalLayer({{sizeof(fp32), {64, 64, 3}},                                     // Input Data
                                         {sizeof(fp32), {60, 60, 32}},                                    // Output Data
                                         {sizeof(fp32), {5, 5, 3, 32}, modelPath / "conv1_weights.bin"},  // Weights
                                         {sizeof(fp32), {32}, modelPath / "conv1_biases.bin"}});          // Bias
    model.addLayer(conv1);

    // --- Conv 2: L2 ---
    // Input shape: 60x60x32
    // Output shape: 56x56x32

    // --- MPL 1: L3 ---
    // Input shape: 56x56x32
    // Output shape: 28x28x32

    // --- Conv 3: L4 ---
    // Input shape: 28x28x32
    // Output shape: 26x26x64

    // --- Conv 4: L5 ---
    // Input shape: 26x26x64
    // Output shape: 24x24x64

    // --- MPL 2: L6 ---
    // Input shape: 24x24x64
    // Output shape: 12x12x64

    // --- Conv 5: L7 ---
    // Input shape: 12x12x64
    // Output shape: 10x10x64

    // --- Conv 6: L8 ---
    // Input shape: 10x10x64
    // Output shape: 8x8x128

    // --- MPL 3: L9 ---
    // Input shape: 8x8x128
    // Output shape: 4x4x128

    // --- Flatten 1: L10 ---
    // Input shape: 4x4x128
    // Output shape: 2048

    // --- Dense 1: L11 ---
    // Input shape: 2048
    // Output shape: 256

    // --- Dense 2: L12 ---
    // Input shape: 256
    // Output shape: 200

    // --- Softmax 1: L13 ---
    // Input shape: 200
    // Output shape: 200

    return model;
}

void runBasicTest(const Model& model, const fs::path& basePath) {
    logInfo("--- Running Basic Test ---");

    // Load an image
    fs::path imgPath("./data/image_0.bin");
    dimVec dims = {64, 64, 3};
    Array3D_fp32 img = loadArray<Array3D_fp32>(imgPath, dims);

    // Compare images
    std::cout << "Comparing image 0 to itself (max error): " << compareArray<Array3D_fp32>(img, img, dims) << std::endl
              << "Comparing image 0 to itself (T/F within epsilon " << ML::Config::EPSILON << "): " << std::boolalpha
              << compareArrayWithin<Array3D_fp32>(img, img, dims, ML::Config::EPSILON) << std::endl;

    // Test again with a modified copy
    std::cout << "\nChange a value by 0.1 and compare again" << std::endl;
    Array3D_fp32 imgCopy = allocArray<Array3D_fp32>(dims);
    copyArray<Array3D_fp32>(img, imgCopy, dims);
    imgCopy[0][0][0] += 0.1;

    // Compare images
    compareArrayWithinPrint(img, imgCopy, dims);

    // Test again with a modified copy
    log("Change a value by 0.1 and compare again...");
    imgCopy[0][0][0] += 0.1;

    // Compare Images
    compareArrayWithinPrint(img, imgCopy, dims);

    // Clean up after ourselves
    freeArray<Array3D_fp32>(img, dims);
    freeArray<Array3D_fp32>(imgCopy, dims);
}

void runLayerTest(const std::size_t layerNum, const Model& model, const fs::path& basePath) {
    // Load an image
    logInfo("--- Running Infrence Test ---");
    dimVec inDims = {64, 64, 3};

    // Construct a LayerData object from a LayerParams one
    LayerData img({sizeof(fp32), inDims, basePath / "image_0.bin"});
    img.loadData<Array3D_fp32>();

    // Run infrence on the model
    const LayerData output = model.infrenceLayer(img, layerNum, Layer::InfType::NAIVE);

    // Compare the output
    // Construct a LayerData object from a LayerParams one
    dimVec outDims = model[layerNum]->getOutputParams().dims;
    LayerData expected({sizeof(fp32), outDims, basePath / "image_0_data" / "layer_0_output.bin"});
    expected.loadData<Array3D_fp32>();
    output.compareWithinPrint<Array3D_fp32>(expected);
}

void runInfrenceTest(const Model& model, const fs::path& basePath) {
    // Load an image
    logInfo("--- Running Infrence Test ---");
    dimVec inDims = {64, 64, 3};

    // Construct a LayerData object from a LayerParams one
    LayerData img({sizeof(fp32), inDims, basePath / "image_0.bin"});
    img.loadData<Array3D_fp32>();

    // Run infrence on the model
    const LayerData output = model.infrence(img, Layer::InfType::NAIVE);

    // Compare the output
    // Construct a LayerData object from a LayerParams one
    dimVec outDims = model.getOutputLayer()->getOutputParams().dims;
    LayerData expected({sizeof(fp32), outDims, basePath / "image_0_data" / "layer_0_output.bin"});
    expected.loadData<Array3D_fp32>();
    output.compareWithinPrint<Array3D_fp32>(expected);
}

// clang-format off
#ifdef ZEDBOARD
int runModelTest(){
#else
int main(int argc, char** argv) {
    // Hanlde command line arguments
    Args& args = Args::getInst();
    args.parseArgs(argc, argv);
#endif

    // Base input data path (determined from current directory of where you are running the command)
    fs::path basePath("data");  // May need to be altered for zedboards loading from SD Cards

    // Build the model and allocate the buffers
    Model model = buildToyModel(basePath / "model");
    model.allocLayers<fp32>();

    // Run some framework tests as an example of loading data
    runBasicTest(model, basePath);

    // Run a layer infrence test
    runLayerTest(0, model, basePath);

    // Run an end-to-end infrence test
    runInfrenceTest(model, basePath);

    // Clean up
    model.freeLayers<fp32>();

    return 0;
}
// clang-format on

#ifdef ZEDBOARD
}
;  // namespace ML;
#endif
