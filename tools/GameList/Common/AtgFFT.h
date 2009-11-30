#pragma once
#include <vectorintrinsics.h>


void FFT_Unoptimized( float* pReal, float* pImaginary, float sign, unsigned long countLog2, unsigned long passes = 0 );

void FFT_8( __vector4* __restrict pReal, __vector4* __restrict pImag );
void FFT_16( __vector4* __restrict pReal, __vector4* __restrict pImag );
void FFT_32( __vector4* __restrict pReal, __vector4* __restrict pImag );
void FFT_64( __vector4* __restrict pReal, __vector4* __restrict pImag );
void FFT_256( __vector4* __restrict pReal, __vector4* __restrict pImag );
void FFT_1024( __vector4* __restrict pReal, __vector4* __restrict pImag );

//
// Warning: Unswizzle function is not optimized (although it is intended
// for use with the optimized FFTs above--the FFT_Unoptimized function does
// it's own unswizzling)
//
void FFT_Unswizzle( float* __restrict pOut, float* __restrict pIn, int nFloatsLog2 );
