#include "Utils.h"

namespace ML {

#ifndef ZEDBOARD
// --- Argument Parsing ---
const char* argp_program_version =
    "Machine Learning Model Framework (CprE 487/587 @ ISU)\n\tAuthor: Matthew Dwyer (dwyer@iastate.edu)\n\tVersion: 0.1\n\tDate: August 28th, 2022";
const char* argp_program_bug_address = "<dwyer@iastate.edu>";
static char doc[] = "The ML model framework for CprE 487/587 @ ISU";
static char args_doc[] = "ml [options] baseFilePath";

static struct argp_option options[] = {{0, 0, 0, 0, "Valid options:", -1},
                                       {"verify", 'v', 0, OPTION_ARG_OPTIONAL, "Verify layer outputs"},
                                       {"single", 's', "singleLayer", 0, "Run a single layer"},
                                       {"debug", 'g', 0, OPTION_ARG_OPTIONAL, "Produce debug output"},
                                       {0, 'd', 0, OPTION_ALIAS, "Produce debug output"},
                                       {0}};

// Define out parser
static error_t parse_opt(int arg_key, char* arg_val, struct argp_state* arg_state);
static struct argp argp = {options, parse_opt, args_doc, doc};

static error_t parse_opt(int arg_key, char* arg_val, struct argp_state* arg_state) {
    struct arguments* args = (arguments*)arg_state->input;

    switch (arg_key) {
    case 'd':
        args->debug = true;
        break;
    case 'g':
        args->debug = true;
        break;
    case 'v':
        args->verify = true;
        break;
    case 's':
        args->singleLayer = true;
        args->layerNum = std::stoi(arg_val);
        break;
    // Additional values given
    case ARGP_KEY_ARG:
        // args->basePath = std::string(arg_val);
        break;
    case ARGP_KEY_END:
        if (args->singleLayer && args->layerNum < 0) argp_failure(arg_state, 1, 0, "You must specify a layer number to run");
        // else if (args->basePath.empty()) argp_failure(arg_state, 1, 0, "No input data base path given");
        break;

    default:
        /// Unknown arg
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

// Parse the args with argp
void Args::parseArgs(int argc, char** argv) {
    // Set default args
    _args.debug = false;
    _args.verify = false;
    _args.singleLayer = false;
    _args.layerNum = -1;
    _args.basePath = (char*)"";

    // Parse the input
    argp_parse(&argp, argc, argv, 0, 0, &_args);
    this->debug = _args.debug;
    this->verify = _args.verify;
    this->singleLayer = _args.singleLayer;
    this->layerNum = _args.layerNum;
    this->basePath = _args.basePath;
    this->version = std::string(argp_program_version) + "\n";
}
#endif

}  // namespace ML