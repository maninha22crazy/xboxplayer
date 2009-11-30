#include "stdafx.h"
#pragma warning( disable:4709 )
#pragma warning( disable:4239 )

#include "atgfft.h"
#include <xtl.h>
#include "ATGMetaVMX.h"

using namespace ATG;


#define _USE_MATH_DEFINES
#include <math.h> // definition of PI
#include <complex>

unsigned int reverse4( unsigned int n, unsigned int size )
{
    n = ( ( n & 0xcccccccc ) >> 2 ) | ( ( n & 0x33333333 ) << 2 );
    n = ( ( n & 0xf0f0f0f0 ) >> 4 ) | ( ( n & 0x0f0f0f0f ) << 4 );
    n = ( ( n & 0xff00ff00 ) >> 8 ) | ( ( n & 0x00ff00ff ) << 8 );
    n = ( ( n & 0xffff0000 ) >> 16 ) | ( ( n & 0x0000ffff ) << 16 );

    n = n >> ( 32 - size );
    return n;
}

unsigned int reverse2( unsigned int n, unsigned int size )
{
    n = ( ( n & 0xaaaaaaaa ) >> 1 ) | ( ( n & 0x55555555 ) << 1 );
    n = ( ( n & 0xcccccccc ) >> 2 ) | ( ( n & 0x33333333 ) << 2 );
    n = ( ( n & 0xf0f0f0f0 ) >> 4 ) | ( ( n & 0x0f0f0f0f ) << 4 );
    n = ( ( n & 0xff00ff00 ) >> 8 ) | ( ( n & 0x00ff00ff ) << 8 );
    n = ( ( n & 0xffff0000 ) >> 16 ) | ( ( n & 0x0000ffff ) << 16 );

    n = n >> ( 32 - size );
    return n;
}

void FFT_Unoptimized( float* pReal, float* pImaginary, float sign, DWORD countLog2, DWORD passes )
{
    if( passes == 0 ) passes = countLog2;
    DWORD count = 1 << countLog2;
    std::complex<float>* pData = new std::complex<float>[count];
    std::complex<float>* pTwiddles = new std::complex<float>[count];

    for( DWORD i = 0; i < count; ++i )
    {
        pTwiddles[i] = std::exp( sign * 2.0f * (float)M_PI * std::complex<float>(0.0f,1.0f) * (float)i/ (float)count );
    }

    for( DWORD i = 0; i < count; ++i )
    {
        pData[i] = std::complex<float>( pReal[i], pImaginary[i] );
    }

    for( DWORD i = 0; i < passes; ++i )
    {
        for( DWORD j = 0; j < count; j += count >> i )
        {
            DWORD len = (count/2)>>i;
            for( DWORD k = 0; k < len; ++k )
            {

                std::complex<float> a = pData[j + k];
                std::complex<float> b = pData[j + len + k];

                pData[j+k] = a + b;
                pData[j + len + k] = (a - b) * pTwiddles[k*(1<<i)];
            }
        }
    }
  
    for( DWORD i = 0; i < count; ++i )
    {
        pReal[i] = pData[reverse2(i,countLog2)].real();
        pImaginary[i] = pData[reverse2(i,countLog2)].imag();
    }
    delete[] pTwiddles;
    delete[] pData;
}


template< int length >
struct RootsOfUnity
{
    const static int width = length / 4;
    __vector4 real[width];
    __vector4 imag[width];

    RootsOfUnity()
    {
        float step = 2.0f * (float)M_PI / (float)length;
        for( int i = 0; i < 4; ++i )
        {
            for( int j = 0; j < width; ++j )
            {
                int index = ( i * width ) + j;
                real[index/4].v[index%4] = cos( (float)i * (float)j * step );
                imag[index/4].v[index%4] = -sin( (float)i * (float)j * step );
            }
        }
    }
};




//
// Parallel multiplication of four complex numbers, assuming that
// real and complex values are stored in separate vectors
//
void vmulComplex4( vector4_out r1, vector4_out i1, vector4_in r2, vector4_in i2 )
{
    // (r1,i1) * (r2,i2) = (r1r2 - i1i2, r1i2 + r2i1 )
    __vector4 rTemp = __vnmsubfp( i1, i2, __vmulfp( r1, r2 ) );
    i1 = __vmaddfp( r1, i2, __vmulfp( r2, i1 ) );
    r1 = rTemp;
}

void vmulComplex4( vector4_out rOut, vector4_out iOut, vector4_in r1, vector4_in i1, vector4_in r2, vector4_in i2 )
{
    // (r1,i1) * (r2,i2) = (r1r2 - i1i2, r1i2 + r2i1 )
    rOut = __vnmsubfp( i1, i2, __vmulfp( r1, r2 ) );
    iOut = __vmaddfp( r1, i2, __vmulfp( r2, i1 ) );
}


//
// Sign and permutation constants for radix-4 butterflies
//
const static __vector4 v4DFT4SignBits1 = { 0.0f, -0.0f, 0.0f, -0.0f };
const static __vector4 v4DFT4SignBits2 = { 0.0f, 0.0f, -0.0f, -0.0f };
const static __vector4 v4DFT4SignBits3 = { 0.0f, -0.0f, -0.0f, 0.0f };
const static ULONG __declspec(align(16)) ulPermute_Z1W2Z1W2[] = { 0x08090a0b, 0x1c1d1e1f, 0x08090a0b, 0x1c1d1e1f };
const static __vector4 v4Permute_Z1W2Z1W2 = *(__vector4*)ulPermute_Z1W2Z1W2;


void __forceinline ButterflyDIT2_4( vector4_out rData1, vector4_out rData2, vector4_out iData1, vector4_out iData2 )
{
    const __vector4 r = { 1, .707168f, 0, -.707168f };
    const __vector4 i = { 0, .707168f, 1, .707168f };
    vmulComplex4( rData2, iData2, r, i );

    rData1 = __vaddfp( rData1, rData2 );
    rData2 = __vsubfp( rData1, rData2 );
    iData1 = __vaddfp( iData1, iData2 );
    iData2 = __vsubfp( iData1, iData2 );
}

void __forceinline ButterflyDIF2_4( vector4_out rData1, vector4_out rData2, vector4_out iData1, vector4_out iData2 )
{
    const __vector4 r = { 1, .707168f, 0, -.707168f };
    const __vector4 i = { 0, .707168f, 1, .707168f };
    __vector4 r1 = __vaddfp( rData1, rData2 );
    __vector4 i1 = __vaddfp( iData1, iData2 );

    __vector4 r2 = __vsubfp( rData1, rData2 );
    __vector4 i2 = __vsubfp( iData1, iData2 );
    vmulComplex4( r2, i2, r, i );

    rData1 = r1;
    rData2 = r2;
    iData1 = i1;
    iData2 = i2;
}

//
// Radix-4 decimation-in-time FFT butterfly
//
// This version assumes that all four elements of the butterfly are adjacent
// in a single __vector4.
//
void __forceinline ButterflyDIT4_1( vector4_out rData, vector4_out iData)
{
    /*  Compute the product of the complex input vector and the
    4-element DFT matrix:

    |  1  1  1  1  |    | (Xr,Xi) |      
    |  1 -j -1  j  |    | (Yr,Yi) |
    |  1 -1  1 -1  |    | (Zr,Zi) |
    |  1  j -1 -j  |    | (Wr,Wi) |

    This matrix can be decomposed into two simpler ones to reduce the number
    of additions needed. The decomposed matrices look like this:

    |  1  0  1  0  |    |  1  0  1  0  | 
    |  0  1  0 -j  |    |  1  0 -1  0  |
    |  1  0 -1  0  |    |  0  1  0  1  |
    |  0  1  0  j  |    |  0  1  0 -1  |

    Combine as follows:

            |  1  0  1  0  |    | (rInX,iInX) |          | (rInX + rInZ, iInX + iInZ) |
    Out =   |  1  0 -1  0  | *  | (rInY,iInY) |       =  | (rInX - rInZ, iInX - iInZ) |
            |  0  1  0  1  |    | (rInZ,iInZ) |          | (rInY + rInW, iInY + iInW) |
            |  0  1  0 -1  |    | (rInW,iInW) |          | (rInY - rInW, iInY - iInW) |

            |  1  0  1  0  |    | (rOutX,iOutX) |        | (rOutX + rOutZ, iOutX + iOutZ) |
    Out =   |  0  1  0 -j  | *  | (rOutY,iOutY) |     =  | (rOutY + iOutW, iOutY - rOutW) |
            |  1  0 -1  0  |    | (rOutZ,iOutZ) |        | (rOutX - rOutZ, iOutX - iOutZ) |
            |  0  1  0  j  |    | (rOutW,iOutW) |        | (rOutY - iOutW, iOutY + rOutW) |
    */

    // First matrix
    //
    __vector4 rTemp = __vaddfp( 
        __vpermwi( rData, VPERMWI_CONST( 0, 0, 1, 1 ) ),                              // X,X,Y,Y
        __vxor( __vpermwi( rData, VPERMWI_CONST( 2, 2, 3, 3 ) ), v4DFT4SignBits1 ) );   // Z,-Z,W,-W 

    __vector4 iTemp = __vaddfp(
        __vpermwi( iData, VPERMWI_CONST( 0, 0, 1, 1 ) ),      // X,X,Y,Y
        __vxor( __vpermwi( iData, VPERMWI_CONST( 2, 2, 3, 3 ) ), v4DFT4SignBits1 ) );    // Z,Z,W,W 

    // Second
    //
    __vector4 rZiWrZiW = __vperm( rTemp, iTemp, v4Permute_Z1W2Z1W2 );
    __vector4 iZrWiZrW = __vperm( iTemp, rTemp, v4Permute_Z1W2Z1W2 );
    rData = __vaddfp(
        __vpermwi( rTemp, VPERMWI_CONST( 0, 1, 0, 1 ) ),                 // X,Y,X,Y
        __vxor( rZiWrZiW, v4DFT4SignBits2 ) );                                

    iData = __vaddfp( 
        __vpermwi( iTemp, VPERMWI_CONST( 0, 1, 0, 1 ) ),                 // X,Y,X,Y
        __vxor( iZrWiZrW, v4DFT4SignBits3 ) );                               
}



//
// Radix-4 decimation-in-time FFT butterfly
//
// This version assumes that the elements of the butterfly are
// in different __vector4s, so that each __vector4 in the input
// contains elements from four different butterflies. The four
// separate butterflies are processed in parallel.
//
template<int stride, bool last>
void __forceinline ButterflyDIT4_4( 
                             vector4_out rData0, 
                             vector4_out rData1, 
                             vector4_out rData2, 
                             vector4_out rData3, 
                             vector4_out iData0, 
                             vector4_out iData1, 
                             vector4_out iData2, 
                             vector4_out iData3, 
                            const __vector4* __restrict rW,
                            const __vector4* __restrict iW) 
{
    /*
    The calculations here are the same as the ones in the single-vector radix-4 DFT, 
    but instead of being done on a single vector (X,Y,Z,W) they're done in parallel on
    sixteen independent complex values. There is no interdependence between the vector
    elements.

    |  1  0  1  0  |    | (rIn0,iIn0) |                         | (rIn0 + rIn2, iIn0 + iIn2) |
    |  1  0 -1  0  | *  | (rIn1,iIn1) |       = (rOut,iOut) =   | (rIn0 - rIn2, iIn0 - iIn2) |
    |  0  1  0  1  |    | (rIn2,iIn2) |                         | (rIn1 + rIn3, iIn1 + iIn3) |
    |  0  1  0 -1  |    | (rIn3,iIn3) |                         | (rIn1 - rIn3, iIn1 - iIn3) |

            |  1  0  1  0  |    | (rOut0,iOut0) |        | (rOut0 + rOut2, iOut0 + iOut2) |
    Out =   |  0  1  0 -j  | *  | (rOut1,iOut1) |     =  | (rOut1 + iOut3, iOut1 - rOut3) |
            |  1  0 -1  0  |    | (rOut2,iOut2) |        | (rOut0 - rOut2, iOut0 - iOut2) |
            |  0  1  0  j  |    | (rOut3,iOut3) |        | (rOut1 - iOut3, iOut1 + rOut3) |
    */
    __vector4 rTemp0, rTemp1, rTemp2, rTemp3;
    __vector4 rTemp4, rTemp5, rTemp6, rTemp7;
    __vector4 iTemp0, iTemp1, iTemp2, iTemp3;
    __vector4 iTemp4, iTemp5, iTemp6, iTemp7;
    // first matrix
    rTemp0 = __vaddfp( rData0, rData2 );   iTemp0 = __vaddfp( iData0, iData2 );

    rTemp1 = __vsubfp( rData0, rData2 );   iTemp1 = __vsubfp( iData0, iData2 );

    rTemp2 = __vaddfp( rData1, rData3 );   iTemp2 = __vaddfp( iData1, iData3 );

    rTemp3 = __vsubfp( rData1, rData3 );   iTemp3 = __vsubfp( iData1, iData3 );

    // second matrix
    rTemp4 = __vaddfp( rTemp0, rTemp2 );     iTemp4 = __vaddfp( iTemp0, iTemp2 );
                                             
    rTemp5 = __vaddfp( rTemp1, iTemp3 );     iTemp5 = __vsubfp( iTemp1, rTemp3 );
                                             
    rTemp6 = __vsubfp( rTemp0, rTemp2 );     iTemp6 = __vsubfp( iTemp0, iTemp2 );
                                             
    rTemp7 = __vsubfp( rTemp1, iTemp3 );     iTemp7 = __vaddfp( iTemp1, rTemp3 );

    // Do the twiddle-factor multiplication
    // vmulComplex4( rOut0, iOut0, rOut0, iOut0, rW0, iW0 ); // first one is always trivial
    vmulComplex4( rTemp5, iTemp5, rW[1*stride], iW[1*stride] );
    vmulComplex4( rTemp6, iTemp6, rW[2*stride], iW[2*stride] );
    vmulComplex4( rTemp7, iTemp7, rW[3*stride], iW[3*stride] );

    if( last )
    {
        ButterflyDIT4_1( rTemp4, iTemp4 );
        ButterflyDIT4_1( rTemp5, iTemp5 );
        ButterflyDIT4_1( rTemp6, iTemp6 );
        ButterflyDIT4_1( rTemp7, iTemp7 );
    }

    rData0 = rTemp4;
    rData1 = rTemp5;
    rData2 = rTemp6;
    rData3 = rTemp7;

    iData0 = iTemp4;
    iData1 = iTemp5;
    iData2 = iTemp6;
    iData3 = iTemp7;

}

template< int stage_len >
struct FFT_DIT4
{
    template<int count>
    static void __forceinline eval( __vector4* __restrict rData, __vector4* __restrict iData )
    {
        const int total = count * stage_len;
        const int total_vectors = total / 4;
        const int stage_vectors = stage_len/4;
        const int stride = stage_vectors / 4; // stride between butterfly elements
        const int skip = stage_vectors - stride;

        // butterfly
        int n = 0;
        for( int i = 0; i < total_vectors / 4; ++i )
        {
            n = ( i/stride ) * ( stride + skip ) + ( i % stride );
            ButterflyDIT4_4<stride,false>( 
                rData[ n ],
                rData[ n + stride ],
                rData[ n + stride * 2 ],
                rData[ n + stride * 3 ],
                iData[ n ],
                iData[ n + stride * 1 ],
                iData[ n + stride * 2 ],
                iData[ n + stride * 3 ],
                roots.real + n % stage_vectors,
                roots.imag + n % stage_vectors);


        }
        FFT_DIT4< stage_len / 4>::eval<count * 4 >( rData, iData );
    }
    static RootsOfUnity<stage_len> roots;

};
template< int stage_len >
RootsOfUnity<stage_len> FFT_DIT4<stage_len>::roots;

template< >
struct FFT_DIT4<4>
{
    template<int nsubs>
    static void __forceinline eval( __vector4* __restrict rData, __vector4* __restrict iData )
    {
        for( int i = 0; i < nsubs; ++i )
        {
            ButterflyDIT4_1( rData[i], iData[i] );
        }
    }
};

template<>
struct FFT_DIT4<8>
{
    template<int nsubs>
    static void __forceinline eval( __vector4* __restrict rData, __vector4* __restrict iData )
    {
        for( int j = 0; j < nsubs; ++j )
        {
            __vector4* pReal = rData + j *2;
            __vector4* pImag = iData + j *2;

            __vector4 oddsR = { pReal[0].v[1], pReal[0].v[3], pReal[1].v[1], pReal[1].v[3] };
            __vector4 evensR = { pReal[0].v[0], pReal[0].v[2], pReal[1].v[0], pReal[1].v[2] };
            __vector4 oddsI = { pImag[0].v[1], pImag[0].v[3], pImag[1].v[1], pImag[1].v[3] };
            __vector4 evensI = { pImag[0].v[0], pImag[0].v[2], pImag[1].v[0], pImag[1].v[2] };

            ButterflyDIT4_1( oddsR, oddsI );
            ButterflyDIT4_1( evensR, evensI );

            const __vector4 wr1 = { 1, .707168f, 0, -.707168f };
            const __vector4 wi1 = { 0, -.707168f, -1, -.707168f };
            const __vector4 wr2 = { -1, -.707168f, 0, .707168f };
            const __vector4 wi2 = { 0, .707168f, 1, .707168f };

            __vector4 r, i;
            vmulComplex4( r, i, oddsR, oddsI, wr1, wi1 );
            pReal[0] = __vaddfp( evensR, r );
            pImag[0] = __vaddfp( evensI, i );

            vmulComplex4( r, i, oddsR, oddsI, wr2, wi2 );
            pReal[1] = __vaddfp( evensR, r );
            pImag[1] = __vaddfp( evensI, i );
        }
    }
};


template< >
struct FFT_DIT4<16>
{
    template<int nsubs>
    static void __forceinline eval( __vector4* __restrict rData, __vector4* __restrict iData )
    {
        // butterfly
        for( int i = 0; i < nsubs; ++i )
        {
            ButterflyDIT4_4<1,true>( 
                rData[i * 4],
                rData[ i * 4 + 1 ],
                rData[ i * 4 + 2 ],
                rData[ i * 4 + 3 ],
                iData[ i * 4 ],
                iData[ i * 4 + 1 ],
                iData[ i * 4 + 2 ],
                iData[ i * 4 + 3 ],
                roots.real,
                roots.imag );
        }
    }
    static RootsOfUnity<16> roots;
};
RootsOfUnity<16> FFT_DIT4<16>::roots;


void FFT_8( __vector4* __restrict pReal, __vector4* __restrict pImag )
{
    FFT_DIT4<8>::eval<1>(pReal, pImag);

}
void FFT_16( __vector4* __restrict pReal, __vector4* __restrict pImag )
{
    FFT_DIT4<16>::eval<1>( pReal, pImag );
}

void FFT_32( __vector4* __restrict pReal, __vector4* __restrict pImag )
{
    FFT_DIT4<32>::eval<1>( pReal, pImag );
}

void FFT_64( __vector4* __restrict pReal, __vector4* __restrict pImag )
{
    FFT_DIT4<64>::eval<1>( pReal, pImag );
}

void FFT_256( __vector4* __restrict pReal, __vector4* __restrict pImag )
{
    FFT_DIT4<256>::eval<1>( pReal, pImag );
}

void FFT_1024( __vector4* __restrict pReal, __vector4* __restrict pImag )
{
    FFT_DIT4<1024>::eval<1>( pReal, pImag );
}


void FFT_Unswizzle( float* __restrict pOut, float* __restrict pIn, int nFloatsLog2 )
{
    if( nFloatsLog2 % 2 == 0 )
    {
        for( int i = 0; i < (1 << nFloatsLog2); ++i )
        {
            int other = reverse4(i,nFloatsLog2);
            pOut[other] = pIn[i];
        }
    }
    else
    {
        for( int i = 0; i < 1 << nFloatsLog2; ++i )
        {
            int other = (i/8)+(i%8)*4;
            pOut[other] = pIn[i];
        }
    }
}




