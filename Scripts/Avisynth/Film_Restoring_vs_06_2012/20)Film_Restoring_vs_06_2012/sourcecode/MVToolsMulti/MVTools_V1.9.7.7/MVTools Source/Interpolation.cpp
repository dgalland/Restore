// Functions that interpolates a frame
// Author: Manao
// Copyright(c)2006 A.G.Balakhnin aka Fizick - bicubic, Wiener

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

#include "Interpolation.h"
#include <algorithm>

void RB2F_C(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
            int nSrcPitch, int nWidth, int nHeight)
{
    for ( int y = 0; y < nHeight; ++y ) {
        for ( int x = 0; x < nWidth; ++x ) {
            int x2=x<<1;
            pDst[x] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[x2]) + static_cast<unsigned int>(pSrc[x2+1]) + 
                                                  static_cast<unsigned int>(pSrc[x2+nSrcPitch+1]) + 
                                                  static_cast<unsigned int>(pSrc[x2+nSrcPitch]) + 2) >> 2);
        }
        pDst += nDstPitch;
        pSrc += (nSrcPitch << 1);
    }
}

void VerticalBilin(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                   int nSrcPitch, int nWidth, int nHeight)
{
    for ( int j = 0; j < nHeight - 1; ++j ) {
        for ( int i = 0; i < nWidth; ++i )
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch]) + 1) >> 1);
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    // last row
    for ( int i = 0; i < nWidth; ++i )
        pDst[i] = pSrc[i];
}

void HorizontalBilin(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                     int nSrcPitch, int nWidth, int nHeight)
{
    for ( int j = 0; j < nHeight; ++j ) {
        for ( int i = 0; i < nWidth - 1; ++i )
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + 
                                                  static_cast<unsigned int>(pSrc[i + 1]) + 1) >> 1);

        pDst[nWidth - 1] = pSrc[nWidth - 1];
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
}

void DiagonalBilin(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                   int nSrcPitch, int nWidth, int nHeight)
{
    for ( int j = 0; j < nHeight - 1; ++j ) {
        for ( int i = 0; i < nWidth - 1; ++i )
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + static_cast<unsigned int>(pSrc[i + 1]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch + 1]) + 2) >> 2);

        pDst[nWidth - 1] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[nWidth - 1]) + 
                                                       static_cast<unsigned int>(pSrc[nWidth + nSrcPitch - 1]) + 1) >> 1);
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    for ( int i = 0; i < nWidth - 1; ++i )
        pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + 
                                              static_cast<unsigned int>(pSrc[i + 1]) + 1) >> 1);
    pDst[nWidth - 1] = pSrc[nWidth - 1];
}

// so called Wiener interpolation. (sharp, similar to Lanczos ?)
// invarint simplified, 6 taps. Weights: (1, -5, 20, 20, -5, 1)/32 - added by Fizick
void VerticalWiener(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                    int nSrcPitch, int nWidth, int nHeight)
{
    for ( int j = 0; j < 2; ++j ) {
        for ( int i = 0; i < nWidth; ++i )
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch]) + 1) >> 1);
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    for ( int j = 2; j < nHeight - 4; ++j ) {
        for ( int i = 0; i < nWidth; ++i ) {
            pDst[i] = static_cast<unsigned char>(
                      std::min<int>(255, std::max<int>(0, (static_cast<int>(pSrc[i-nSrcPitch*2]) + 
                                                          (-static_cast<int>(pSrc[i-nSrcPitch]) + (static_cast<int>(pSrc[i])<<2) + 
                                                          (static_cast<int>(pSrc[i+nSrcPitch])<<2) - 
                                                          static_cast<int>(pSrc[i+nSrcPitch*2]) )*5 +
                                                          static_cast<int>(pSrc[i+nSrcPitch*3]) + 16)>>5) ));
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    for ( int j = nHeight - 4; j < nHeight - 1; ++j ) {
        for ( int i = 0; i < nWidth; ++i ) {
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch]) + 1) >> 1);
        }

        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    // last row
    for ( int i = 0; i < nWidth; ++i )
        pDst[i] = pSrc[i];
}

void HorizontalWiener(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                      int nSrcPitch, int nWidth, int nHeight)
{
    for ( int j = 0; j < nHeight; ++j ) {
        pDst[0] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[0]) + static_cast<unsigned int>(pSrc[1]) + 1) >> 1);
        pDst[1] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[1]) + static_cast<unsigned int>(pSrc[2]) + 1) >> 1);
        for ( int i = 2; i < nWidth - 4; ++i ) {
            pDst[i] = static_cast<unsigned char>(
                std::min<int>(255, std::max<int>(0, (static_cast<int>(pSrc[i-2]) + (-static_cast<int>(pSrc[i-1]) + 
                                                     (static_cast<int>(pSrc[i])<<2) + (static_cast<int>(pSrc[i+1])<<2) -
                                                     static_cast<int>(pSrc[i+2]))*5 + static_cast<int>(pSrc[i+3]) + 16)>>5)));
        }
        for ( int i = nWidth - 4; i < nWidth - 1; ++i )
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + 
                                                  static_cast<unsigned int>(pSrc[i + 1]) + 1) >> 1);

        pDst[nWidth - 1] = pSrc[nWidth - 1];
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
}

void DiagonalWiener(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                    int nSrcPitch, int nWidth, int nHeight)
{
    for ( int j = 0; j < 2; ++j ) {
        for ( int i = 0; i < nWidth - 1; ++i )
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + static_cast<unsigned int>(pSrc[i + 1]) +
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch + 1]) + 2) >> 2);

        pDst[nWidth - 1] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[nWidth - 1]) + 
                                                       static_cast<unsigned int>(pSrc[nWidth + nSrcPitch - 1]) + 1) >> 1);
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    for ( int j = 2; j < nHeight - 4; ++j ) {
        for ( int i = 0; i < 2; ++i )
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + static_cast<unsigned int>(pSrc[i + 1]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch + 1]) + 2) >> 2);
        for ( int i = 2; i < nWidth - 4; ++i ) {
            pDst[i] = static_cast<unsigned char>(
                      std::min<int>(255, std::max<int>(0, (static_cast<int>(pSrc[i-2-nSrcPitch*2]) + 
                                                           (-static_cast<int>(pSrc[i-1-nSrcPitch]) + 
                                                           (static_cast<int>(pSrc[i])<<2) + 
                                                           (static_cast<int>(pSrc[i+1+nSrcPitch])<<2) - 
                                                           (static_cast<int>(pSrc[i+2+nSrcPitch*2])<<2))*5 + 
                                                           static_cast<int>(pSrc[i+3+nSrcPitch*3]) +
                                                           static_cast<int>(pSrc[i+3-nSrcPitch*2]) + 
                                                           (-static_cast<int>(pSrc[i+2-nSrcPitch]) + 
                                                           (static_cast<int>(pSrc[i+1])<<2) + 
                                                           (static_cast<int>(pSrc[i+nSrcPitch])<<2) - 
                                                           static_cast<int>(pSrc[i-1+nSrcPitch*2]))*5 + 
                                                           static_cast<int>(pSrc[i-2+nSrcPitch*3]) + 32)>>6)));
        }
        for ( int i = nWidth - 4; i < nWidth - 1; ++i )
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + static_cast<unsigned int>(pSrc[i + 1]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch + 1]) + 2) >> 2);

        pDst[nWidth - 1] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[nWidth - 1]) + 
                                                       static_cast<unsigned int>(pSrc[nWidth + nSrcPitch - 1]) + 1) >> 1);
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    for ( int j = nHeight - 4; j < nHeight - 1; ++j ) {
        for ( int i = 0; i < nWidth - 1; i++ )
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + static_cast<unsigned int>(pSrc[i + 1]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch + 1]) + 2) >> 2);

        pDst[nWidth - 1] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[nWidth - 1]) + 
                                                       static_cast<unsigned int>(pSrc[nWidth + nSrcPitch - 1]) + 1) >> 1);
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    for ( int i = 0; i < nWidth - 1; ++i )
        pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + 
                                              static_cast<unsigned int>(pSrc[i + 1]) + 1) >> 1);
    pDst[nWidth - 1] = pSrc[nWidth - 1];
}

// bicubic (Catmull-Rom 4 taps interpolation)
void VerticalBicubic(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                     int nSrcPitch, int nWidth, int nHeight)
{
    for ( int j = 0; j < 1; ++j ) {
        for ( int i = 0; i < nWidth; ++i )
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch]) + 1) >> 1);
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    for ( int j = 1; j < nHeight - 3; ++j ) {
        for ( int i = 0; i < nWidth; ++i ) {
            pDst[i] = static_cast<unsigned char>(
                      std::min<int>(255, std::max<int>(0, (-static_cast<int>(pSrc[i-nSrcPitch]) - 
                                                           static_cast<int>(pSrc[i+nSrcPitch*2]) + 
                                                           (static_cast<int>(pSrc[i]) + 
                                                           static_cast<int>(pSrc[i+nSrcPitch]))*9 + 8)>>4)));
        }
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    for ( int j = nHeight - 3; j < nHeight - 1; ++j ) {
        for ( int i = 0; i < nWidth; ++i ) {
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch]) + 1) >> 1);
        }

        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    // last row
    for ( int i = 0; i < nWidth; ++i )
        pDst[i] = pSrc[i];
}

void HorizontalBicubic(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                       int nSrcPitch, int nWidth, int nHeight)
{
    for ( int j = 0; j < nHeight; ++j ) {
        pDst[0] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[0]) + static_cast<unsigned int>(pSrc[1]) + 1) >> 1);
        for ( int i = 1; i < nWidth - 3; ++i ) {   
            pDst[i] = static_cast<unsigned char>(
                      std::min<int>(255, std::max<int>(0, (-(static_cast<int>(pSrc[i-1]) + static_cast<int>(pSrc[i+2])) + 
                                                           (static_cast<int>(pSrc[i]) + static_cast<int>(pSrc[i+1]))*9 + 8)>>4)));
        }
        for ( int i = nWidth - 3; i < nWidth - 1; ++i )
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + static_cast<unsigned int>(pSrc[i + 1]) + 1) >> 1);

        pDst[nWidth - 1] = pSrc[nWidth - 1];
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
}

void DiagonalBicubic(unsigned char *pDst, const unsigned char *pSrc, int nDstPitch,
                     int nSrcPitch, int nWidth, int nHeight)
{
    for ( int j = 0; j < 1; ++j ) {
        for ( int i = 0; i < nWidth - 1; ++i )
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + static_cast<unsigned int>(pSrc[i + 1]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch + 1]) + 2) >> 2);

        pDst[nWidth - 1] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[nWidth - 1]) + static_cast<unsigned int>(pSrc[nWidth + nSrcPitch - 1]) + 1) >> 1);
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    for ( int j = 1; j < nHeight - 3; ++j ) {
        for ( int i = 0; i < 1; ++i )
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + static_cast<unsigned int>(pSrc[i + 1]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch + 1]) + 2) >> 2);
        for ( int i = 1; i < nWidth - 3; ++i ) {
            pDst[i] = static_cast<unsigned char>(
                      std::min<int>(255, std::max<int>(0, (-static_cast<int>(pSrc[i-1-nSrcPitch]) - 
                                                           static_cast<int>(pSrc[i+2+nSrcPitch*2]) + 
                                                           (static_cast<int>(pSrc[i]) + static_cast<int>(pSrc[i+1+nSrcPitch]))*9 - 
                                                           static_cast<int>(pSrc[i-1+nSrcPitch*2]) - 
                                                           static_cast<int>(pSrc[i+2-nSrcPitch]) + (static_cast<int>(pSrc[i+1]) + 
                                                           static_cast<int>(pSrc[i+nSrcPitch]))*9 + 16)>>5)));
        }
        for ( int i = nWidth - 3; i < nWidth - 1; ++i )
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + static_cast<unsigned int>(pSrc[i + 1]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch + 1]) + 2) >> 2);

        pDst[nWidth - 1] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[nWidth - 1]) + 
                                                       static_cast<unsigned int>(pSrc[nWidth + nSrcPitch - 1]) + 1) >> 1);
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    for ( int j = nHeight - 3; j < nHeight - 1; ++j ) {
        for ( int i = 0; i < nWidth - 1; i++ )
            pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + static_cast<unsigned int>(pSrc[i + 1]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch]) + 
                                                  static_cast<unsigned int>(pSrc[i + nSrcPitch + 1]) + 2) >> 2);

        pDst[nWidth - 1] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[nWidth - 1]) + 
                                                       static_cast<unsigned int>(pSrc[nWidth + nSrcPitch - 1]) + 1) >> 1);
        pDst += nDstPitch;
        pSrc += nSrcPitch;
    }
    for ( int i = 0; i < nWidth - 1; ++i )
        pDst[i] = static_cast<unsigned char>((static_cast<unsigned int>(pSrc[i]) + 
                                              static_cast<unsigned int>(pSrc[i + 1]) + 1) >> 1);
    pDst[nWidth - 1] = pSrc[nWidth - 1];
}

void Average2(unsigned char *pDst, const unsigned char *pSrc1, const unsigned char *pSrc2,
                     int nPitch, int nWidth, int nHeight)
{ // assume all pitches equal
    int nPitch1=nPitch-nWidth;

    for ( int j = 0; j < nHeight; ++j ) {
        for ( int i = 0; i < nWidth; ++i )
            *(pDst++) = static_cast<unsigned char>((static_cast<unsigned int>(*(pSrc1++)) + 
                                                    static_cast<unsigned int>(*(pSrc2++)) + 1) >> 1);

        pDst += nPitch1;
        pSrc1 += nPitch1;
        pSrc2 += nPitch1;
    }
}
