/*
 * DIPlib 3.0
 * This file contains the definition the arithmetic operators.
 *
 * (c)2016, Cris Luengo.
 * Based on original DIPlib code: (c)1995-2014, Delft University of Technology.
 */

#include <iostream>
#include <array>

#include "diplib.h"
#include "dip_framework.h"
#include "dip_overload.h"
#include "dip_saturated_arithmetic.h"

namespace dip {

//
template< typename TPI >
static void dip__Add(
      const std::vector<Framework::ScanBuffer>&   inBuffer,
      std::vector<Framework::ScanBuffer>&         outBuffer,
      dip::uint            bufferLength,
      dip::uint            dimension,
      UnsignedArray        position,
      const void*          functionParameters,
      void*                functionVariables
) {
   const TPI* lhs = (const TPI*)inBuffer[0].buffer;
   const TPI* rhs = (const TPI*)inBuffer[1].buffer;
   TPI* out = (TPI*)outBuffer[0].buffer;
   for( dip::uint ii = 0; ii < bufferLength; ++ii ) {
      for( dip::uint jj = 0; jj < outBuffer[0].tensorLength; ++jj ) { // all 3 buffers have same number of tensor elements
         out[jj * outBuffer[0].tensorStride] = saturated_add( lhs[jj * inBuffer[0].tensorStride],
                                                              rhs[jj * inBuffer[1].tensorStride]);
      }
      lhs += inBuffer[0].stride;
      rhs += inBuffer[1].stride;
      out += outBuffer[0].stride;
   }
}

void Add(
      const Image& lhs,
      const Image& rhs,
      Image& out,
      DataType dt
) {
   Framework::ScanFilter filter;
   DIP_OVL_ASSIGN_ALL( filter, dip__Add, dt );
   std::vector<void*> vars;
   Framework::ScanOptions opts;
   Framework::ScanDyadic( lhs, rhs, out, dt, dt, filter, nullptr, vars, opts );
}

//
template< typename TPI >
static void dip__Sub(
      const std::vector<Framework::ScanBuffer>&   inBuffer,
      std::vector<Framework::ScanBuffer>&         outBuffer,
      dip::uint            bufferLength,
      dip::uint            dimension,
      UnsignedArray        position,
      const void*          functionParameters,
      void*                functionVariables
) {
   const TPI* lhs = (const TPI*)inBuffer[0].buffer;
   const TPI* rhs = (const TPI*)inBuffer[1].buffer;
   TPI* out = (TPI*)outBuffer[0].buffer;
   for( dip::uint ii = 0; ii < bufferLength; ++ii ) {
      for( dip::uint jj = 0; jj < outBuffer[0].tensorLength; ++jj ) { // all 3 buffers have same number of tensor elements
         out[jj * outBuffer[0].tensorStride] = saturated_sub( lhs[jj * inBuffer[0].tensorStride],
                                                              rhs[jj * inBuffer[1].tensorStride]);
      }
      lhs += inBuffer[0].stride;
      rhs += inBuffer[1].stride;
      out += outBuffer[0].stride;
   }
}

void Sub(
      const Image& lhs,
      const Image& rhs,
      Image& out,
      DataType dt
) {
   Framework::ScanFilter filter;
   DIP_OVL_ASSIGN_ALL( filter, dip__Sub, dt );
   std::vector<void*> vars;
   Framework::ScanOptions opts;
   Framework::ScanDyadic( lhs, rhs, out, dt, dt, filter, nullptr, vars, opts );
}

//
template< typename TPI >
static void dip__Mul(
      const std::vector<Framework::ScanBuffer>&   inBuffer,
      std::vector<Framework::ScanBuffer>&         outBuffer,
      dip::uint            bufferLength,
      dip::uint            dimension,
      UnsignedArray        position,
      const void*          functionParameters,
      void*                functionVariables
) {
   const TPI* lhs = (const TPI*)inBuffer[0].buffer;
   const TPI* rhs = (const TPI*)inBuffer[1].buffer;
   TPI* out = (TPI*)outBuffer[0].buffer;
   const std::array< dip::uint, 3 >* params = (const std::array< dip::uint, 3 >*)functionParameters;
   // This function is only called for two non-scalar images.

   for( dip::uint ii = 0; ii < bufferLength; ++ii ) {
      for( dip::uint row = 0; row < (*params)[0]; ++row ) {
         for( dip::uint col = 0; col < (*params)[1]; ++col ) {
            TPI v = 0;
            for( dip::uint jj = 0; jj < (*params)[2]; ++jj ) {
               v = saturated_add( v, saturated_mul( lhs[(row + jj * (*params)[0]) * inBuffer[0].tensorStride],
                                                    rhs[(jj + col * (*params)[0]) * inBuffer[1].tensorStride]));
            }
            out[(row + col * (*params)[0]) * outBuffer[0].tensorStride] = v;
         }
      }
      lhs += inBuffer[0].stride;
      rhs += inBuffer[1].stride;
      out += outBuffer[0].stride;
   }
}

template< typename TPI >
static void dip__MulSamples(
      const std::vector<Framework::ScanBuffer>&   inBuffer,
      std::vector<Framework::ScanBuffer>&         outBuffer,
      dip::uint            bufferLength,
      dip::uint            dimension,
      UnsignedArray        position,
      const void*          functionParameters,
      void*                functionVariables
) {
   const TPI* lhs = (const TPI*)inBuffer[0].buffer;
   const TPI* rhs = (const TPI*)inBuffer[1].buffer;
   TPI* out = (TPI*)outBuffer[0].buffer;
   for( dip::uint ii = 0; ii < bufferLength; ++ii ) {
      for( dip::uint jj = 0; jj < outBuffer[0].tensorLength; ++jj ) { // all 3 buffers have same number of tensor elements
         out[jj * outBuffer[0].tensorStride] = saturated_mul( lhs[jj * inBuffer[0].tensorStride],
                                                              rhs[jj * inBuffer[1].tensorStride]);
      }
      lhs += inBuffer[0].stride;
      rhs += inBuffer[1].stride;
      out += outBuffer[0].stride;
   }
}

void Mul(
      const Image& lhs,
      const Image& rhs,
      Image& out,
      DataType dt
) {
   dip_ThrowIf( lhs.TensorColumns() != rhs.TensorRows(),  );

   Tensor outTensor;
   Framework::ScanOptions opts;
   bool samplewise = true;
   if( lhs.IsScalar() ) {
      outTensor = rhs.Tensor();
      opts += Framework::Scan_TensorAsSpatialDim;
      //samplewise = true;
   } else if( rhs.IsScalar() ) {
      outTensor = lhs.Tensor();
      opts += Framework::Scan_TensorAsSpatialDim;
      //samplewise = true;
   } else if( lhs.TensorColumns() == rhs.TensorRows() ) {
      // TODO: a special case could be for diagonal matrices, where each row/column of the other matrix is multiplied by the same value.
      outTensor = Tensor( lhs.TensorRows(), rhs.TensorColumns() );
      opts += Framework::Scan_ExpandTensorInBuffer;
      samplewise = false;
   } else {
      dip_Throw( "Inner tensor dimensions must match in multiplication" );
   }
   ImageConstRefArray inar { lhs, rhs };
   ImageRefArray outar { out };
   DataTypeArray inBufT { dt, dt };
   DataTypeArray outBufT { dt };
   DataTypeArray outImT { dt };
   UnsignedArray nElem { outTensor.Elements() };
   Framework::ScanFilter filter;
   if( samplewise ) {
      DIP_OVL_ASSIGN_ALL( filter, dip__MulSamples, dt );
   } else {
      DIP_OVL_ASSIGN_ALL( filter, dip__Mul, dt );
   }
   std::vector<void*> vars;
   std::array< dip::uint, 3 >params { lhs.TensorRows(), rhs.TensorColumns(), lhs.TensorColumns() }; // only used by dip__Mul, not by dip__MulSamples.
   Framework::Scan( inar, outar, inBufT, outBufT, outImT, nElem, filter, &params, vars, opts );
   out.ReshapeTensor( outTensor );
}

void MulSamples(
      const Image& lhs,
      const Image& rhs,
      Image& out,
      DataType dt
) {
   Framework::ScanFilter filter;
   DIP_OVL_ASSIGN_ALL( filter, dip__MulSamples, dt );
   std::vector<void*> vars;
   Framework::ScanOptions opts;
   Framework::ScanDyadic( lhs, rhs, out, dt, dt, filter, nullptr, vars, opts );
}

//
template< typename TPI >
static void dip__Div(
      const std::vector<Framework::ScanBuffer>&   inBuffer,
      std::vector<Framework::ScanBuffer>&         outBuffer,
      dip::uint            bufferLength,
      dip::uint            dimension,
      UnsignedArray        position,
      const void*          functionParameters,
      void*                functionVariables
) {
   const TPI* lhs = (const TPI*)inBuffer[0].buffer;
   const TPI* rhs = (const TPI*)inBuffer[1].buffer;
   TPI* out = (TPI*)outBuffer[0].buffer;
   for( dip::uint ii = 0; ii < bufferLength; ++ii ) {
      for( dip::uint jj = 0; jj < outBuffer[0].tensorLength; ++jj ) { // all 3 buffers have same number of tensor elements
         out[jj * outBuffer[0].tensorStride] = saturated_div( lhs[jj * inBuffer[0].tensorStride],
                                                              rhs[jj * inBuffer[1].tensorStride]);
      }
      lhs += inBuffer[0].stride;
      rhs += inBuffer[1].stride;
      out += outBuffer[0].stride;
   }
}

void Div(
      const Image& lhs,
      const Image& rhs,
      Image& out,
      DataType dt
) {
   dip_ThrowIf( rhs.TensorElements() != 1, "Divisor must be scalar image" );
   Framework::ScanFilter filter;
   DIP_OVL_ASSIGN_ALL( filter, dip__Div, dt );
   std::vector<void*> vars;
   Framework::ScanOptions opts;
   Framework::ScanDyadic( lhs, rhs, out, dt, dt, filter, nullptr, vars, opts );
}

//
template< typename TPI >
static void dip__Mod(
      const std::vector<Framework::ScanBuffer>&   inBuffer,
      std::vector<Framework::ScanBuffer>&         outBuffer,
      dip::uint            bufferLength,
      dip::uint            dimension,
      UnsignedArray        position,
      const void*          functionParameters,
      void*                functionVariables
) {
   const TPI* lhs = (const TPI*)inBuffer[0].buffer;
   const TPI* rhs = (const TPI*)inBuffer[1].buffer;
   TPI* out = (TPI*)outBuffer[0].buffer;
   for( dip::uint ii = 0; ii < bufferLength; ++ii ) {
      for( dip::uint jj = 0; jj < outBuffer[0].tensorLength; ++jj ) { // all 3 buffers have same number of tensor elements
         out[jj * outBuffer[0].tensorStride] = static_cast<TPI>( std::fmod( lhs[jj * inBuffer[0].tensorStride],
                                                                            rhs[jj * inBuffer[1].tensorStride]));
         //*out = *lhs % *rhs; // this only works for integer types.
      }
      lhs += inBuffer[0].stride;
      rhs += inBuffer[1].stride;
      out += outBuffer[0].stride;
   }
}

void Mod(
      const Image& lhs,
      const Image& rhs,
      Image& out,
      DataType dt
) {
   dip_ThrowIf( rhs.TensorElements() != 1, "Divisor must be scalar image" );
   Framework::ScanFilter filter;
   DIP_OVL_ASSIGN_REAL( filter, dip__Mod, dt ); // NOTE: non-binary and non-complex.
   std::vector<void*> vars;
   Framework::ScanOptions opts;
   Framework::ScanDyadic( lhs, rhs, out, dt, dt, filter, nullptr, vars, opts );
}

} // namespace dip