#pragma once

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "Config.h"

// Check for string formatting support
#if __has_include(<format>)
#    include <format>
#    include <string_view>
using std::format;
using std::make_format_args;
#elif __has_include(<fmt/core.h>)
//#define FMT_HEADER_ONLY
#    include <fmt/core.h>
using fmt::make_format_args;
using fmt::vformat;
#else
#    define STR_FORMAT_NONE
#endif

#ifdef ZEDBOARD
#    include "xilsd.h"
#else
#    include <argp.h>
#endif

namespace ML {

// --- Color Logging Codes ---
enum CCode {
    RESET = 0,
    BRIGHT = 1,
    UNDERLINE = 4,
    INVERSE = 7,
    BRIGHT_OFF = 21,
    UNDERLINE_OFF = 24,
    INVERSE_OFF = 27,

    FG_BLACK = 30,
    FG_RED = 31,
    FG_GREEN = 32,
    FG_YELLOW = 33,
    FG_BLUE = 34,
    FG_MAGENTA = 35,
    FG_CYAN = 36,
    FG_WHITE = 37,
    FG_DEFAULT = 39,

    BG_BLACK = 40,
    BG_RED = 41,
    BG_GREEN = 42,
    BG_YELLOW = 43,
    BG_BLUE = 44,
    BG_MAGENTA = 45,
    BG_CYAN = 46,
    BG_WHITE = 47,
    BG_DEFAULT = 49
};

// --- Logging helper ---
class LogMod {
   private:
    CCode code;

   public:
    LogMod(CCode pCode) : code(pCode) {}
    friend std::ostream& operator<<(std::ostream& os, const LogMod& mod) { return os << "\033[" << mod.code << "m"; }
};

// TODO: Use fmt lib formatting for colors and other styles instead if available
// Log a non-decorated message
#ifndef STR_FORMAT_NONE
template <typename... Args> inline void log(const std::string_view& fmt_str, Args&&... args) {
    std::string msg = vformat(fmt_str, make_format_args(args...));
#else
template <typename... Args> inline void log(const std::string& msg, Args&&... args) {
#endif
    if (Config::FANCY_LOGGING)
        std::cout << msg << std::endl;
    else
        std::cout << msg << std::endl;
}

// Log a info Message
#ifndef STR_FORMAT_NONE
template <typename... Args> inline void logInfo(const std::string_view& fmt_str, Args&&... args) {
    std::string msg = vformat(fmt_str, make_format_args(args...));
#else
template <typename... Args> inline void logInfo(const std::string& msg, Args&&... args) {
#endif
    if (Config::FANCY_LOGGING)
        std::cout << "[" << LogMod(CCode::FG_CYAN) << "Info" << LogMod(CCode::FG_DEFAULT) << "]: " << msg << std::endl;
    else
        std::cout << "[Info]: " << msg << std::endl;
}

// Log a debug Message
#ifndef STR_FORMAT_NONE
template <typename... Args> inline void logDebug(const std::string_view& fmt_str, Args&&... args) {
    std::string msg = vformat(fmt_str, make_format_args(args...));
#else
template <typename... Args> inline void logDebug(const std::string& msg, Args&&... args) {
#endif
    if (Config::FANCY_LOGGING)
        std::cout << "[" << LogMod(CCode::FG_GREEN) << "Debug" << LogMod(CCode::FG_DEFAULT) << "]: " << msg << std::endl;
    else
        std::cout << "[Debug]: " << msg << std::endl;
}

// Log a warning Message
#ifndef STR_FORMAT_NONE
template <typename... Args> inline void logWarn(const std::string_view& fmt_str, Args&&... args) {
    std::string msg = vformat(fmt_str, make_format_args(args...));
#else
template <typename... Args> inline void logWarn(cosnt std::string& msg, Args&&... args) {
#endif
    if (Config::FANCY_LOGGING)
        std::cout << "[" << LogMod(CCode::FG_YELLOW) << "Warning" << LogMod(CCode::FG_DEFAULT) << "]: " << msg << std::endl;
    else
        std::cout << "[Warning]: " << msg << std::endl;
}

// Log an error Message
#ifndef STR_FORMAT_NONE
template <typename... Args> inline void logError(const std::string_view& fmt_str, Args&&... args) {
    std::string msg = vformat(fmt_str, make_format_args(args...));
#else
template <typename... Args> inline void logError(const std::string& msg, Args&&... args) {
#endif
    if (Config::FANCY_LOGGING)
        std::cerr << "[" << LogMod(CCode::FG_RED) << "Error" << LogMod(CCode::FG_DEFAULT) << "]: " << msg << std::endl;
    else
        std::cerr << "[Error]: " << msg << std::endl;
}

// --- Timing Functions ---
class Timer {
   public:
    std::chrono::time_point<std::chrono::steady_clock> begin, end;
    long int elapsed;
    float miliseconds;

    Timer() {}
    ~Timer() {}

    // Start the Timer
    void start() {
#ifndef DISABLE_TIMING
        begin = std::chrono::steady_clock::now();
#endif
    }

    // Stop the Timer
    void stop() {
#ifndef DISABLE_TIMING
        end = std::chrono::steady_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
        miliseconds = elapsed / 1000.0f;
#endif
    }

    // Print the Time with a custom message
    void print(const std::string& msg) {
#ifndef DISABLE_TIMING
        std::string timeMsg = "Timer: " + msg + "; elapsed=" + std::to_string(miliseconds) + "ms";
        log(timeMsg);
#endif
    }

    // Print without custom message
    void print() {
#ifndef DISABLE_TIMING
        print("");
#endif
    }
};

class FuncTimer {
   public:
    const std::string fn, title;
    const std::chrono::time_point<std::chrono::steady_clock> start;
    FuncTimer(const std::string& fn, const std::string& title) : fn(std::move(fn)), title(std::move(title)), start(std::chrono::steady_clock::now()) {}
    ~FuncTimer() {
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
        std::string msg = title;
        logInfo("%s: function=%s; elasepd=%f ms\n", title.c_str(), fn.c_str(), elapsed / 1000.0f);
    }
};

#ifndef DISABLE_TIMING
static constexpr inline void dummy_fn() {}
#    define START_FUNC_TIMER(...) dummy_fn()
#else
#    define START_FUNC_TIMER(title) FuncTimer timer(__FUNCTION__, title)
#endif

#define START_TIMER() \
    {                 \
        const std::chrono::time_point<std::chrono::steady_clock> __timerStart;
#define END_TIMER() \
    {               \
        const std::chrono::time_point<std::chrono::steady_clock> __timerStart;

template <typename T_FUNC, typename... Args> typename std::result_of<T_FUNC && (Args && ...)>::type timeFunc(T_FUNC&& fn, Args&&... args) {
    START_FUNC_TIMER("Timer:");
    return fn(std::forward<Args>(args)...);
}

// --- Data Helper Functions ---

#ifndef ZEDBOARD
// --- Argument Parsing ---
// GCC argument input struct
class arguments {
   public:
    bool debug;
    bool verify;
    bool singleLayer;
    int layerNum;
    std::filesystem::path basePath;
    char* args[1];
};

// Args handling class (singlton)
class Args {
   public:
    Args(Args& other) = delete;
    void operator=(const Args&) = delete;

    static Args& getInst() {
        static Args inst;
        return inst;
    }

    void parseArgs(int argc, char** argv);

   private:
    Args() : debug(false), verify(false), singleLayer(false), layerNum(-1), basePath("") {}

   public:
    // Options
    bool debug;
    bool verify;
    bool singleLayer;
    int layerNum;
    std::filesystem::path basePath;
    std::string version;

   private:
    struct arguments _args;
};
#endif

// Metaprogramming type helpers
template <typename T> struct remove_all_pointers { typedef T type; };

template <typename T> struct remove_all_pointers<T*> { typedef typename remove_all_pointers<T>::type type; };

// --- Implmentation ---

// --- Data Helper Functions ---
// Type cast helpers
template <typename T> inline T castData(const void* data) { return (T)data; }

// Recursive allocator, takes a raw array of dimentsions and the base type for allocation
template <typename T_BASE> T_BASE* allocArray(const std::size_t* dims, const std::size_t dimsLen, const std::size_t dimIndex = 0) {
    static_assert(!std::is_pointer<T_BASE>(), "Cannot allocate pointer type values (arrays)");

    // Recursively allocate multidimentional arrays
    if (dimsLen > 1) {
        T_BASE** data = new T_BASE*[dims[dimIndex]];

        for (std::size_t i = 0; i < dims[dimIndex]; i++) {
            if (dimIndex < (dimsLen - 1)) {
                data[i] = allocArray<T_BASE>(dims, dimsLen, dimIndex + 1);
            } else {
                data[i] = 0;
            }
        }

        return reinterpret_cast<T_BASE*>(data);
        // Handle 1D arrays gracefully
    } else {
        return new T_BASE[dims[dimIndex]];
    }
}

// Take a vector of dims, takes the final array type as a template
template <typename T> inline T allocArray(const std::vector<std::size_t>& dims, const std::size_t dimIndex = 0) {
    static_assert(std::is_pointer<T>(), "Cannot allocate non-pointer values (arrays)");
    // assert(std::rank<T>() == dims.size() && "Array type does not have the same rank as the dims provided");

    typedef typename remove_all_pointers<T>::type T_BASE;
    return reinterpret_cast<T>(allocArray<T_BASE>(dims.data(), dims.size(), dimIndex));
}

// --- Data array deallocation helpers ---
template <typename T_BASE> void freeArray(T_BASE* data, const std::size_t* dims, const std::size_t dimsLen, const std::size_t dimIndex = 0) {
    static_assert(!std::is_pointer<T_BASE>(), "Cannot deallocate non-pointer values (arrays)");

    // Recursively free the multidimentional array as needed
    if (dimsLen > 1) {
        T_BASE** dataCast = reinterpret_cast<T_BASE**>(data);

        for (std::size_t i = 0; i < dims[dimIndex]; i++) {
            if (dimIndex < dimsLen - 2) {
                freeArray<T_BASE>(dataCast[i], dims, dimsLen, dimIndex + 1);
            } else {
                delete[] dataCast[i];
            }
        }
        // Handle 1D Arrays
    } else {
        delete[] data;
    }
}

// Take a vector of dims
template <typename T> inline void freeArray(T data, const std::vector<std::size_t>& dims, const std::size_t dimIndex = 0) {
    static_assert(std::is_pointer<T>(), "Cannot deallocate non-pointer values (arrays)");
    // assert(std::rank<T>() == dims.size() && "Array type does not have the same rank as the dims provided");

    typedef typename remove_all_pointers<T>::type T_BASE;
    freeArray<T_BASE>(reinterpret_cast<T_BASE*>(data), dims.data(), dims.size(), dimIndex);
}

// --- Compare Functions ---
// Compare value within EPSILON
template <typename T_BASE, typename T_EP = float> constexpr inline bool compareWithin(const T_BASE value, const T_EP epsilon = Config::EPSILON) {
    return std::abs(value) <= epsilon;
}

// Primary base compare function. Calls itself recursively as it decends structure
template <typename T_BASE>
float compareArray(const T_BASE* data1, const T_BASE* data2, const std::size_t* dims, const std::size_t dimsLen, const std::size_t dimIndex = 0) {
    static_assert(!std::is_pointer<T_BASE>(), "Cannot compare pointer type values (arrays)");
    float curr_diff = 0.0;
    float max_diff = 0.0;
    const T_BASE** dataCast1 = reinterpret_cast<const T_BASE**>(const_cast<T_BASE*>(data1));
    const T_BASE** dataCast2 = reinterpret_cast<const T_BASE**>(const_cast<T_BASE*>(data2));

    // Recurse as needed into each array
    for (std::size_t i = 0; i < dims[dimIndex]; i++) {
        if (dimIndex < dimsLen - 1) {
            // Recurse to compare another dimention down
            curr_diff = compareArray<T_BASE>(dataCast1[i], dataCast2[i], dims, dimsLen, dimIndex + 1);
        } else {
            // Check the values and get their absolute difference
            curr_diff = (float)fabs(data1[i] - data2[i]);
        }

        // Update our max difference if it is larger
        if (curr_diff > max_diff) {
            max_diff = curr_diff;
        }
    }

    return max_diff;
}

// Compares two LayerData arrays of size N and returns the maximum difference
template <typename T> inline float compareArray(const T data1, const T data2, const std::vector<std::size_t>& dims, const std::size_t dimIndex = 0) {
    // assert(std::rank<T>() == dims.size() && "Array type does not have the same rank as the dims provided");

    typedef typename remove_all_pointers<T>::type T_BASE;
    return compareArray<T_BASE>(reinterpret_cast<T_BASE*>(data1), reinterpret_cast<T_BASE*>(data2), dims.data(), dims.size(), dimIndex);
}

// Performs a compare operation and checks if the max difference is within the provided epsilon
template <typename T, typename EP_T = float>
inline bool compareArrayWithin(const T data1, const T data2, const std::size_t* dims, const std::size_t dimsLen, EP_T epsilon, const std::size_t dimIndex = 0) {
    static_assert(!std::is_pointer<EP_T>(), "Cannot compare with pointer type (arrays) epsilon values");
    // assert(std::rank<T>() == dimsLen && "Array type does not have the same rank as the dims provided");

    typedef typename remove_all_pointers<T>::type T_BASE;
    return !compareWithin(compareArray<T_BASE>(reinterpret_cast<T_BASE*>(data1), reinterpret_cast<T_BASE*>(data2), dims, dimsLen, dimIndex));
}

// Performs a compare operation and checks if the max difference is within the provided epsilon
template <typename T, typename EP_T = float>
inline bool compareArrayWithin(const T data1, const T data2, const std::vector<std::size_t>& dims, EP_T epsilon, const std::size_t dimIndex = 0) {
    static_assert(!std::is_pointer<EP_T>(), "Cannot compare with pointer type (arrays) epsilon values");
    // assert(std::rank<T>() == dims.size() && "Array type does not have the same rank as the dims provided");

    return compareArrayWithin<T, EP_T>(data1, data2, dims.data(), dims.size(), epsilon, dimIndex);
}

template <typename T_ARRAY, typename T_EP = float>
bool compareArrayWithinPrint(const T_ARRAY& array0, const T_ARRAY& array1, const std::vector<std::size_t>& dims, const T_EP epsilon = Config::EPSILON) {
    T_EP max_error = compareArray<T_ARRAY>(array0, array1, dims);
    bool result = compareWithin(max_error, epsilon);

    std::string msg = "Comparing images (max error): ";
    msg += result ? "True" : "False";
    msg += " (";
    msg += std::to_string(max_error);
    msg += ")";
    log(msg);
    return result;
}

// --- File Data Loading ---
// Recursive array loading function that can load an array of data from multiple dimentions from a binary file
template <typename T_BASE>
#ifdef ZEDBOARD
void loadArrayData(FIL* file, T_BASE* values, const std::size_t* dims, const std::size_t dimsLen, const std::size_t dimIndex = 0) {
#else
void loadArrayData(std::ifstream& file, T_BASE* values, const std::size_t* dims, const std::size_t dimsLen, const std::size_t dimIndex = 0) {
#endif
    static_assert(!std::is_pointer<T_BASE>(), "Cannot load pointer type values (arrays)");

    // Read the values and recurse if needed
    if (dimsLen > 1) {
        T_BASE** valuesCast = reinterpret_cast<T_BASE**>(values);

        for (std::size_t i = 0; i < dims[dimIndex]; i++) {
            if (dimIndex < dimsLen - 2) {
                // We do not care about the data pointer returned here since we have that already stored in a array
                loadArrayData<T_BASE>(file, valuesCast[i], dims, dimsLen, dimIndex + 1);
            } else if (
#ifdef ZEDBOARD
                !xilsd_fread(reinterpret_cast<char*>(valuesCast[i]), sizeof(T_BASE), dims[dimIndex + 1], file)
#else
                !file.read(reinterpret_cast<char*>(valuesCast[i]), sizeof(T_BASE) * dims[dimIndex + 1])
#endif
            ) {  // Read our values
                logError("Failed to read data values from file at index");
                assert(false && "Failed to read file data");
            }
        }
        // Handle 1D Arrays
    } else {
        if (
#ifdef ZEDBOARD
            !xilsd_fread(reinterpret_cast<char*>(values), sizeof(T_BASE), dims[dimIndex], file)
#else
            !file.read(reinterpret_cast<char*>(values), sizeof(T_BASE) * dims[dimIndex])
#endif
        ) {  // Read our values
            std::cerr << "Failed to read data values from file" << std::endl;
            assert(false && "Failed to read file data");
        }
    }
}

// Entry point to loading data from a binary file into an array
template <typename T> T loadArray(const std::filesystem::path& filepath, const std::vector<std::size_t>& dims) {
    static_assert(std::is_pointer<T>(), "Cannot load non-pointer values (arrays)");
    // assert(std::rank<T>() == dims.size() && "Array type does not have the same rank as the dims provided");

    // Open our file and check for issues
#ifdef ZEDBOARD
    FIL* file = nullptr;
    if (!xilsd_fopen(file, filepath.c_str()))  // Create and open our file on the SD card
#else
    std::ifstream file(filepath, std::ios::binary);  // Create and open our file
    if (file.is_open())
#endif
    {
        std::cout << "Opening binary file " << filepath << std::endl;
    } else {
        std::cerr << "Failed to open binary file " << filepath << std::endl;
    }

    // Allocate our array
    T values = allocArray<T>(dims);

    // Load the data
    typedef typename remove_all_pointers<T>::type T_BASE;
    loadArrayData<T_BASE>(file, reinterpret_cast<T_BASE*>(values), dims.data(), dims.size());

#ifdef ZEDBOARD
    // Close our file (ifstream deconstructor does this for us)
    xilsd_fclose(file);
#endif

    return values;
}

// --- Copy Helpers ---
// Deep copy an array by allocating a new one of the same size and recursively copying values
template <typename T_BASE>
void copyArray(const T_BASE* array, T_BASE* newArray, const std::size_t* dims, const std::size_t dimsLen, const std::size_t dimIndex = 0) {
    static_assert(!std::is_pointer<T_BASE>(), "Cannot copy pointer type values (arrays)");
    const T_BASE** arrayCast = reinterpret_cast<const T_BASE**>(const_cast<T_BASE*>(array));
    T_BASE** newArrayCast = reinterpret_cast<T_BASE**>(newArray);

    // Copy each value
    for (std::size_t i = 0; i < dims[dimIndex]; i++) {
        if (dimIndex < (dimsLen - 1)) {
            copyArray<T_BASE>(arrayCast[i], newArrayCast[i], dims, dimsLen, dimIndex + 1);
        } else {
            newArray[i] = array[i];
        }
    }
}

// Create a copy of an array, allocate a new array and recurse all values
// Entry point
template <typename T> inline void copyArray(const T array, T newArray, const std::vector<std::size_t>& dims) {
    static_assert(std::is_pointer<T>(), "Cannot copy non-pointer type values (arrays)");
    // assert(std::rank<T>() == dims.size() && "Array type does not have the same rank as the dims provided");

    typedef typename remove_all_pointers<T>::type T_BASE;
    copyArray<T_BASE>(reinterpret_cast<T_BASE*>(array), reinterpret_cast<T_BASE*>(newArray), dims.data(), dims.size(), 0);
}

// Deep copy an array by allocating a new one of the same size and recursively copying values
// Creates a flat desitnation array in memory
template <typename T_BASE>
void copyArrayFlat(const T_BASE* array, T_BASE* newArray, const std::size_t* dims, const std::size_t dimsLen, const std::size_t dimIndex = 0,
                   std::size_t& index = 0) {
    static_assert(!std::is_pointer<T_BASE>(), "Cannot copy pointer type values (arrays)");
    const T_BASE** arrayCast = reinterpret_cast<const T_BASE**>(const_cast<T_BASE*>(array));
    T_BASE** newArrayCast = reinterpret_cast<T_BASE**>(newArray);

    // Copy each value
    for (std::size_t i = 0; i < dims[dimIndex]; i++) {
        if (dimIndex < (dimsLen - 1)) {
            copyArrayFlat<T_BASE>(arrayCast[i], newArray, dims, dimsLen, dimIndex + 1, index);
        } else {
            newArray[index++] = array[i];
        }
    }
}

// Create a copy of an array, allocate a new array and recurse all values
// Creates a flat desitnation array in memory
// Entry point
template <typename T> inline void copyArrayFlat(const T array, typename remove_all_pointers<T>::type* newArray, const std::vector<std::size_t>& dims) {
    static_assert(std::is_pointer<T>(), "Cannot copy non-pointer type values (arrays)");
    // assert(std::rank<T>() == dims.size() && "Array type does not have the same rank as the dims provided");

    typedef typename remove_all_pointers<T>::type T_BASE;
    copyArray<T_BASE>(reinterpret_cast<T_BASE*>(array), newArray, dims.data(), dims.size(), 0, 0);
}

}  // namespace ML
