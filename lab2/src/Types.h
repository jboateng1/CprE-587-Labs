#pragma once

#include <cstdint>
#include <type_traits>
#include <vector>

namespace ML {

// --- Data Types ---
using size = std::size_t;
using dimVec = std::vector<std::size_t>;

// Signed int
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

// Unsigned int
using ui8 = std::uint8_t;
using ui16 = std::uint16_t;
using ui32 = std::uint32_t;
using ui64 = std::uint64_t;

// Floating point
using fp32 = float;
using fp64 = double;
using fp96 = long double;

// --- Array Types ---
// 1D Array
template <typename T> using Array1D = T*;

// Signed int
using Array1D_i8 = Array1D<i8>;
using Array1D_i16 = Array1D<i16>;
using Array1D_i32 = Array1D<i32>;
using Array1D_i64 = Array1D<i64>;

// Unsigned int
using Array1D_ui8 = Array1D<ui8>;
using Array1D_ui16 = Array1D<ui16>;
using Array1D_ui32 = Array1D<ui32>;
using Array1D_ui64 = Array1D<ui64>;

// Floating point
using Array1D_fp32 = Array1D<fp32>;
using Array1D_fp64 = Array1D<fp64>;
using Array1D_fp96 = Array1D<fp96>;

// 2D Array
template <typename T> using Array2D = T**;

// Signed int
using Array2D_i8 = Array2D<i8>;
using Array2D_i16 = Array2D<i16>;
using Array2D_i32 = Array2D<i32>;
using Array2D_i64 = Array2D<i64>;

// Unsigned int
using Array2D_ui8 = Array2D<ui8>;
using Array2D_ui16 = Array2D<ui16>;
using Array2D_ui32 = Array2D<ui32>;
using Array2D_ui64 = Array2D<ui64>;

// Floating point
using Array2D_fp32 = Array2D<fp32>;
using Array2D_fp64 = Array2D<fp64>;
using Array2D_fp96 = Array2D<fp96>;

// 3D Array
template <typename T> using Array3D = T***;

// Signed int
using Array3D_i8 = Array3D<i8>;
using Array3D_i16 = Array3D<i16>;
using Array3D_i32 = Array3D<i32>;
using Array3D_i64 = Array3D<i64>;

// Unsigned int
using Array3D_ui8 = Array3D<ui8>;
using Array3D_ui16 = Array3D<ui16>;
using Array3D_ui32 = Array3D<ui32>;
using Array3D_ui64 = Array3D<ui64>;

// Floating point
using Array3D_fp32 = Array3D<fp32>;
using Array3D_fp64 = Array3D<fp64>;
using Array3D_fp96 = Array3D<fp96>;

// 4D Array
template <typename T> using Array4D = T****;

// Signed int
using Array4D_i8 = Array4D<i8>;
using Array4D_i16 = Array4D<i16>;
using Array4D_i32 = Array4D<i32>;
using Array4D_i64 = Array4D<i64>;

// Unsigned int
using Array4D_ui8 = Array4D<ui8>;
using Array4D_ui16 = Array4D<ui16>;
using Array4D_ui32 = Array4D<ui32>;
using Array4D_ui64 = Array4D<ui64>;

// Floating point
using Array4D_fp32 = Array4D<fp32>;
using Array4D_fp64 = Array4D<fp64>;
using Array4D_fp96 = Array4D<fp96>;

// 5D Array
template <typename T> using Array5D = T*****;

// Signed int
using Array5D_i8 = Array5D<i8>;
using Array5D_i16 = Array5D<i16>;
using Array5D_i32 = Array5D<i32>;
using Array5D_i64 = Array5D<i64>;

// Unsigned int
using Array5D_ui8 = Array5D<ui8>;
using Array5D_ui16 = Array5D<ui16>;
using Array5D_ui32 = Array5D<ui32>;
using Array5D_ui64 = Array5D<ui64>;

// Floating point
using Array5D_fp32 = Array5D<fp32>;
using Array5D_fp64 = Array5D<fp64>;
using Array5D_fp96 = Array5D<fp96>;

}  // namespace ML