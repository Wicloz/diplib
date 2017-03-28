/*
 * DIPlib 3.0
 * This file contains declarations for linear image filters
 *
 * (c)2017, Cris Luengo.
 * Based on original DIPlib code: (c)1995-2014, Delft University of Technology.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#ifndef DIP_LINEAR_H
#define DIP_LINEAR_H

#include "diplib.h"
#include "diplib/boundary.h"


/// \file
/// \brief Declares functions that implement linear filters.


namespace dip {


/// \defgroup linear Linear filters
/// \ingroup filtering
/// \brief Functions that implement linear filters
/// \{

/// \brief Describes a 1D filter
///
/// The weights are in `filter`. The origin is placed either at the index given by `origin`, if it's non-negative, or
/// at index `filter.size() / 2` if `origin` is negative. This location is either the middle pixel if the filter is
/// odd in length, or the pixel to the right of the center if it is even in length:
///
///     filter size is odd :      filter data :  x x x x x        orgin = -1
///                                                  ^
///                                                  0
///
///     filter size is even :     filter data :  x x x x x x      orgin = -1
///                                                    ^
///                                                    0
///     origin specified :        filter data :  x x x x x x      orgin = 1
///                                                ^
///                                                0
///
/// Note that `origin` must be an index to one of the samples in the `filter` array
///
/// `symmetry` indicates the filter shape: `"general"` (or an empty string) indicates no symmetry.
/// `"even"` indicates even symmetry, and `"odd"` indicates odd symmetry. In both cases, the filter represents
/// the left half of the full filter, with the righmost element at the origin (and not repeated). The full filter
/// is thus always odd in size. `"d-even"` and `"d-odd"` are similar, but duplicate the rightmost element, yielding
/// an even-sized filter. The origin for the symmetric filters is handled identically to the general filter case.
///
///     filter array:                a  b  c              array has N elements
///
///     symmetry = "general":        a  b  c              filter size = N
///     symmetry = "even":           a  b  c  b  a        filter size = N + N - 1
///     symmetry = "odd":            a  b  c -b -a        filter size = N + N - 1
///     symmetry = "d-even":         a  b  c  c  b  a     filter size = N + N
///     symmetry = "d-odd":          a  b  c -c -b -a     filter size = N + N
///
/// The convolution is applied to each tensor component separately, which is always the correct behavior for linear
/// filters.
struct DIP_NO_EXPORT OneDimensionalFilter {
   FloatArray filter;            ///< Filter weights
   dip::sint origin = -1;        ///< Origin of the filter if non-negative
   String symmetry = "";         ///< Filter shape: `""` == `"general"`, `"even"`, `"odd"`, `"d-even"` or `"d-odd"`
};

/// \brief An array of 1D filters
using OneDimensionalFilterArray = std::vector< OneDimensionalFilter >;

// TODO: Implement code to separate an image into 1D filters to be applied with SeparableConvolution
DIP_EXPORT OneDimensionalFilterArray SeparateFilter( Image const& filter );

/// \brief Applies a convolution with a filter kernel (PSF) that is separable.
///
/// `filter` is an array with exactly one element for each dimension of `in`. Alternatively, it can have a single
/// element, which will be used unchanged for each dimension. For the dimensions that are not processed (`process` is
/// `false` for those dimensions), the `filter` array can have non-sensical data or a zero-length filter weights array.
/// Any `filter` array that is zero size or the equivalent of `{1}` will not be applied either.
///
/// `boundaryCondition` indicates how the boundary should be expanded in each dimension. See `dip::BoundaryCondition`.
///
/// \see dip::SeparateFilter, dip::GeneralConvolution, dip::ConvolveFT, dip::Framework::Separable
DIP_EXPORT void SeparableConvolution(
      Image const& in,                    ///< Input image
      Image& out,                         ///< Output image
      OneDimensionalFilterArray const& filterArray, ///< The filter
      StringArray const& boundaryCondition = {}, ///< The boundary condition
      BooleanArray process = {}           ///< Which dimensions to process, can be `{}` to indicate all dimensions are to be processed
);
inline Image SeparableConvolution(
      Image const& in,
      OneDimensionalFilterArray const& filter,
      StringArray const& boundaryCondition = {},
      BooleanArray const& process = {}
) {
   Image out;
   SeparableConvolution( in, out, filter, boundaryCondition, process );
   return out;
}

/// \brief Applies a convolution with a filter kernel (PSF) by multiplication in the Fourier domain.
///
/// `filter` is an image, and must be equal in size or smaller than `in`. If both `in` and `filter`
/// are real, `out` will be real too, otherwise it will have a complex type.
///
/// As elsewhere, the origin of `filter` is in the middle of the image, on the pixel to the right of
/// the center in case of an even-sized image.
///
/// If `in` or `filter` is already Fourier transformed, set `inRepresentation` or `filterRepresentation`
/// to `"frequency"` (actually, any string different from `"spatial"` will do). Similarly, if
/// `outRepresentation` is `"frequency"`, the output will not be inverse-transformed, so will be in
/// the frequency domain.
///
/// \see dip::GeneralConvolution, dip::SeparableConvolution
DIP_EXPORT void ConvolveFT(
      Image const& in,
      Image const& filter,
      Image& out,
      String const& inRepresentation = "spatial",
      String const& filterRepresentation = "spatial",
      String const& outRepresentation = "spatial"
);
inline Image ConvolveFT(
      Image const& in,
      Image const& filter,
      String const& inRepresentation = "spatial",
      String const& filterRepresentation = "spatial",
      String const& outRepresentation = "spatial"
) {
   Image out;
   ConvolveFT( in, filter, out, inRepresentation, filterRepresentation, outRepresentation );
   return out;
}

/// \brief Applies a convolution with a filter kernel (PSF) by direct implementation of the convolution sum.
///
/// `filter` is an image, and must be equal in size or smaller than `in`. `filter` must be real-valued.
///
/// As elsewhere, the origin of `filter` is in the middle of the image, on the pixel to the right of
/// the center in case of an even-sized image.
///
/// Note that this is a really expensive way to compute the convolution for any `filter` that has more than a
/// small amount of non-zero values. It is always advantageous to try to separate your filter into a set of 1D
/// filters (see `dip::SeparateFilter` and `dip::SeparableConvolution`). If this is not possible, use
/// `dip::ConvolveFT` with larger filters to compute the convolution in the Fourier domain.
///
/// Also, if all non-zero filter weights have the same value, `dip::Uniform` implements a more efficient
/// algorithm. If `filter` is a binary image, `dip::Uniform` is called.
///
/// `boundaryCondition` indicates how the boundary should be expanded in each dimension. See `dip::BoundaryCondition`.
///
/// \see dip::ConvolveFT, dip::SeparableConvolution, dip::SeparateFilter, dip::Uniform
DIP_EXPORT void GeneralConvolution(
      Image const& in,
      Image const& filter,
      Image& out,
      StringArray const& boundaryCondition = {}
);
inline Image GeneralConvolution(
      Image const& in,
      Image const& filter,
      StringArray const& boundaryCondition = {}
) {
   Image out;
   GeneralConvolution( in, filter, out, boundaryCondition );
   return out;
}

/// \brief Applies a convolution with a kernel with uniform weights, leading to an average (mean) filter.
///
/// The size and shape of the kernel is given by `filterSize` and `filterShape`. `filterShape` can be any
/// of the strings recognized by `dip::PixelTable`: `"rectangular"`, `"elliptic"`, and `"diamond"`. `filterSize`
/// is the diameter of the circle (sphere/hypersphere) in the corresponding metric: \f$L^\infty\f$, \f$L^2\f$,
/// and \f$L^1\f$.
///
/// `boundaryCondition` indicates how the boundary should be expanded in each dimension. See `dip::BoundaryCondition`.
///
/// \see dip::ConvolveFT, dip::SeparableConvolution, dip::GeneralConvolution
DIP_EXPORT void Uniform(
      Image const& in,
      Image& out,
      FloatArray filterSize = { 7 }, // TODO: generalize the dip::StructuringElement class to dip::Filter or something like that, so we can use it here and in many other places
      String const& filterShape = "elliptic",
      StringArray const& boundaryCondition = {}
);
inline Image Uniform(
      Image const& in,
      FloatArray filterSize = { 7 },
      String const& filterShape = "elliptic",
      StringArray const& boundaryCondition = {}
) {
   Image out;
   Uniform( in, out, filterSize, filterShape, boundaryCondition );
   return out;
}

/// \brief Applies a convolution with a kernel with uniform weights, leading to an average (mean) filter.
///
/// The kernel is given by the binary image `neighborhood`. Note that the kernel is not mirrored, as it would
/// be in the convolution, inless `mode` is equal to the string `"convolution"`.
///
/// `boundaryCondition` indicates how the boundary should be expanded in each dimension. See `dip::BoundaryCondition`.
///
/// \see dip::ConvolveFT, dip::SeparableConvolution, dip::GeneralConvolution
DIP_EXPORT void Uniform(
      Image const& in,
      Image const& neighborhood,
      Image& out,
      StringArray const& boundaryCondition = {},
      String const& mode = "" // set to "convolution" to mirror `neighborhood`
);
inline Image Uniform(
      Image const& in,
      Image const& neighborhood,
      StringArray const& boundaryCondition = {},
      String const& mode = "" // set to "convolution" to mirror `neighborhood`
) {
   Image out;
   Uniform( in, neighborhood, out, boundaryCondition, mode );
   return out;
}

/// \brief Finite impulse response implementation of the Gaussian filter and its derivatives
///
/// Convolves the image with a 1D Gaussian kernel along each dimension. For each dimension,
/// provide a value in `sigmas` and `derivativeOrder`. The zeroth-order derivative is a plain
/// smoothing, no derivative is computed. Derivatives with order up to 3 can be computed with
/// this function. For higher-order derivatives, use `dip::GaussFT`.
///
/// The value of sigma determines the smoothing effect. For values smaller than about 0.8, the
/// result is an increasingly poor approximation to the Gaussian filter. Use `dip::GaussFT` for
/// very small sigmas. Conversely, for very large sigmas it is more efficient to use `dip::GaussIIR`,
/// which runs in a constant time with respect to the sigma.
///
/// For the smoothing filter (`derivativeOrder` is 0), the size of the kernel is given by
/// `2 * truncation * sigma + 1`. The default value for `truncation` is 3, which assures a good
/// approximation of the Gaussian kernel without unnecessary expense. It is possible to reduce
/// computation slightly by decreasing this parameter, but it is not recommended. For derivatives,
/// the value of `truncation` is increased by `0.5 * derivativeOrder`.
///
/// `boundaryCondition` indicates how the boundary should be expanded in each dimension. See `dip::BoundaryCondition`.
///
/// Set `process` to false for those dimensions that should not be filtered. Alternatively, set
/// `sigmas` to 0 or a negative value.
///
/// \see dip::Gauss, dip::GaussIIR, dip::GaussFT, dip::Derivative, dip::FiniteDifference, dip::Uniform
DIP_EXPORT void GaussFIR(
      Image const& in,
      Image& out,
      FloatArray sigmas = { 1.0 },
      UnsignedArray derivativeOrder = { 0 },
      StringArray const& boundaryCondition = {},
      BooleanArray process = {},
      dfloat truncation = 3 // truncation gets automatically increased for higher-order derivatives
);
inline Image GaussFIR(
      Image const& in,
      FloatArray const& sigmas = { 1.0 },
      UnsignedArray const& derivativeOrder = { 0 },
      StringArray const& boundaryCondition = {},
      BooleanArray const& process = {},
      dfloat truncation = 3
) {
   Image out;
   GaussFIR( in, out, sigmas, derivativeOrder, boundaryCondition, process, truncation );
   return out;
}

/// \brief Fourier implementation of the Gaussian filter and its derivatives
///
/// Convolves the image with a Gaussian kernel by multiplication in the Fourier domain.
/// For each dimension, provide a value in `sigmas` and `derivativeOrder`. The value of sigma determines
/// the smoothing effect. The zeroth-order derivative is a plain smoothing, no derivative is computed.
///
/// The values of `sigmas` are translated to the Fourier domain, and a Fourier-domain Gaussian is computed.
/// Frequencies above `2 * ( truncation + 0.5 * derivativeOrder ) * FDsigma` are set to 0. It is a relatively
/// minute computational difference if `truncation` were to be infinity, so it is not worth while to try to
/// speed up the operation by decreasing `truncation`.
///
/// Set `process` to false for those dimensions that should not be filtered. Alternatively, set
/// `sigmas` to 0 or a negative value.
///
/// \see dip::Gauss, dip::GaussFIR, dip::GaussIIR, dip::Derivative, dip::FiniteDifference, dip::Uniform
DIP_EXPORT void GaussFT(
      Image const& in,
      Image& out,
      FloatArray sigmas = { 1.0 },
      UnsignedArray derivativeOrder = { 0 },
      BooleanArray process = {},
      dfloat truncation = 3
);
inline Image GaussFT(
      Image const& in,
      FloatArray const& sigmas = { 1.0 },
      UnsignedArray const& derivativeOrder = { 0 },
      BooleanArray const& process = {},
      dfloat truncation = 3
) {
   Image out;
   GaussFT( in, out, sigmas, derivativeOrder, process, truncation );
   return out;
}

/// \brief Infinite impulse response implementation of the Gaussian filter and its derivatives
///
/// Convolves the image with an IIR 1D Gaussian kernel along each dimension. For each dimension,
/// provide a value in `sigmas` and `derivativeOrder`. The zeroth-order derivative is a plain
/// smoothing, no derivative is computed. Derivatives with order up to 4 can be computed with this
/// function. For higher-order derivatives, use `dip::GaussFT`.
///
/// The value of sigma determines the smoothing effect. For smaller values, the result is an
/// increasingly poor approximation to the Gaussian filter. This function is efficient only for
/// very large sigmas.
///
/// `boundaryCondition` indicates how the boundary should be expanded in each dimension. See `dip::BoundaryCondition`.
///
/// Set `process` to false for those dimensions that should not be filtered. Alternatively, set
/// `sigmas` to 0 or a negative value.
///
/// The `filterOrder` and `designMethod` determine how the filter is implemented. By default,
/// `designMethod` is "discrete time fit". This is the method described in van Vliet et al. (1998).
/// `filterOrder` can be between 1 and 5, with 3 producing good results, and increasing order producing
/// better results. When computing derivatives, a higher `filterOrder` is necessary. By default,
/// `filterOrder` is `3 + derivativeOrder`, capped at 5. The alternative `designMethod` is "forward backward".
/// This is the method described in Young and van Vliet (1995). Here `filterOrder` can be between 3 and 5.
///
/// \see dip::Gauss, dip::GaussFIR, dip::GaussFT, dip::Derivative, dip::FiniteDifference, dip::Uniform
///
/// See: I.T. Young and L.J. van Vliet, Recursive implementation of the Gaussian filter, Signal Processing,
/// 44(2):139-151, 1995.
///
/// See: L.J. van Vliet, I.T. Young and P.W. Verbeek, Recursive Gaussian Derivative Filters,
/// in: Proc. 14th Int. Conference on Pattern Recognition, IEEE Computer Society Press, 1998, 509-514.
DIP_EXPORT void GaussIIR(
      Image const& in,
      Image& out,
      FloatArray sigmas = { 1.0 },
      UnsignedArray derivativeOrder = { 0 },
      StringArray const& boundaryCondition = {},
      BooleanArray process = {},
      UnsignedArray filterOrder = {}, // means compute order depending on derivativeOrder.
      String const& designMethod = "", // default is "discrete time fit", alt is "forward backward".
      dfloat truncation = 3 // truncation gets automatically increased for higher-order derivatives
);
inline Image GaussIIR(
      Image const& in,
      FloatArray const& sigmas = { 1.0 },
      UnsignedArray const& derivativeOrder = { 0 },
      StringArray const& boundaryCondition = {},
      BooleanArray const& process = {},
      UnsignedArray const& filterOrder = {},
      String const& designMethod = "",
      dfloat truncation = 3
) {
   Image out;
   GaussIIR( in, out, sigmas, derivativeOrder, boundaryCondition, process, filterOrder, designMethod, truncation );
   return out;
}

/// \brief Convolution with a Gaussian kernel and its derivatives
///
/// Convolves the image with a Gaussian kernel. For each dimension, provide a value in `sigmas` and
/// `derivativeOrder`. The value of sigma determines the smoothing effect. The zeroth-order derivative
/// is a plain smoothing, no derivative is computed.
///
/// How the convolution is computed depends on the value of `method`:
/// - `"FIR"`: Finite inpulse response implementation, see `dip::GaussFIR`.
/// - `"IIR"`: Infinite inpulse response implementation, see `dip::GaussIIR`.
/// - `"FT"`: Fourier domain implementation, see `dip::GaussFT`.
/// - `"best"`: Picks the best method, according to the values of `sigmas` and `derivativeOrder`:
///     - if any `derivativeOrder` is larger than 3, use the FT method,
///     - else if any `sigmas` is smaller than 0.8, use the FT method,
///     - else if any `sigmas` is larger than 10, use the IIR method,
///     - else use the FIR method.
///
/// `boundaryCondition` indicates how the boundary should be expanded in each dimension. See `dip::BoundaryCondition`.
///
/// Set `process` to false for those dimensions that should not be filtered. Alternatively, set
/// `sigmas` to 0 or a negative value.
///
/// \see dip::GaussFIR, dip::GaussFT, dip::GaussIIR, dip::Derivative, dip::FiniteDifference, dip::Uniform
DIP_EXPORT void Gauss(
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 },
      UnsignedArray const& derivativeOrder = { 0 },
      StringArray const& boundaryCondition = {},
      BooleanArray const& process = {},
      String method = "best",
      dfloat truncation = 3
);
inline Image Gauss(
      Image const& in,
      FloatArray const& sigmas = { 1.0 },
      UnsignedArray const& derivativeOrder = { 0 },
      StringArray const& boundaryCondition = {},
      BooleanArray const& process = {},
      String const& method = "best",
      dfloat truncation = 3
) {
   Image out;
   Gauss( in, out, sigmas, derivativeOrder, boundaryCondition, process, method, truncation );
   return out;
}

/// \brief Finite difference derivatives
///
/// Computes derivatives using the finite difference method. Set a `derivativeOrder` for each dimension.
/// Derivatives of oder up to 2 can be computed with this function. The zeroth-order derivative implies either
/// a smoothing is applied (`smoothFlag == "smooth"`) or the dimension is not processed at all.
///
/// The smoothing filter is `[1,2,1]/4` (as in the Sobel filter), the first order derivative is `[1,0,-1]/2`
/// (central difference), and the second order derivative is `[1,-2,1]` (which is the composition of twice the
/// non-central difference `[1,-1]`). Thus, computing the first derivative twice does not yield the same result
/// as computing the second derivative directly.
///
/// `boundaryCondition` indicates how the boundary should be expanded in each dimension. See `dip::BoundaryCondition`.
///
/// Set `process` to false for those dimensions that should not be filtered.
///
/// \see dip::Derivative, dip::SobelGradient
DIP_EXPORT void FiniteDifference(
      Image const& in,
      Image& out,
      UnsignedArray derivativeOrder = { 0 },
      String const& smoothFlag = "smooth",
      StringArray const& boundaryCondition = {},
      BooleanArray process = {}
);
inline Image FiniteDifference(
      Image const& in,
      UnsignedArray const& derivativeOrder = { 0 },
      String const& smoothFlag = "smooth",
      StringArray const& boundaryCondition = {},
      BooleanArray const& process = {}
) {
   Image out;
   FiniteDifference( in, out, derivativeOrder, smoothFlag, boundaryCondition, process );
   return out;
}

/// \brief The Sobel derivative filter
///
/// This function applies the generalization of the Sobel derivative filter to arbitrary dimensions. Along the
/// dimension `dimension`, the central difference is computed, and along all other dimensions, the triangular
/// smoothing filter `[1,2,1]/4` is applied.
///
/// `boundaryCondition` indicates how the boundary should be expanded in each dimension. See `dip::BoundaryCondition`.
///
/// This function calls `dip::FiniteDifference`.
inline void SobelGradient(
      Image const& in,
      Image& out,
      dip::uint dimension = 0,
      StringArray const& boundaryCondition = {}
) {
   DIP_THROW_IF( dimension >= in.Dimensionality(), E::PARAMETER_OUT_OF_RANGE );
   UnsignedArray derivativeOrder( in.Dimensionality(), 0 );
   derivativeOrder[ dimension ] = 1;
   FiniteDifference( in, out, derivativeOrder, "smooth", boundaryCondition );
}
inline Image SobelGradient(
      Image const& in,
      dip::uint dimension = 0,
      StringArray const& boundaryCondition = {}
) {
   Image out;
   SobelGradient( in, out, dimension, boundaryCondition );
   return out;
}

/// \brief Computes derivatives
///
/// This function provides an interface to the various derivative filters in DIPlib.
///
/// For each dimension, provide a value in `sigmas` and `derivativeOrder`. The value of sigma determines
/// the smoothing effect. The zeroth-order derivative is a plain smoothing, no derivative is computed.
///
/// `method` indicates which derivative filter is used:
/// - `"best"`: A Gaussian derivative, see `dip::Gauss`.
/// - `"gaussfir"`: The FIR implementation of the Gaussian derivative.
/// - `"gaussiir"`: The IIR implementation of the Gaussian derivative.
/// - `"gaussft"`: The FT implementation of the Gaussian derivative.
/// - `"finitediff"`: A finite difference derivative, see `dip::FiniteDifference`.
///
/// A finite difference derivative is an approximation to the derivative operator on the discrete grid.
/// In contrast, convolving an image with the derivative of a Gaussian provides the exact derivative of
/// the image convolved with a Gaussian:
/// \f$\frac{\partial G}{\partial x}\otimes f = \frac{\partial}{\partial x}(G \otimes f)\f$
/// Thus (considering the regularization provided by the Gaussian smoothing is beneficial) it is always
/// better to use Gaussian derivatives than finite difference derivatives.
///
/// `boundaryCondition` indicates how the boundary should be expanded in each dimension. See `dip::BoundaryCondition`.
///
/// Set `process` to false for those dimensions that should not be filtered. Alternatively, set
/// `sigmas` to 0 or a negative value.
///
/// \see dip::Gauss, dip::FiniteDifference
DIP_EXPORT void Derivative(
      Image const& in,
      Image& out,
      UnsignedArray const& derivativeOrder,
      FloatArray const& sigmas = { 1.0 },
      String const& method = "best",
      StringArray const& boundaryCondition = {},
      BooleanArray const& process = {},
      dfloat truncation = 3
);
inline Image Derivative(
      Image const& in,
      UnsignedArray const& derivativeOrder,
      FloatArray const& sigmas = { 1.0 },
      String const& method = "best",
      StringArray const& boundaryCondition = {},
      BooleanArray const& process = {},
      dfloat truncation = 3
) {
   Image out;
   Derivative( in, out, derivativeOrder, sigmas, method, boundaryCondition, process, truncation );
   return out;
}

/// \brief Computes the first derivative along x, see `dip::Derivative`.
inline void Dx(
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 }
) {
   DIP_THROW_IF( in.Dimensionality() < 1, E::DIMENSIONALITY_NOT_SUPPORTED );
   UnsignedArray derivativeOrder( in.Dimensionality(), 0 );
   derivativeOrder[ 0 ] = 1;
   Derivative( in, out, derivativeOrder, sigmas );
}
inline Image Dx(
      Image const& in,
      FloatArray const& sigmas = { 1.0 }
) {
   Image out;
   Dx( in, out, sigmas );
   return out;
}

/// \brief Computes the first derivative along y, see `dip::Derivative`.
inline void Dy(
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 }
) {
   DIP_THROW_IF( in.Dimensionality() < 2, E::DIMENSIONALITY_NOT_SUPPORTED );
   UnsignedArray derivativeOrder( in.Dimensionality(), 0 );
   derivativeOrder[ 1 ] = 1;
   Derivative( in, out, derivativeOrder, sigmas );
}
inline Image Dy(
      Image const& in,
      FloatArray const& sigmas = { 1.0 }
) {
   Image out;
   Dy( in, out, sigmas );
   return out;
}

/// \brief Computes the first derivative along z, see `dip::Derivative`.
inline void Dz(
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 }
) {
   DIP_THROW_IF( in.Dimensionality() < 3, E::DIMENSIONALITY_NOT_SUPPORTED );
   UnsignedArray derivativeOrder( in.Dimensionality(), 0 );
   derivativeOrder[ 2 ] = 1;
   Derivative( in, out, derivativeOrder, sigmas );
}
inline Image Dz(
      Image const& in,
      FloatArray const& sigmas = { 1.0 }
) {
   Image out;
   Dz( in, out, sigmas );
   return out;
}

/// \brief Computes the second derivative along x, see `dip::Derivative`.
inline void Dxx(
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 }
) {
   DIP_THROW_IF( in.Dimensionality() < 1, E::DIMENSIONALITY_NOT_SUPPORTED );
   UnsignedArray derivativeOrder( in.Dimensionality(), 0 );
   derivativeOrder[ 0 ] = 2;
   Derivative( in, out, derivativeOrder, sigmas );
}
inline Image Dxx(
      Image const& in,
      FloatArray const& sigmas = { 1.0 }
) {
   Image out;
   Dxx( in, out, sigmas );
   return out;
}

/// \brief Computes the second derivative along y, see `dip::Derivative`.
inline void Dyy(
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 }
) {
   DIP_THROW_IF( in.Dimensionality() < 2, E::DIMENSIONALITY_NOT_SUPPORTED );
   UnsignedArray derivativeOrder( in.Dimensionality(), 0 );
   derivativeOrder[ 1 ] = 2;
   Derivative( in, out, derivativeOrder, sigmas );
}
inline Image Dyy(
      Image const& in,
      FloatArray const& sigmas = { 1.0 }
) {
   Image out;
   Dyy( in, out, sigmas );
   return out;
}

/// \brief Computes the second derivative along z, see `dip::Derivative`.
inline void Dzz(
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 }
) {
   DIP_THROW_IF( in.Dimensionality() < 3, E::DIMENSIONALITY_NOT_SUPPORTED );
   UnsignedArray derivativeOrder( in.Dimensionality(), 0 );
   derivativeOrder[ 2 ] = 2;
   Derivative( in, out, derivativeOrder, sigmas );
}
inline Image Dzz(
      Image const& in,
      FloatArray const& sigmas = { 1.0 }
) {
   Image out;
   Dzz( in, out, sigmas );
   return out;
}

/// \brief Computes the first derivative along x and y, see `dip::Derivative`.
inline void Dxy(
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 }
) {
   DIP_THROW_IF( in.Dimensionality() < 2, E::DIMENSIONALITY_NOT_SUPPORTED );
   UnsignedArray derivativeOrder( in.Dimensionality(), 0 );
   derivativeOrder[ 0 ] = 1;
   derivativeOrder[ 1 ] = 1;
   Derivative( in, out, derivativeOrder, sigmas );
}
inline Image Dxy(
      Image const& in,
      FloatArray const& sigmas = { 1.0 }
) {
   Image out;
   Dxy( in, out, sigmas );
   return out;
}

/// \brief Computes the first derivative along x and z, see `dip::Derivative`.
inline void Dxz(
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 }
) {
   DIP_THROW_IF( in.Dimensionality() < 3, E::DIMENSIONALITY_NOT_SUPPORTED );
   UnsignedArray derivativeOrder( in.Dimensionality(), 0 );
   derivativeOrder[ 0 ] = 1;
   derivativeOrder[ 2 ] = 1;
   Derivative( in, out, derivativeOrder, sigmas );
}
inline Image Dxz(
      Image const& in,
      FloatArray const& sigmas = { 1.0 }
) {
   Image out;
   Dxz( in, out, sigmas );
   return out;
}

/// \brief Computes the first derivative along y and y, see `dip::Derivative`.
inline void Dyz(
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 }
) {
   DIP_THROW_IF( in.Dimensionality() < 3, E::DIMENSIONALITY_NOT_SUPPORTED );
   UnsignedArray derivativeOrder( in.Dimensionality(), 0 );
   derivativeOrder[ 1 ] = 1;
   derivativeOrder[ 2 ] = 1;
   Derivative( in, out, derivativeOrder, sigmas );
}
inline Image Dyz(
      Image const& in,
      FloatArray const& sigmas = { 1.0 }
) {
   Image out;
   Dyz( in, out, sigmas );
   return out;
}

DIP_EXPORT void Gradient(
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 },
      String const& method = "best",
      StringArray const& boundaryCondition = {},
      BooleanArray const& process = {},
      dfloat truncation = 3
);

// Same as Norm(Gradient()), but more efficient
DIP_EXPORT void GradientMagnitude(
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 },
      String const& method = "best",
      StringArray const& boundaryCondition = {},
      BooleanArray const& process = {},
      dfloat truncation = 3
);

// 2D only. Implemented as Atan2(Dy(),Dx()).
DIP_EXPORT void GradientDirection2D(
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 },
      String const& method = "best",
      StringArray const& boundaryCondition = {},
      BooleanArray const& process = {},
      dfloat truncation = 3
);

DIP_EXPORT void Hessian (
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 },
      String const& method = "best",
      StringArray const& boundaryCondition = {},
      BooleanArray const& process = {},
      dfloat truncation = 3
);

// Same as Trace(Hessian()), but more efficient. If "finitediff", uses the well-known 3-by-3 matrix.
DIP_EXPORT void Laplace (
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 },
      String const& method = "best",
      StringArray const& boundaryCondition = {},
      BooleanArray const& process = {},
      dfloat truncation = 3
);

DIP_EXPORT void Dgg(
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 },
      String const& method = "best",
      StringArray const& boundaryCondition = {},
      BooleanArray const& process = {},
      dfloat truncation = 3
);

DIP_EXPORT void LaplacePlusDgg(
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 },
      String const& method = "best",
      StringArray const& boundaryCondition = {},
      BooleanArray const& process = {},
      dfloat truncation = 3
);

DIP_EXPORT void LaplaceMinDgg(
      Image const& in,
      Image& out,
      FloatArray const& sigmas = { 1.0 },
      String const& method = "best",
      StringArray const& boundaryCondition = {},
      BooleanArray const& process = {},
      dfloat truncation = 3
);

DIP_EXPORT void OrientedGauss(
      Image const& in,
      Image& out,
      FloatArray,
      FloatArray
);

DIP_EXPORT void GaborFIR(
      Image const& in,
      Image& out,
      FloatArray sigmas,
      FloatArray frequencies,
      StringArray const& boundaryCondition = {},
      BooleanArray process = {},
      dfloat truncation = 3
);

DIP_EXPORT void GaborIIR(
      Image const& in,
      Image& out,
      FloatArray sigmas,
      FloatArray frequencies,
      StringArray const& boundaryCondition = {},
      BooleanArray process = {},
      IntegerArray filterOrder = {},
      dfloat truncation = 3
);

/// \}

} // namespace dip

#endif // DIP_LINEAR_H
