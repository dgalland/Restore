// Functions that interpolates a frame

// See legal notice in Copying.txt for more information

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .

/*! \file Interpolation.h
 *  \brief Interpolating functions.
 *
 *	This set of functions is used to create a picture 4 times bigger, by
 *	interpolating the value of the half pixels. There are three different
 *  interpolations : horizontal computes the pixels ( x + 0.5, y ), vertical
 *	computes the pixels ( x, y + 0.5 ), and finally, diagonal computes the
 *  pixels ( x + 0.5, y + 0.5 ).
 *	For each type of interpolations, there are two functions, one in classical C,
 *  and one optimized for iSSE processors.
 */

#ifndef __INTERPOL__
#define __INTERPOL__

void VerticalBilin(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                   int nSrcPitch, int nWidth, int nHeight);
void HorizontalBilin(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                     int nSrcPitch, int nWidth, int nHeight);
void DiagonalBilin(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                   int nSrcPitch, int nWidth, int nHeight);

void RB2F_C(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
            int nSrcPitch, int nWidth, int nHeight);
void RB2Filtered_C(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
            int nSrcPitch, int nWidth, int nHeight);
void RB2BilinearFiltered_C(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
            int nSrcPitch, int nWidth, int nHeight);
void RB2Quadratic_C(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
            int nSrcPitch, int nWidth, int nHeight);
void RB2Cubic_C(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
            int nSrcPitch, int nWidth, int nHeight);

extern "C" void __cdecl VerticalBilin_iSSE(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                                           int nSrcPitch, int nWidth, int nHeight);
extern "C" void __cdecl HorizontalBilin_iSSE(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                                             int nSrcPitch, int nWidth, int nHeight);
extern "C" void __cdecl DiagonalBilin_iSSE(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                                           int nSrcPitch, int nWidth, int nHeight);

extern "C" void __cdecl RB2F_iSSE(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                                  int nSrcPitch, int nWidth, int nHeight);

void VerticalWiener(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                   int nSrcPitch, int nWidth, int nHeight);
void HorizontalWiener(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                     int nSrcPitch, int nWidth, int nHeight);
void DiagonalWiener(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                   int nSrcPitch, int nWidth, int nHeight);

extern "C" void __cdecl VerticalWiener_iSSE(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                                           int nSrcPitch, int nWidth, int nHeight);
extern "C" void __cdecl HorizontalWiener_iSSE(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                                           int nSrcPitch, int nWidth, int nHeight);

void VerticalBicubic(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                   int nSrcPitch, int nWidth, int nHeight);
void HorizontalBicubic(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                     int nSrcPitch, int nWidth, int nHeight);
void DiagonalBicubic(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                   int nSrcPitch, int nWidth, int nHeight);

extern "C" void __cdecl VerticalBicubic_iSSE(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                                           int nSrcPitch, int nWidth, int nHeight);
extern "C" void __cdecl HorizontalBicubic_iSSE(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                                           int nSrcPitch, int nWidth, int nHeight);

void Average2(unsigned char *pDst, const unsigned char *pSrc1, const unsigned char *pSrc2,
                   int nPitch, int nWidth, int nHeight);

#endif
